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

to_do = command + ' ' + ip + ' ' + str(int(port) + 1) + ' SET ' + ' data < data.txt'
print(to_do + '\n')
os.system(to_do)


f_out = open('out.log', 'w+')
default_stdout = os.dup(sys.stdout.fileno())
os.dup2(f_out.fileno(), sys.stdout.fileno())

f_err = open('err.log', 'w+')
default_stderr = os.dup(sys.stderr.fileno())
os.dup2(f_err.fileno(), sys.stderr.fileno())



for i in range(0, peercount):
    print('\n\n\n' + str(i))
    to_do = command + ' ' + ip + ' ' + str(int(port) + i) + ' GET ' + ' data'
    print(to_do)
    os.system(to_do)
    time.sleep(2.0)

    message1 = 'Error was found ! at peer ' + str(i)
    message2 = 'No answer ! from peer ' + str(i)
    message3 = 'No data was got ! from peer' + str(i)


    try:
        os.dup2(default_stderr, sys.stderr.fileno())
        test1 = no_err()
        test2 = is_answered()
        test3 = got_no_data()
        os.dup2(f_err.fileno(), sys.stderr.fileno())
    except IndexError:
        os.dup2(default_stderr, sys.stderr.fileno())
        print("file ended")
        os.dup2(f_err.fileno(), sys.stderr.fileno())
    assert test1,message1
    assert test2,message2
    assert test3,message3



os.dup2(default_stdout, sys.stdout.fileno())
os.dup2(default_stderr, sys.stderr.fileno())

