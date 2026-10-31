#pragma once
#include <lwip/sockets.h>
#include <errno.h>
#include "http_json.h"
struct HttpResponseMetrics {
  uint32_t timeouts=0,failures=0,maxMs=0;
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
  httpMetrics.maxMs=max(httpMetrics.maxMs,response.durationMs());
  if(response.error()==BoundedHttp::Error::Timeout)httpMetrics.timeouts++;
  else if(response.failed())httpMetrics.failures++;
}
template <typename Emit> void sendScannerJson(Emit emit){
  NetworkClient client=web.client();
  BoundedHttp response(scannerHttpOperations(client));
  writeHttpJson(response,emit);scannerHttpRecord(response);
}
