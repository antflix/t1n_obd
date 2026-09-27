/*
  Sprinter T1N CR2 Scanner V1.3 - Configurable
  Target: 2001-2003 T1N OM612 CR2/EDC15C6 on OBD pin 7
  Hardware already proven on this van:
    ESP32 GPIO17 -> 1k -> P2N2222A base, emitter GND, collector K-line
    ESP32 GPIO16 <- LM311 output, 7.5k pull-up to 3.3V

  This scanner is built from the exact Autel replay / glitchless UART attach
  sequence that successfully opened CR2 on the vehicle.

  Features:
    - One-button CR2 connect using the proven full Autel preamble
    - Configurable post-C1 request (default 83 00), optional/non-blocking
    - Configurable TesterPresent keep-alive
    - Automatic reconnect status handling
    - Live polling of native CR2 local-identifier groups
    - Configurable native DTC read payload (default 18 02 00 00)
    - Configurable DTC clear payload (default 14 00 00) with UI confirmation
    - KWP payload terminal for additional services without hand-building checksum
    - Raw response display and decoded live-data display

  IMPORTANT:
    Several native live-data byte mappings came from the recovered T1N workbench
    and should still be treated as experimental until each value is cross-checked
    against a known-good scanner. Raw responses are always shown.
*/

#include <WiFi.h>
#include <WebServer.h>
#include <HardwareSerial.h>
#include <Preferences.h>
#include <driver/uart.h>
#include <Update.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <mbedtls/md.h>

// ---------------- Hardware ----------------
static const int TX_PIN = 17;
static const int RX_PIN = 16;
static const bool TX_DRIVE_LOW_HIGH = true;
static const bool RX_HIGH_MEANS_K_HIGH = true;

static const char* FIRMWARE_VERSION = "1.8.0";
WebServer web(80);
HardwareSerial KL(2);
Preferences prefs;

// ---------------- Configurable scanner settings ----------------
struct ScannerConfig {
  uint32_t baud = 10400;
  uint32_t byteSpacingUs = 5952;
  uint32_t fastLowUs = 25002;
  uint32_t fastFirstByteUs = 50082;
  uint32_t preIdleMs = 300;
  uint32_t slowBitUs = 200000;
  uint8_t slowAddress = 0x33;
  uint8_t ecuAddr = 0x12;
  uint8_t testerAddr = 0xF3;
  uint32_t rxFirstTimeoutMs = 700;
  uint32_t rxQuietMs = 25;
  uint32_t p3MinMs = 60;
  uint32_t c1WaitMs = 500;

  uint32_t rC133_1 = 0;
  uint32_t rC133_2 = 4282441;
  uint32_t rSlow   = 13775447;
  uint32_t rF7     = 15888088;
  uint32_t rVin1   = 16338504;
  uint32_t rVin2   = 16756626;
  uint32_t rVin3   = 17148459;
  uint32_t rA81_1  = 21685853;
  uint32_t rA81_2  = 25309547;
  uint32_t rA81_3  = 28843252;
  uint32_t rA82_1  = 29540752;
  uint32_t rA82_2  = 30223306;
  uint32_t rA82_3  = 30909506;
  uint32_t rCR2    = 33458185;

  bool sendPostC1 = true;
  bool requirePostC1 = false;
  uint32_t postC1DelayMs = 620;
  uint32_t postC1TimeoutMs = 700;
  uint32_t keepaliveIntervalMs = 1800;
  uint8_t keepaliveMissLimit = 3;
  uint32_t pollIntervalMs = 300;

  // K-line relay router. relayModule: 0=Engine, 1=ABS, 2=EGS, 255=unused.
  uint8_t relayGpio[4] = {25,26,27,32};
  uint8_t relayObdPin[4] = {7,9,11,15};
  uint8_t relayModule[4] = {0,1,2,255};
  bool relayActiveLow = true;

  String frameC133 = "C1 33 F1 81 66";
  String frameF7   = "F7";
  String frameVIN  = "68 6A F1 09 02 CE";
  String frameA81  = "81 01 F3 81 F6";
  String frameA82  = "81 01 F3 82 F7";
  String frameCR2  = "81 12 F3 81 07";
  String expectedSync = "55 08 08";
  String expectedComp = "CC";
  String expectedC1 = "83 F3 12 C1 EF 8F C7";

