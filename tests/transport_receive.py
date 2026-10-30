#!/usr/bin/env python3
"""Compile actual receive loops with a slow HTTP client and timed UART replies."""
import pathlib, subprocess, tempfile

ROOT = pathlib.Path(__file__).resolve().parents[1]
source = (ROOT / 'firmware/t1n_obd/10_transport.ino').read_text()

def function(name):
    start = source.index(name + '(')
    start = source.rfind('\n', 0, start) + 1
    opening = source.index('{', start)
    depth = 1
    end = opening + 1
    while depth:
        if source[end] == '{': depth += 1
        elif source[end] == '}': depth -= 1
        end += 1
    return source[start:end]

wrapper = r'''
#include <cassert>
#include <cstdint>
#include <cstring>
#include <vector>
uint32_t clockMs=0;
uint32_t millis(){return clockMs;}
void delay(uint32_t n){clockMs+=n;}
struct {uint8_t testerAddr=0xf3;} cfg;
uint8_t activeEcuAddr=0x12;
struct UART {
  std::vector<uint8_t> bytes;size_t offset=0;uint32_t ready=80;
  int available(){return (int32_t)(clockMs-ready)>=0 ? bytes.size()-offset : 0;}
  int read(){return bytes.at(offset++);}
} KL;
struct HTTP {int calls=0;void handleClient(){calls++;clockMs+=1000;}} web;
'''
wrapper += '\n'.join(function(n) for n in ['checksum8','validChecksum','extractEcuFrame','readRaw','readUntilEcuFrame'])
wrapper += r'''
void reset(uint32_t now=0){
 clockMs=now;KL.offset=0;KL.ready=now+80;KL.bytes={0x81,0xf3,0x12,0x61,0};
 KL.bytes.back()=checksum8(KL.bytes.data(),4);web.calls=0;
}
int main(){
 uint8_t raw[16]={},frame[16]={};size_t size=0;
 reset();assert(readUntilEcuFrame(raw,16,frame,size,16,700)==5);
 assert(size==5 && clockMs==80 && web.calls==0);
 reset();assert(readRaw(raw,16,700,25)==5);
 assert(clockMs==105 && web.calls==0);
 reset();KL.bytes.clear();assert(readUntilEcuFrame(raw,16,frame,size,16,100)==0);
 assert(size==0 && clockMs==100 && web.calls==0);
 reset();KL.bytes.back()^=1;assert(readUntilEcuFrame(raw,16,frame,size,16,100)==5);
 assert(size==0 && clockMs==100);
 reset(UINT32_MAX-30);assert(readUntilEcuFrame(raw,16,frame,size,16,700)==5);
 assert(size==5 && (uint32_t)(clockMs-(UINT32_MAX-30))==80);
 reset();uint8_t bounded[4]={0,0,0xa5,0x5a};
 assert(readUntilEcuFrame(bounded,2,frame,size,16,100)==2);
 assert(size==0 && bounded[2]==0xa5 && bounded[3]==0x5a);
}
'''
with tempfile.TemporaryDirectory() as d:
    cpp = pathlib.Path(d)/'receive.cpp'; cpp.write_text(wrapper)
    binary = str(pathlib.Path(d)/'receive')
    subprocess.run(['g++','-std=c++17','-Wall','-Wextra','-Werror',str(cpp),'-o',binary],check=True)
    subprocess.run([binary],check=True)
print('Actual UART receive: slow HTTP exclusion, silence deadline, checksum, rollover and bounded buffer PASS')
