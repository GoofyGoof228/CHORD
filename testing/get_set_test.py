import os
import time
import sys


def no_err():
    with open('err.log') as f:
        if 'error' in f.read():
            return False
        if 'Error' in f.read():
            return False
        return True


def is_answered():
    with open('out.log', 'r') as f:
        lines = f.readlines()
        for i in range(0, len(lines)):
            line = lines[i]
            if line == 'Sending: \n':
                next_line = lines[i+2]
                if next_line != 'recieved back: \n':
                    return False
        return True


def got_no_data():
    with open('out.log', 'r') as f:
        lines = f.readlines()
        for i in range(0, len(lines)):
            line = lines[i]
            if line == 'recieved back: \n':
                next_line = lines[i+2]
                if next_line != 'datadatadata' and next_line != 'datadatadata\n':
                    return False
        return True


peercount = 10
ip = '127.0.0.1'
command = '../client_osx'
port = 4000
time_interval = 0.0

to_do = command + ' ' + ip + ' ' + str(int(port) + 1) + ' SET ' + ' data < data.txt'
print(to_do + '\n')
os.system(to_do)

for i in range(0, peercount):
    print('\n\n\n' + str(i))
    to_do = command + ' ' + ip + ' ' + str(int(port) + i) + ' GET ' + ' data'
    print(to_do)
    os.system(to_do)
    if time_interval != 0.0:
        time.sleep(time_interval)
print('\n\n')
to_do = command + ' ' + ip + ' ' + str(int(port) + 1) + ' DELETE ' + ' data'
os.system(to_do)