  String postC1Payload = "83 00";
  String postC1Expected = "C3 00";
  String testerPresentPayload = "3E";
  String testerPresentExpected = "7E";
  String dtcReadPayload = "18 02 00 00";
  String dtcClearPayload = "14 00 00";
  String disconnectPayload = "20";
  String pollGroups = "10,12,13,18,30";
} cfg;

// ---------------- Wi-Fi ----------------
String wifiSSID = "";
String wifiPASS = "";
String apSSID   = "T1N-Scanner";
String apPASS   = "sprinter123";

// ---------------- State ----------------
volatile bool uartAttached = false;
volatile bool jobConnect = false;
volatile bool busy = false;
bool cr2Connected = false;
bool diagnosticSession = false;
bool livePolling = false;
uint32_t nextPollMs = 0;
uint32_t lastGoodTrafficMs = 0;

enum ModuleId : uint8_t { MOD_ENGINE=0, MOD_ABS=1, MOD_EGS=2 };
bool relayGpioUsable(uint8_t pin);
void allRelaysOff();
void initRelayRouting();
int relayIndexForModule(ModuleId m);
bool selectRelayForModule(ModuleId m);
ModuleId activeModule = MOD_ENGINE;
uint8_t activeEcuAddr = 0x12;
ModuleId requestedModule = MOD_ENGINE;
bool autoReconnect = true;
uint32_t reconnectAtMs = 0;
uint8_t consecutiveRequestFailures = 0;

struct ValueState {
  double value = 0;
  String text = "--";
  String unit = "";
  bool valid = false;
  uint32_t updatedMs = 0;
};

ValueState engRpm, engCoolant, engIntake, engBoost, engRail, engLowFuel, engBattery, engSpeed, engEgr, engBoostDuty, engBoostTarget;
ValueState absWheelFL, absWheelFR, absWheelRL, absWheelRR, absVoltage, absWheelSensorV, absBrakeLamp, absBrakeSwitch, absPump, absOutletFL, absOutletFR;
ValueState egsTemp, egsGear, egsSelector, egsOutputRpm, egsTurbineRpm, egsVehicleSpeed, egsBattery;

void setNum(ValueState &v,double x,const String &unit,int decimals=1){v.value=x;v.unit=unit;v.text=String(x,decimals);v.valid=true;v.updatedMs=millis();}
void setTxt(ValueState &v,const String &x){v.text=x;v.unit="";v.valid=true;v.updatedMs=millis();}
bool stale(const ValueState &v,uint32_t age=3500){return !v.valid || millis()-v.updatedMs>age;}
String moduleName(ModuleId m){return m==MOD_ENGINE?"Engine / CR2":m==MOD_ABS?"ABS / ABSBR901":"Transmission / EGS52";}
uint8_t moduleAddr(ModuleId m){return m==MOD_ENGINE?0x12:m==MOD_ABS?0x34:0x20;}
uint8_t moduleObdPin(ModuleId m){for(int i=0;i<4;i++)if(cfg.relayModule[i]==(uint8_t)m)return cfg.relayObdPin[i];return m==MOD_ENGINE?7:m==MOD_ABS?9:11;}

uint32_t p3ReadyAtMs = 0;
uint8_t keepaliveMisses = 0;

String scannerStatus = "DISCONNECTED";
String lastTxHex = "";
String lastRxHex = "";
String lastError = "";
String dtcText = "No DTC read yet.";
String logText = "";
String activityLogText = "";

struct RxEvent { uint32_t us; uint8_t b; };
static const size_t EVENT_MAX = 1024;
RxEvent events[EVENT_MAX];
size_t eventCount = 0;
uint32_t replayT0 = 0;

struct LiveGroup {
  uint8_t id;
  const char *name;
  bool enabled;
  String raw;
  String decoded;
};

LiveGroup liveGroups[] = {
  {0x10, "OBD-style values", true,  "", ""},
  {0x12, "Primary sensors", true,  "", ""},
  {0x13, "Additional sensors / low fuel pressure", true, "", ""},
  {0x18, "Actuator duties / rail pressure", true, "", ""},
  {0x20, "Raw sensor voltages", false, "", ""},
  {0x22, "Diagnostic-converted sensors", false, "", ""},
  {0x28, "Cylinder-selective data", false, "", ""},
  {0x30, "Requested / target values", true, "", ""}
};
static const int LIVE_GROUP_COUNT = sizeof(liveGroups) / sizeof(liveGroups[0]);
int pollIndex = 0;

void markSessionLost(const String &why);
void notePollResult(bool ok);
