import os
import time
peercount = 10
ip = '127.0.0.1'
command = '../client_osx'
port = 4000

to_do = command + ' ' + ip + ' ' + str( int(port) + 1) + ' SET ' + ' data < data.txt'
print(to_do + '\n')
os.system(to_do)

for i in range(0,10):
    print('\n\n\n' + str(i))
    to_do = command + ' ' + ip + ' ' + str( int(port) + i) + ' GET ' + ' data'
    print(to_do)
    os.system(to_do)
    time.sleep(3.0)

