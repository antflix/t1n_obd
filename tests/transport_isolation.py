"""Compile actual receive functions against timed UART and slow HTTP fixtures."""
import pathlib, subprocess, tempfile

ROOT = pathlib.Path(__file__).resolve().parents[1]
PATH = 'firmware/t1n_obd/10_transport.ino'
current = (ROOT / PATH).read_text()
# Frozen SDK-sender baseline: these production functions serviced HTTP in waits.
previous = (ROOT / 'tests/fixtures/transport_receive_v28.ino').read_text()

def function(source, signature):
    start = source.index(signature)
    opening = source.index('{', start)
    depth = 1
    end = opening + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]

signatures = ['uint8_t checksum8(', 'bool validChecksum(',
              'bool extractEcuFrame(', 'size_t readRaw(',
              'size_t readUntilEcuFrame(']
fixture = r'''
uint32_t ticks=0;
uint32_t millis(){return ticks;}
void delay(uint32_t ms){ticks+=ms;}
struct {uint8_t testerAddr=0xF1;} cfg;
uint8_t activeEcuAddr=0x12;
struct FakeUart {
 uint32_t due=90;size_t at=0;std::vector<uint8_t> bytes;
 int available(){return (int32_t)(ticks-due)>=0?bytes.size()-at:0;}
 int read(){assert(available());return bytes[at++];}
} KL;
struct {unsigned calls=0;void handleClient(){calls++;ticks+=250;}} web;
void reset(uint32_t start=0,uint32_t arrival=90){
 ticks=start;KL.due=start+arrival;KL.at=0;
 KL.bytes={0x82,0xF1,0x12,0x61,0x01,0xE7};web.calls=0;
}
'''
program = '#include <cassert>\n#include <cstdint>\n#include <cstring>\n#include <vector>\n#include <iostream>\n'
for name, source in [('previous', previous), ('current', current)]:
    program += '\nnamespace ' + name + ' {\n' + fixture
    program += '\n'.join(function(source, signature) for signature in signatures)
    program += '\n}\n'
program += r'''
int main(){
 uint8_t raw[32]={},frame[32]={};size_t fn=0,n;
 previous::reset();
 n=previous::readUntilEcuFrame(raw,sizeof(raw),frame,fn,sizeof(frame),100);
 assert(n==0&&fn==0&&previous::web.calls==1&&previous::ticks>100);
 assert(previous::KL.available()==6); // Valid reply buffered, deadline already missed.
 previous::reset();n=previous::readRaw(raw,sizeof(raw),100,5);
 assert(n==0&&previous::web.calls==1&&previous::ticks>100);
 for(uint32_t start: {0U,0xFFFFFFE0U}){
  current::reset(start);
  n=current::readUntilEcuFrame(raw,sizeof(raw),frame,fn,sizeof(frame),100);
  assert(n==6&&fn==6&&current::validChecksum(frame,fn));
  assert(!memcmp(frame,current::KL.bytes.data(),fn));
  assert(uint32_t(current::ticks-start)==90&&current::web.calls==0);
  current::reset(start);n=current::readRaw(raw,sizeof(raw),100,5);
  assert(n==6&&!memcmp(raw,current::KL.bytes.data(),n));
  assert(uint32_t(current::ticks-start)==95&&current::web.calls==0);
  current::reset(start);current::KL.bytes.clear();
  n=current::readUntilEcuFrame(raw,sizeof(raw),frame,fn,sizeof(frame),100);
  assert(n==0&&fn==0&&uint32_t(current::ticks-start)==100&&current::web.calls==0);
  current::reset(start);current::KL.bytes.clear();n=current::readRaw(raw,sizeof(raw),100,5);
  assert(n==0&&uint32_t(current::ticks-start)==100&&current::web.calls==0);
  current::reset(start,101);
  n=current::readUntilEcuFrame(raw,sizeof(raw),frame,fn,sizeof(frame),100);
  assert(n==0&&fn==0&&uint32_t(current::ticks-start)==100&&current::web.calls==0);
  current::reset(start);current::KL.bytes.back()^=1;
  n=current::readUntilEcuFrame(raw,sizeof(raw),frame,fn,sizeof(frame),100);
  assert(n==6&&fn==0&&uint32_t(current::ticks-start)==100&&current::web.calls==0);
  current::reset(start);
  n=current::readUntilEcuFrame(raw,3,frame,fn,sizeof(frame),100);
  assert(n==3&&fn==0&&uint32_t(current::ticks-start)==100&&current::web.calls==0);
  current::reset(start);n=current::readRaw(raw,3,100,5);
  assert(n==3&&uint32_t(current::ticks-start)==95&&current::web.calls==0);
 }
 std::cout<<"Production UART receive: slow nested HTTP misses reply; isolated receive, quiet periods, timeout, late/corrupt frames, bounds and rollover PASS\n";
}
'''
with tempfile.TemporaryDirectory(prefix='t1n-uart-isolation-') as folder:
    source = pathlib.Path(folder) / 'receive.cpp'
    binary = pathlib.Path(folder) / 'receive'
    source.write_text(program)
    subprocess.run(['g++', '-std=c++17', '-Wall', '-Wextra', '-Werror',
                    str(source), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)

# Every other transport wait remains HTTP-free; only loop dispatches web work.
assert 'web.handleClient' not in current and 'waitMsWeb' not in current
assert function(current, 'void waitMs(').count('delay(1)') == 1
calls = [(p.name, p.read_text().count('web.handleClient('))
         for p in (ROOT / 'firmware/t1n_obd').glob('*.ino')]
assert [(name, count) for name, count in calls if count] == [('90_setup.ino', 1)]
