#!/usr/bin/env python3
import csv,hashlib,importlib.util,pathlib,tempfile,unittest
spec=importlib.util.spec_from_file_location('merge',pathlib.Path(__file__).resolve().parents[1]/'scripts/prepare_history_merge.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
class MergeTest(unittest.TestCase):
 def test_backup_dedup_chronology_no_overwrite_and_conflicts(self):
  with tempfile.TemporaryDirectory() as d:
   d=pathlib.Path(d);csvpath=d/'recovered.csv'
   csvpath.write_text('tier,unix_epoch,metric_hash_hex,min,max,average,sample_count\nrecent,1793300000,1234,10,10,10,1\nrecent,1793300000,1234,10,10,10,1\ndaily,1793200000,1234,1,3,2,4\n')
   sha=hashlib.sha256(csvpath.read_bytes()).hexdigest();current=d/'backup';current.mkdir()
   for name,_ in m.TIERS.values():(current/name).write_bytes(b'')
   original={p.name:p.read_bytes() for p in current.iterdir()}
   out=d/'prepared';report=m.prepare(csvpath,sha,current,out)
   self.assertEqual(report['duplicatesRemoved'],1);self.assertEqual(original,{p.name:p.read_bytes() for p in current.iterdir()})
   for name,_ in m.TIERS.values():self.assertEqual((out/'originals'/name).read_bytes(),original[name])
   with self.assertRaises(ValueError):m.prepare(csvpath,sha,current,out)
   with self.assertRaises(ValueError):m.prepare(csvpath,'0'*64)
   raw=m.RECORD.pack(1,1793200000,0x1234,1,4,3,4,0);(current/'hist_day.dat').write_bytes(raw[:28]+m.struct.pack('<I',m.checksum(raw[:28])))
   with self.assertRaises(ValueError):m.prepare(csvpath,sha,current,d/'conflicting')
   self.assertFalse((d/'conflicting').exists())
if __name__=='__main__':unittest.main()
