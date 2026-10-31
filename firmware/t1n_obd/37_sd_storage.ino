// Optional 3.3V SPI microSD storage for the T1N scanner.
// CS=GPIO22, CLK=GPIO18, MISO=GPIO19, MOSI=GPIO17, common ground.
// No flash history or NVS is modified by installing this support.
#include <SPI.h>
#include <SD.h>

static const uint8_t SD_CS_PIN=22, SD_CLK_PIN=18, SD_MISO_PIN=19, SD_MOSI_PIN=17;
static bool sdReady=false;
// Accessor used by the history and monitor modules without exposing internals.
bool sdCardReady(){return sdReady;}

static String sdLastError="SD not checked";
static File sdUploadFile;
static String sdUploadName, sdUploadError;
static size_t sdUploadBytes=0;
static bool sdUploadComplete=false;

bool sdAllowedName(const String &name) {
  return name=="index.html"||name=="advanced.html"||name=="pi.html"||name=="manifest.json";
}
void sdInitialize() {
  // K-line pins are deliberately excluded from the custom SPI routing.
  SPI.begin(SD_CLK_PIN,SD_MISO_PIN,SD_MOSI_PIN,SD_CS_PIN);
  sdReady=SD.begin(SD_CS_PIN,SPI,4000000);
  if(!sdReady){sdLastError="SD mount failed (check card, wiring, FAT32)";return;}
  if(SD.cardType()==CARD_NONE){SD.end();sdReady=false;sdLastError="No microSD card detected";return;}
  if(!SD.exists("/www")&&!SD.mkdir("/www")){
    SD.end();sdReady=false;sdLastError="Cannot create /www on card";return;
  }
  sdLastError="";
}
String sdStatusJson() {
  String out="{\"mounted\":"+String(sdReady?"true":"false")
    +",\"cs\":"+String(SD_CS_PIN)+",\"clk\":"+String(SD_CLK_PIN)
    +",\"miso\":"+String(SD_MISO_PIN)+",\"mosi\":"+String(SD_MOSI_PIN)
    +",\"error\":\""+jsonEscape(sdLastError)+"\"";
  if(sdReady){
    out+=",\"cardType\":"+String((int)SD.cardType())
      +",\"capacityMiB\":"+String((unsigned long)(SD.totalBytes()/1048576ULL))
      +",\"usedMiB\":"+String((unsigned long)(SD.usedBytes()/1048576ULL));
  }
  const char* names[]={"index.html","advanced.html","pi.html","manifest.json"};
  out+=",\"files\":[";
  for(int i=0;i<4;i++){
    if(i)out+=",";
    String name=names[i],path="/www/"+name;
    File file=sdReady?SD.open(path,FILE_READ):File();
    out+="{\"name\":\""+name+"\",\"present\":"+String(file?"true":"false")
       +",\"bytes\":"+String(file?file.size():0)+"}";
    if(file)file.close();
  }
  return out+"]}";
}
bool sdUiReady(){
  if(!sdReady)return false;
  File manifest=SD.open("/www/manifest.json",FILE_READ);
  if(!manifest||manifest.size()>4096){if(manifest)manifest.close();return false;}
  String body=manifest.readString();manifest.close();
  int key=body.indexOf("\"version\"");
  int colon=key<0?-1:body.indexOf(':',key+9);
  int quote=colon<0?-1:body.indexOf('"',colon+1);
  int end=quote<0?-1:body.indexOf('"',quote+1);
  return end>quote&&body.substring(quote+1,end)==FIRMWARE_VERSION;
}
bool sdServePage(const char*path) {
  if(!sdUiReady())return false;
  File f=SD.open(path,FILE_READ);
  if(!f||f.isDirectory()){if(f)f.close();return false;}
  web.sendHeader("Cache-Control","no-store");
  web.streamFile(f,"text/html; charset=utf-8");
  f.close();
  return true;
}
bool sdTestCard(String &result) {
  if(!sdReady){result=sdLastError;return false;}
  if(busy||diagnosticSession){result="Disconnect diagnostic session before SD test";return false;}
  static const char testData[]="T1N-SD-TEST-v1";
  const char*name="/.sdtest";
  SD.remove(name);
  File f=SD.open(name,FILE_WRITE);
  if(!f){result="Could not create test file";return false;}
  size_t n=f.write((const uint8_t*)testData,sizeof(testData)-1);
  f.flush();f.close();
  f=SD.open(name,FILE_READ);
  bool ok=f&&n==sizeof(testData)-1&&f.size()==sizeof(testData)-1;
  if(ok)for(size_t i=0;i<sizeof(testData)-1;i++)
    if(f.read()!=(uint8_t)testData[i]){ok=false;break;}
  if(f)f.close();
  SD.remove(name);
  result=ok?"microSD write / read / verify / delete passed":"microSD verification failed";
  return ok;
}
void sdUploadChunk() {
  HTTPUpload &u=web.upload();
  if(u.status==UPLOAD_FILE_START){
    sdUploadError="";sdUploadBytes=0;sdUploadComplete=false;
    sdUploadName=web.arg("name");
    if(!sdAllowedName(sdUploadName)){sdUploadError="Unsupported filename";return;}
    if(!sdReady){sdUploadError="microSD is not mounted";return;}
    if(busy||jobConnect||diagnosticSession){sdUploadError="Disconnect scanner before SD upload";return;}
    String part="/www/"+sdUploadName+".part";
    SD.remove(part);
    sdUploadFile=SD.open(part,FILE_WRITE);
    if(!sdUploadFile)sdUploadError="Cannot create temporary SD file";
  }else if(u.status==UPLOAD_FILE_WRITE){
    if(sdUploadError.length())return;
    if(sdUploadBytes+u.currentSize>524288){sdUploadError="SD asset exceeds 512 KiB";return;}
    if(!sdUploadFile||sdUploadFile.write(u.buf,u.currentSize)!=u.currentSize){
      sdUploadError="microSD write failed";return;
    }
    sdUploadBytes+=u.currentSize;
  }else if(u.status==UPLOAD_FILE_END){
    if(sdUploadFile){sdUploadFile.flush();sdUploadFile.close();}
    sdUploadComplete=sdUploadError.length()==0&&sdUploadBytes>0;
  }else if(u.status==UPLOAD_FILE_ABORTED){
    if(sdUploadFile)sdUploadFile.close();
    sdUploadError="Upload aborted";
  }
}
void sdUploadFinish() {
  if(sdUploadFile)sdUploadFile.close();
  String part="/www/"+sdUploadName+".part";
  if(sdUploadError.length()||!sdUploadComplete){
    if(sdReady&&sdAllowedName(sdUploadName))SD.remove(part);
    web.send(400,"text/plain",sdUploadError.length()?sdUploadError:"Incomplete upload");
    return;
  }
  String dest="/www/"+sdUploadName,bak=dest+".bak";
  SD.remove(bak);
  bool hadOld=SD.exists(dest);
  if(hadOld&&!SD.rename(dest,bak)){
    SD.remove(part);web.send(500,"text/plain","Could not back up existing asset");return;
  }
  if(!SD.rename(part,dest)){
    if(hadOld)SD.rename(bak,dest);
    web.send(500,"text/plain","Cannot activate new SD asset");return;
  }
  SD.remove(bak);
  web.send(200,"text/plain","Saved "+sdUploadName+" ("+String(sdUploadBytes)+" bytes)");
}
void sdServeAssetDownload() {
  String name=web.arg("name");
  if(!sdAllowedName(name)){web.send(400,"text/plain","Unknown SD asset");return;}
  if(!sdReady){web.send(503,"text/plain",sdLastError);return;}
  File f=SD.open("/www/"+name,FILE_READ);
  if(!f){web.send(404,"text/plain","SD asset absent");return;}
  web.sendHeader("Cache-Control","no-store");
  web.streamFile(f,name.endsWith(".json")?"application/json":"text/html; charset=utf-8");
  f.close();
}
