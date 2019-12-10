import os
import time
import random


def choose_num(min, max):
    return random.randint(min, max)

stat_get = False
peercount = 5
ip = '127.0.0.1'
command = '../client_osx'
start_port = 4000
port = choose_num(start_port, start_port + peercount - 1)
time_interval = 0.0

to_do = command + ' ' + ip + ' ' + str(int(port)) + ' SET ' + ' data < data.txt'
print('\n' + to_do)
os.system(to_do)

if stat_get:
    print('\n\n\n')
    to_do = command + ' ' + ip + ' ' + str(int(start_port) + 2) + ' GET ' + ' data'
    print(to_do)
    os.system(to_do)
else:
    for i in range(0, peercount):
        print('\n\n\n' + str(i))
        to_do = command + ' ' + ip + ' ' + str(int(start_port) + i) + ' GET ' + ' data'
        print(to_do)
        os.system(to_do)
        if time_interval != 0.0:
            time.sleep(time_interval)
print('\n\n')

to_do = command + ' ' + ip + ' ' + str(int(start_port)) + ' DELETE ' + ' data'
os.system(to_do)
