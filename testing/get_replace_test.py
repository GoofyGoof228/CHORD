import os
import time
import random


def choose_num(min, max):
    return random.randint(min, max+1)


peercount = 10
ip = '127.0.0.1'
command = '../client_osx'
start_port = 4000
port = choose_num(start_port, start_port+peercount)
time_interval = 0.0


to_do = command + ' ' + ip + ' ' + str(choose_num(4000, 4000+peercount)) + ' SET ' + ' data < data.txt'
print(to_do + '\n\n')
os.system(to_do)

for i in range(0, peercount):
    print('\n\n' + str(i))
    to_do = command + ' ' + ip + ' ' + str(int(start_port) + i) + ' GET ' + ' data'
    print(to_do)
    os.system(to_do)

    to_do = command + ' ' + ip + ' ' + str(choose_num(start_port, start_port+peercount)) + ' SET ' + ' data < data2.txt'
    print('\n' + to_do)
    os.system(to_do)
    to_do = command + ' ' + ip + ' ' + str(int(start_port) + i) + ' GET ' + ' data'
    print(to_do)
    os.system(to_do)

    to_do = command + ' ' + ip + ' ' + str(choose_num(start_port, start_port+peercount)) + ' SET ' + ' data < data.txt'
    print(to_do)
    os.system(to_do)

    if time_interval != 0.0:
        time.sleep(time_interval)
