import logging
import subprocess
import random
import threading
import numpy as np
import time
import unittest
import os
import platform
from queue import Queue, Empty
import socket, errno
import string
import filecmp

formatter = logging.Formatter('%(asctime)s : %(message)s\n')

#http://eyalarubas.com/python-subproc-nonblock.html

class Logger:

	def __init__(self,name, level=logging.INFO):
		# TODO: This is may not clean folder creation: Better use abspath
		if not os.path.exists("logs"):
			os.makedirs("logs")
		self.logger = logging.getLogger(name)
		self.logger.setLevel(level)
		self.logger.addHandler(handler)
		self.info("Searching for pen and paper... Let's move it!")

	def info(self, line):
		self.logger.info(line)

class Tests(unittest.TestCase):

	def port_in_use(self, ip, port, logger):

		s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

		try:
			s.bind((ip, port))
		except socket.error as e:
			if e.errno == errno.EADDRINUSE:
				logger.info("Port is already in use!")
				return True
			else:
				logger.info(e) # something else raised the socket.error exception
				return True
		s.close()
		return False

	def reader(self,process, pipe, queue):
		try:
			print("Starting std thread"+str(pipe))
			with pipe:
				while True:
					output = pipe.readline()
					if (output == b'' or output == '') and process.poll() != None:
						queue.put((pipe, "TERMINATING"))
						break
					if output:

						queue.put((pipe, output))
		finally:
			queue.put((None,None))

	def thread_function(self, cmd, thread_identifier, out_logger, err_logger):

		if self.port_in_use(str(cmd[1]),int(cmd[2]),err_logger):
			print("ADDRESS ALREADY IN USE: This is may an issue if a client is started!")

		# start the subprocess
		process = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True)

		# init a queue to save stdouts and stderrs
		queue = Queue()

		# start parsing stds with readerThread - since readline will block system
		stdout_thread = threading.Thread(target=self.reader, args=(process, process.stdout, queue))
		stderr_thread = threading.Thread(target=self.reader, args=(process, process.stderr, queue))

		stdout_thread.start()
		stderr_thread.start()

		while True:

			# get the outputs from stderr and stdout
			(queue_element,output) = queue.get()

			if queue_element == process.stdout:
				out_logger.info(output)

			if queue_element == process.stderr:
				err_logger.info(output)
			# the next line may intentionally left
			if queue_element == None:
				pass
			# if process child terminated
			if process.poll() != None:
				# wait that other threads (readers) terminate
				stdout_thread.join()
				stderr_thread.join()
				break


	def test_init_peers(self):
		# No need. This just looks cool, if 100 Terminals opening
		# terminal_invocation = 'gnome-terminal -x'
		# terminal_invocation_osx = ['osascript -e \'tell app \"Terminal\" \n do script \"', '\"\n  end tell\' ']

		self.abs_build_folder = 'cmake-build-debug'
		self.ip = '127.0.0.1'

		self.peercount = 10
		startport = 4000
		self.portrange = np.linspace(startport, startport+self.peercount-1, self.peercount)


		self.cmd_lst = [['..'+os.path.sep+self.abs_build_folder+os.path.sep+'peer', self.ip, str(startport)]]

		self.used_ports = [str(startport)]

		idstart = 1
		idend = 65000
		self.idrange = np.linspace(idstart, idend, self.peercount)

		self.used_id = [0]

		self.running_peers_lst = [[self.ip, startport]]

		for i in range(1, self.peercount):
			# Let"s get a little randomness
			dst_peer = random.choice(self.running_peers_lst)
			choosenid = random.choice(self.idrange)

			self.used_id.append(int(choosenid))
			self.idrange = np.delete(self.idrange, np.argwhere(self.idrange == choosenid)[0][0])

			self.cmd_lst.append(['../'+self.abs_build_folder+'/peer', str(self.ip), str(int(self.portrange[i])), str(int(choosenid)), str(dst_peer[0]), str(dst_peer[1])])
			self.running_peers_lst.append([str(self.ip), str(int(self.portrange[i]))])

		#print(self.running_peers_lst)

	def test_start_peer(self):
		self.test_init_peers()

		for i in range(0,self.peercount):

			# set name for logger- and filenamesyntax
			filename_stderror = "PEER_ERR_"+str(i)+".txt"
			filename_stdout   = "PEER_OUT_"+str(i)+".txt"

			# init loggers
			logger_stdout = Logger(filename_stdout).logger
			logger_stderr = Logger(filename_stderror).logger

			logger_stdout.info("Starting job "+str(i)+" with Peer ID"+str(self.cmd_lst[i][2]))
			logger_stderr.info("Starting job "+str(i)+" with Peer ID"+str(self.cmd_lst[i][2]))

			x = threading.Thread(target=self.thread_function, args=(self.cmd_lst[i], str(i), logger_stdout, logger_stderr ))
			print(self.cmd_lst[i])
			x.start()

			time.sleep(0.2) # some time to make sure the peer is started

			#x.join() is missing cause there is more code coming and the peers are not allowed to shut down

	def get_keys_and_data_path(self,path,folder):
		file_list = []
		for file in os.listdir(path+folder):
			file_list.append([file,path+folder+file])
		return file_list

	def get_random_keys_and_data_path(self,path,folder,nullbyte=False):
		file_list = []
		for file in os.listdir(path+folder):
			keyword = []
			for k in range(0,random.randint(1,10)):
				keyword.extend(random.sample(string.printable, random.randint(0,len(string.printable)-1)))
			keyword = "".join(keyword).replace("\t","").replace("\n","").replace("\r","").replace(" ","").replace("\x0b","").replace("\x0c","").strip()

			# randomness not goot enough -> no starting nullbyte
			if nullbyte == True:
				keyword ="\x00".join(keyword[i-1:i+1] for i,c in enumerate(keyword) if i%random.randint(1,9) )
			#print("KEYWORD: ",keyword)

			file_list.append([keyword,path+folder+file])
		return file_list

	def is_nullbyte_in_keyword(self,keyword):
		if "\x00" in keyword:
			return True
		return False

	def create_nullbyte_keywords(self,path,folder):
		return self.get_random_keys_and_data_path(path,folder,True)

	def setter_client(self,port,key_name,file_name,ip = None):
		#./client localhost 4711 SET /pics/cat.jpg < cat.jpg
		if ip == None:
			ip = self.client_ip
		return ['..'+os.path.sep+self.abs_build_folder+os.path.sep+self.client_name, ip, str(port), "SET", str(key_name), "<", str(file_name)]

	def getter_client(self,port,key_name,file_name,ip = None):
		#./client localhost 4711 GET /pics/cat.jpg > cat.jpg
		if ip == None:
			ip = self.client_ip
		return ['..'+os.path.sep+self.abs_build_folder+os.path.sep+self.client_name, ip, str(port), "GET", str(key_name), ">", str(file_name)]

	def del_client(self,port,key_name,ip = None):
		#./client localhost 4711 DELETE /pics/cat.jpg
		if ip == None:
			ip = self.client_ip
		return ['..'+os.path.sep+self.abs_build_folder+os.path.sep+self.client_name, ip, str(port), "DELETE", str(key_name)]

	def setter_client_with_random_key(self,port,file_name = None, ip = None):
		#./client localhost 4711 SET /pics/cat.jpg < cat.jpg
		if ip == None:
			ip = self.client_ip
		key_name = []
		for k in range(0,random.randint(1,10)):
			key_name.extend(random.sample(string.ascii_letters, random.randint(0,len(string.ascii_letters)-1)))
		key_name = "".join(key_name).replace("\t","").replace("\n","").replace("\r","").replace(" ","").replace("\x0b","").replace("\x0c","").strip()
		if file_name == None:
			file_name = random.choice(self.keys_and_paths_pool)[1]
		self.keys_and_paths_pool.append([key_name, file_name])
		return ['..'+os.path.sep+self.abs_build_folder+os.path.sep+self.client_name, ip, str(port), "SET", str(key_name), "<", str(file_name)]

	def test_start_client(self):

		self.client_ip = '127.0.0.1'
		self.abs_build_folder = 'cmake-build-debug'
		self.client_name = 'client'
		self.data_folder = 'data'+os.path.sep
		self.path = os.path.abspath(__file__)[:-len(__file__)]
		self.use_nullbytes_in_keys = True



		self.keys_and_paths_pool      = self.get_keys_and_data_path(self.path,self.data_folder)
		self.keys_and_paths_pool.extend(self.get_random_keys_and_data_path(self.path,self.data_folder))
		if self.use_nullbytes_in_keys:
			self.keys_and_paths_pool.extend(self.create_nullbyte_keywords(self.path,self.data_folder))

		self.client_args_used = []

	def test_all_peers_running(self):
		self.test_start_client()
		self.test_start_peer()

		a = 0
		#TODO: check if threads are running instead of running_peers_list
		while len(self.running_peers_lst) != self.peercount:
			print("Waiting until all peers are started")
			print("Missing:"+str(self.peercount-len(self.running_peers_lst))+" Peers")
			a += 1
			time.sleep(2)
			if (a == 20):
				self.assertFalse(False, msg="Starting all Peers took too long")

	def test_unique_get_to_every_peer(self):
		self.test_start_client()
		self.test_start_peer()
		time.sleep(5)
		#self.running_peers_lst.append([str(self.ip), str(int(self.portrange[i]))])
		peer_box = self.running_peers_lst.copy()

		self.client_count = self.peercount

		self.cmd_clients = []
		for peers in range(self.client_count):
			peer = random.choice(peer_box)
			self.cmd_clients.append(self.setter_client_with_random_key(peer[1]))
			peer_box.remove(peer)
			print(peer_box)
		del peer_box

	def test_start_clients_1(self):
		self.test_unique_get_to_every_peer()

		for i in range(0,self.client_count):

			# set name for logger- and filenamesyntax
			filename_stderror = "CLIENT_ERR_"+str(i)+".txt"
			filename_stdout   = "CLIENT_OUT_"+str(i)+".txt"

			# init loggers
			logger_stdout = Logger(filename_stdout).logger
			logger_stderr = Logger(filename_stderror).logger

			#logger_stdout.info("Starting job "+str(i)+" with Peer ID"+str(self.cmd_lst[i][2]))
			#logger_stderr.info("Starting job "+str(i)+" with Peer ID"+str(self.cmd_lst[i][2]))

			x = threading.Thread(target=self.thread_function, args=(self.cmd_clients[i], str(i), logger_stdout, logger_stderr ))
			x.start()
			self.client_args_used.append([self.cmd_clients[i][4],self.cmd_clients[i][6]])
			#time.sleep(0.2) # some time to make sure the peer is started

		for k,i in enumerate(self.client_args_used):
			print(self.cmd_clients[k])
			print(k,i)

		"""
		if self.client_ip != self.ip:
			raise ValueError("We just test on the same network")

		startport = 2000

		self.client_portrange = np.linspace(startport, startport+self.client_count-1, self.client_count)

		cmd_lst = []
		"""

if __name__ == '__main__':
	if platform.system() == "Linux":
		unittest.main()
	else:
		print("PLEASE RUN THIS CODE ON LINUX")
		unittest.main()

