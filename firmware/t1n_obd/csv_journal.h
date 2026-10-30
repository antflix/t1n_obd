#pragma once
#include <SD.h>
// Interrupted CSV rows remain visible as incomplete lines; isolate them before
// retrying so they cannot absorb the next complete event. No existing byte is
// erased. Callers advance their saved snapshot only after a complete row write.
inline bool sdAppendCsv(const char *path,const char *row,size_t length){
  if(!sdCardReady())return false;
  bool needsNewline=false;
  File previous=SD.open(path,"r");
  if(previous){
    if(previous.size()){
      if(!previous.seek(previous.size()-1,SeekSet)){previous.close();return false;}
      int last=previous.read();if(last<0){previous.close();return false;}
      needsNewline=last!='\n';
    }
    previous.close();
  }else if(SD.exists(path))return false;
  File file=SD.open(path,"a");if(!file)return false;
  if(needsNewline&&file.write((const uint8_t*)"\n",1)!=1){file.close();return false;}
  const bool okay=file.write((const uint8_t*)row,length)==length;
  file.flush();file.close();return okay;
}
