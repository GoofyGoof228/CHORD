import logging
import subprocess
import random
import threading
import numpy as np
import time
import unittest
import os
import platform
formatter = logging.Formatter('%(asctime)s : %(message)s\n')

class Logger:

	def __init__(self,name, level=logging.INFO):
		if not os.path.exists("logs"):
			os.makedirs("logs")

		handler = logging.FileHandler("logs"+os.path.sep+name)
		handler.setFormatter(formatter)

		self.logger = logging.getLogger(name)
		self.logger.setLevel(level)
		self.logger.addHandler(handler)
		self.logger.info("Cleaning dust from Memory ... and injecting Remote Code")

	def writeToFile(self, line):
		if self.Logger.propagate == True:
			self.Logger.debug(line)

	def closeFile(self):
		if self.Logger.propagate == True:
			self.Logger.propagate = False


class Tests(unittest.TestCase):
	def start_peers(self):
		"""
				stdout_logger = logging.getLogger('STDOUT')
		sl = StreamToLogger(stdout_logger, logging.INFO)
		sys.stdout = sl

		stderr_logger =
		sl =
		sys.stderr = sl
		"""
		#print "Test to standard out"
		#raise Exception('Test to standard error')

		terminal_invocation = 'gnome-terminal -x'
		terminal_invocation_osx = ['osascript -e \'tell app \"Terminal\" \n do script \"', '\"\n  end tell\' ']

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

		def thread_function(cmd,threadnumber,out_logger,err_logger):

			out_logger.info("Starting new Peer "+ threadnumber)
			process = subprocess.Popen(cmd, stdout=subprocess.PIPE,stderr=subprocess.PIPE,universal_newlines=True)
				#print(process.communicate())
			while True:
				output = process.stderr.readline()
				std_out = process.stdout.readline()
				if (output == b'' or output == '') and process.poll() != None:
					out_logger.info("\nThread "+threadnumber+" is about to terminate")
					err_logger.info("\nThread "+threadnumber+" is about to terminate")
					break
				if output:
					err_logger.info(output)

				if std_out:
					out_logger.info(std_out)

			rc = process.poll()
			err_logger.info("Terminated")
			out_logger.info("Terminated")

		for i in range(1,peercount):
			dst_peer = random.choice(peer_lst)
			cmd_lst.append(['../'+abs_build_folder+'/peer',str(ip),str(int(portrange[i])),str(int(idrange[i])),str(dst_peer[0]),str(dst_peer[1])])
			peer_lst.append([str(ip),str(int(portrange[i]))])

		for i in range(0,peercount):
			print(cmd_lst[i])
			stderror = "PEER_ERR_"+str(i)+".txt"
			stdout = "PEER_OUT_"+str(i)+".txt"

			x = threading.Thread(target=thread_function, args=(cmd_lst[i],str(i),Logger(stdout).logger,Logger(stderror).logger))
			x.start()
			time.sleep(0.1)

if __name__ == '__main__':
	unittest.main()
