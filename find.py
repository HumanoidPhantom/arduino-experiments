import systemd.journal
import time
import sys

def main():
  j = systemd.journal.Reader()
  j.seek_tail()
  j.get_previous()
  while True:
    event = j.wait(-1)
    if event == systemd.journal.APPEND:
      for entry in j:
         out = entry['MESSAGE']
         if "ch341-uart converter detected" in out:
         	start = time.time()
         	print "Start timer"
         if "ch341-uart converter now disconnected" in out:
         	res = time.time() - start
         	print  "End timer: Result - " + str(res)
		sys.exit()
if __name__ == '__main__':
  main()
  sys.exit()
