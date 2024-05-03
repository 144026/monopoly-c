#!/usr/bin/python3
import os
import sys
import time
import random
import pexpect

start_prompt = "enter 'start' to play> "
end_prompt = "Player .* has won"

player_prompt = "\w+> "
menu_prompt = "input your choice\? "
bool_prompt = " \(y/n\) "

# from pty_spawn.py:
# Note this 'child' is global and used in sigwinch_passthrough.
import struct, fcntl, termios, signal
def sigwinch_passthrough (sig, data):
    s = struct.pack("HHHH", 0, 0, 0, 0)
    a = struct.unpack('hhhh', fcntl.ioctl(sys.stdout.fileno(),
        termios.TIOCGWINSZ , s))
    if not child.closed:
        child.setwinsize(a[0],a[1])

def terminate(sig, data):
    print("terminate")
    child.kill(signal.SIGTERM)

child = pexpect.spawn('./monopoly')
# pty device echos back what we send, only need to log read buffer
child.logfile_read = sys.stdout.buffer

rows, cols = os.popen("stty size").read().split()
child.setwinsize(int(rows), int(cols))

signal.signal(signal.SIGWINCH, sigwinch_passthrough)
signal.signal(signal.SIGINT, terminate)
signal.signal(signal.SIGTERM, terminate)

child.expect(start_prompt)
child.sendline('start')

child.expect('initial money')
child.sendline('4000')

child.expect('select number of player')
child.sendline('4')

for idx in range(1, 5):
    child.expect(menu_prompt)
    child.sendline('%s' % idx)

random.seed()

while True:
    try:
        i = child.expect([player_prompt, menu_prompt, bool_prompt, end_prompt])
    except:
        sys.exit(1)

    if i == 0:
        child.sendline(random.choice(['roll']))
    elif i == 1:
        child.sendline(random.choice(['1', '2']))
    elif i == 2:
        child.sendline(random.choice(['y', 'n']))
    elif i == 3:
        break
    time.sleep(0.5)

child.expect(start_prompt)
child.logfile_read = None
sys.exit(0)
