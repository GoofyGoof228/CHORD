import logging
import subprocess
import random
import threading
import numpy as np
import time
import os, sys

format = "%(asctime)s: %(message)s"
logging.basicConfig(format=format, level=logging.INFO, datefmt="%H:%M:%S",filename='peer.log.txt',filemode='w')

terminal_invocation = 'gnome-terminal -x'
abs_build_folder = 'cmake-build-debug'
ip = '127.0.0.1'

peercount = 50
startport = 4000
portrange = np.linspace(startport,startport+peercount-1,peercount)

cmd_lst = [['../'+abs_build_folder+'/peer',ip,str(startport)]]

idstart = 1
idend = 65000
idrange = np.linspace(idstart,idend,peercount)

peer_lst = [[ip,startport]]

def thread_function(cmd,threadnumber):
	logging.info("Starting new Peer "+ threadnumber)

	file_name = 'peer_' + str(threadnumber) + '_stdou.log'
	log_out = open(file_name, 'w+')
	fd_log = os.fd

	process = subprocess.Popen(cmd, stdout=subprocess.PIPE,stderr=subprocess.PIPE,universal_newlines=True)
	#print(process.communicate())
	while True:
		output = process.stderr.readline()
		if (output == b'' or output == '') and process.poll() != None:
			logging.info("Thread "+threadnumber+": is terminating")
			break
		if output:
			logging.info("Thread "+threadnumber+":"+output.strip())
			print(threadnumber,output.strip())
	rc = process.poll()
	print("\n\nEND :"+threadnumber)

for i in range(1,peercount):
	dst_peer = random.choice(peer_lst)
	cmd_lst.append(['../'+abs_build_folder+'/peer',str(ip),str(int(portrange[i])),str(int(idrange[i])),str(dst_peer[0]),str(dst_peer[1])])
	peer_lst.append([str(ip),str(int(portrange[i]))])

for i in range(0,peercount):
	print(cmd_lst[i])
	x = threading.Thread(target=thread_function, args=(cmd_lst[i],str(i),))
	x.start()
	time.sleep(0.1)


