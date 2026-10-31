#pragma once
#include <lwip/sockets.h>
#include <errno.h>
#include "http_json.h"
struct HttpResponseMetrics {
  uint32_t completed=0,timeouts=0,failures=0,lastMs=0,maxMs=0;
};
static HttpResponseMetrics httpMetrics;
inline uint32_t scannerHttpNow(void*){return millis();}
inline ptrdiff_t scannerHttpSend(const char *data,size_t length,void *context){
  auto &client=*static_cast<NetworkClient*>(context);
  const int socket=client.fd();if(socket<0)return -1;
  int sent=::send(socket,data,length,MSG_DONTWAIT);
  if(sent<0&&(errno==EAGAIN||errno==EWOULDBLOCK||errno==EINTR))return 0;
  return sent;
}
inline void scannerHttpYield(void*){delay(1);}
inline void scannerHttpClose(void *context){static_cast<NetworkClient*>(context)->stop();}
inline BoundedHttp::Operations scannerHttpOperations(NetworkClient &client){
  return {&client,scannerHttpNow,scannerHttpSend,scannerHttpYield,scannerHttpClose};
}
inline void scannerHttpRecord(const BoundedHttp &response){
  httpMetrics.lastMs=response.durationMs();httpMetrics.maxMs=max(httpMetrics.maxMs,httpMetrics.lastMs);
  if(response.error()==BoundedHttp::Error::Timeout)httpMetrics.timeouts++;
  else if(response.failed())httpMetrics.failures++;
  else httpMetrics.completed++;
}
template <typename Emit> void sendScannerJson(Emit emit){
  NetworkClient client=web.client();
  BoundedHttp response(scannerHttpOperations(client));
  writeHttpJson(response,emit);scannerHttpRecord(response);
}
inline void sendScannerBody(const char *type,const char *body,size_t length,const char *encoding=nullptr,const char *extraHeaders=nullptr){
  NetworkClient client=web.client();
  BoundedHttp response(scannerHttpOperations(client));
  if(response.begin(length,type,encoding,extraHeaders))response.write(body,length);
  response.finish();scannerHttpRecord(response);
}
inline void sendScannerText(const char *type,const String &body,const char *extraHeaders=nullptr){
  sendScannerBody(type,body.c_str(),body.length(),nullptr,extraHeaders);
}
inline void sendScannerFile(File &file,const char *type){
  NetworkClient client=web.client();
  BoundedHttp response(scannerHttpOperations(client));
  if(response.begin(file.size(),type)){
    char buffer[1436];
    while(!response.failed()&&file.available()){
      const size_t n=file.read(reinterpret_cast<uint8_t*>(buffer),sizeof(buffer));
      if(!n)break;
      response.write(buffer,n);
    }
  }
  response.finish();scannerHttpRecord(response);
}
