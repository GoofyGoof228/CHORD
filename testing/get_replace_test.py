import os
import time


peercount = 10
ip = '127.0.0.1'
command = '../client_osx'
port = 4000
time_interval = 0.0


to_do = command + ' ' + ip + ' ' + str(int(port) + 1) + ' SET ' + ' data < data.txt'
print(to_do + '\n\n')
os.system(to_do)

for i in range(0, peercount):
    print('\n\n' + str(i))
    to_do = command + ' ' + ip + ' ' + str(int(port) + i) + ' GET ' + ' data'
    print(to_do)
    os.system(to_do)

    to_do = command + ' ' + ip + ' ' + str(int(port) + i) + ' SET ' + ' data < data2.txt'
    print('\n' + to_do)
    os.system(to_do)
    to_do = command + ' ' + ip + ' ' + str(int(port) + i) + ' GET ' + ' data'
    print(to_do)
    os.system(to_do)

    to_do = command + ' ' + ip + ' ' + str(int(port) + i) + ' SET ' + ' data < data.txt'
    print(to_do)
    os.system(to_do)

    if time_interval != 0.0:
        time.sleep(time_interval)
