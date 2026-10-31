#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

// The adapter MUST use a nonblocking send: >0 bytes sent, 0 would block,
// negative disconnected/fatal error. cooperate() must return promptly. There
// is one deadline for the entire response, including its header and all chunks.
// No NetworkClient::write(), heap allocation, or per-chunk retry deadline.
class BoundedHttp {
 public:
  struct Operations {
    void *context;
    uint32_t (*nowMs)(void *);
    ptrdiff_t (*sendNonblocking)(const char *, size_t, void *);
    void (*cooperate)(void *);
    void (*close)(void *);
  };
  enum class Error { None, Timeout, Io, Length, State };
  explicit BoundedHttp(Operations operations, uint32_t budgetMs=250)
      : operations_(operations), startedMs_(operations_.nowMs(operations_.context)),
        budgetMs_(budgetMs>250?250:budgetMs) {}

  bool begin(size_t bodyLength,const char *contentType="application/json",const char *encoding=nullptr,const char *extraHeaders=nullptr) {
    if (begun_ || finished_ || failed()) return fail(Error::State);
    begun_=true; expectedBodyBytes_=bodyLength;
    char header[320];
    const int length=snprintf(header,sizeof(header),
      "HTTP/1.1 200 OK\r\nContent-Type: %s\r\n%s%s%s%s"
      "Content-Length: %lu\r\nConnection: close\r\nCache-Control: no-store\r\n\r\n",
      contentType,encoding?"Content-Encoding: ":"",encoding?encoding:"",encoding?"\r\n":"",extraHeaders?extraHeaders:"",(unsigned long)bodyLength);
    if (length<0 || (size_t)length>=sizeof(header)) return fail(Error::Length);
    return writeWire(header,(size_t)length);
  }
  bool write(const char *data,size_t length) {
    if (failed()) return false;
    if (!begun_ || finished_) return fail(Error::State);
    if (length>expectedBodyBytes_-bodyBytes_) return fail(Error::Length);
    const size_t before=bytesSent_;
    const bool ok=writeWire(data,length);
    bodyBytes_+=bytesSent_-before;
    return ok;
  }
  bool finish() {
    if (failed()) return false;
    if (!begun_ || finished_) return fail(Error::State);
    if (bodyBytes_!=expectedBodyBytes_) return fail(Error::Length);
    if (elapsed()>=budgetMs_) return fail(Error::Timeout);
    finished_=true;close();endedMs_=operations_.nowMs(operations_.context);return true;
  }
  bool failed() const {return error_!=Error::None;}
  Error error() const {return error_;}
  size_t bytesSent() const {return bytesSent_;}
  size_t bodyBytes() const {return bodyBytes_;}
  uint32_t durationMs() const {return (finished_||failed()?endedMs_:operations_.nowMs(operations_.context))-startedMs_;}

 private:
  uint32_t elapsed() const {return operations_.nowMs(operations_.context)-startedMs_;}
  void close() {if(!closed_){closed_=true;operations_.close(operations_.context);}}
  bool fail(Error error) {
    if (!failed()) {error_=error;close();endedMs_=operations_.nowMs(operations_.context);}
    return false;
  }
  bool writeWire(const char *data,size_t length) {
    if (failed()) return false;
    while (length) {
      if (elapsed()>=budgetMs_) return fail(Error::Timeout);
      const ptrdiff_t sent=operations_.sendNonblocking(data,length,operations_.context);
      if (sent<0 || (size_t)sent>length) return fail(Error::Io);
      if (sent) {data+=sent;length-=(size_t)sent;bytesSent_+=(size_t)sent;}
      else operations_.cooperate(operations_.context);
    }
    return true;
  }
  Operations operations_;
  uint32_t startedMs_,budgetMs_,endedMs_=0;
  size_t expectedBodyBytes_=0,bodyBytes_=0,bytesSent_=0;
  Error error_=Error::None;
  bool begun_=false,finished_=false,closed_=false;
};
