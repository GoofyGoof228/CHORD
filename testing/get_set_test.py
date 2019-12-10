import os
import time
import random

def choose_num(min, max):
    for x in range(10):
        return random.randint(min, max+1)


peercount = 10
ip = '127.0.0.1'
command = '../client_osx'
port = choose_num(4000, 4000+peercount)
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

