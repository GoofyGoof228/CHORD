import logging
import subprocess
import random
import threading
import numpy as np
import time
import unittest
import os
import sys
import platform
from queue import Queue, Empty
import socket, errno
import string
import filecmp
from configparser import ConfigParser
import argparse
from textwrap import dedent

formatter = logging.Formatter('%(asctime)s : %(message)s\n')

#http://eyalarubas.com/python-subproc-nonblock.html

class Logger:

	def __init__(self,name, level=logging.INFO):
		# TODO: This is may not clean folder creation: Better use abspath
		if not os.path.exists("logs"):
			os.makedirs("logs")

		handler = logging.FileHandler("logs"+os.path.sep+name)
		handler.setFormatter(formatter)

		self.logger = logging.getLogger(name)
		self.logger.setLevel(level)
		self.logger.addHandler(handler)
		self.info("New Session started!")


	def info(self, line):
		self.logger.info(line)

class cThread:

	def __init__(self,cmd,thread_identifier,out_logger,err_logger):
		self.thread_function(cmd,thread_identifier, out_logger, err_logger)


	def reader(self,process, pipe, queue):
		try:
			print("Starting std thread"+str(pipe))
			with pipe:
				while True:
					output = pipe.readline().strip()
					if (output == b'' or output == '') and process.poll() != None:
						queue.put((pipe, str(process)+" TERMINATING"))
						break
					if output and (output != '' or output == b''):
						#if not 'STABILIZE' in output:
						#if not 'JOIN' in output:
						#if not 'NOTIFY' in output:
						queue.put((pipe, output))
		finally:
			queue.put((None,None))

	def thread_function(self, cmd, thread_identifier, out_logger, err_logger):

		#if self.port_in_use(str(cmd[1]),int(cmd[2]),err_logger):
		#print("ADDRESS ALREADY IN USE: This is may an issue if a client is started!")

		# start the subprocess
		process = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE,shell=True,universal_newlines=True)

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
			#self.assertFalse(True,msg="There was an error!")
			# the next line may intentionally left
			if queue_element == None:
				pass
			# if process child terminated
			if process.poll() != None and queue.qsize() == 0:
				# wait that other threads (readers) terminate
				#stdout_thread.join()
				#stderr_thread.join()
				err_logger.info("LOGGER TERMINATING")
				out_logger.info("LOGGER TERMINATING")
				break


class Peer:

	def __init__(self,configname = 'peer_setup.ini' ):

		parser = ConfigParser()
		parser.read(configname)

		self.abs_build_folder = parser.get('PEER_SETUP','path_to_buildfolder')
		self.path = os.path.abspath(__file__)[:-len(__file__)]
		self.ip = parser.get('PEER_SETUP','ip')
		self.peercount = int(parser.get('PEER_SETUP','peer_count'))
		self.start_port = int(parser.get('PEER_SETUP', 'startport'))
		self.end_port = int(parser.get('PEER_SETUP', 'endport'))
		self.id_start = int(parser.get('PEER_SETUP','id_start'))
		self.id_end = int(parser.get('PEER_SETUP', 'id_end'))
		self.id_setting = int(parser.get('PEER_SETUP','id_setting'))
		self.peer_file = parser.get('PEER_SETUP', 'peer_file')
		self.dst_peer = int(parser.get('PEER_SETUP','dst_peer'))
		self.port_range = int(parser.get('PEER_SETUP','port_range'))
		self.swap_folder = parser.get('PEER_SETUP','swap_folder')
		self.swap_file = parser.get('PEER_SETUP','running_peer_list')

		self.init_peers()
		self.start_peer()
		self.peerinfo_to_file(self.swap_file)

	def peerinfo_to_file(self,filename):
		if not os.path.exists(self.swap_folder):
			os.makedirs(self.path+self.swap_folder)

		with open(self.path+self.swap_folder+os.path.sep+filename,'w') as pool:
			for element in self.running_peers_lst:
				pool.write(str(element[0])+","+str(element[1])+"\n")

	# TODO: enhancement - combine port+id method
	def get_choosen_port(self):

		if self.port_range == 1:
			self.portrange = np.linspace(self.start_port, self.end_port, self.peercount)
		elif self.port_range == 0:
			self.portrange = [k for k in range(self.start_port,self.start_port+self.peercount)]
		else:
			self.portrange = []
			for peer_counter in range(1,self.peercount+1):
				choosen_port = random.choice([k for k in range(self.start_port,self.end_port)])
				while choosen_port in self.portrange:
					choosen_port = random.choice([k for k in range(self.start_port,self.end_port)])
				self.portrange.append(choosen_port)

	def get_choosen_id(self):

		if self.id_setting == 0: # linear id range 0,1,2,3,4...
			self.idrange = [k for k in range(self.id_start-1,self.id_end)] # dirty but -1 because for loop use later
		elif self.id_setting == 1: # linear splitted
			self.idrange = np.linspace(self.id_start, self.id_end, self.peercount)
		else: # choose random ids from
			self.idrange = [0]
			for peer_counter in range(1,self.peercount+1):
				choosen_id = random.choice([k for k in range(self.id_start,self.id_end)])
				while choosen_id in self.idrange:
					choosen_id = random.choice([k for k in range(self.id_start,self.id_end)])
				self.idrange.append(choosen_id)

	def get_dst_peer(self, incrementer):
		if self.dst_peer == 1:
			self.dstpeer = random.choice(self.running_peers_lst)
		else:
			self.dstpeer = self.running_peers_lst[incrementer-1]


	def init_peers(self):

		self.get_choosen_port()
		#TODO: check if not enough ids available for started clients
		self.get_choosen_id()

		#TODO: USE ABSOLUTE PATHS
		self.cmd_lst = [['..'+os.path.sep+self.abs_build_folder+os.path.sep+self.peer_file, self.ip, str(self.start_port)]]

		self.used_ports = [str(self.start_port)]
		self.used_id = [0]
		self.running_peers_lst = [[self.ip, self.start_port]]

		if isinstance(self.idrange,np.ndarray):
			self.idrange = self.idrange.tolist()

		for i in range(1, self.peercount,1):

			choosen_id = self.idrange[i]

			self.get_dst_peer(i)

			self.used_id.append(int(choosen_id))
			#TODO check if delete works
			if isinstance(self.idrange,np.ndarray):
				self.idrange[np.argwhere(self.idrange == choosen_id)[0][0]] = 0
			else:
				self.idrange[np.argwhere(np.array(self.idrange) == choosen_id)[0][0]] =0
				#self.idrange = np.delete(self.idrange, np.argwhere(np.array(self.idrange) == choosen_id)[0][0])

			#TODO: USE ABSOLUTE PATHS

			self.cmd_lst.append(['../'+self.abs_build_folder+os.path.sep+self.peer_file, str(self.ip), str(int(self.portrange[i])), str(int(choosen_id)), str(self.dstpeer[0]), str(self.dstpeer[1])])

			self.running_peers_lst.append([str(self.ip), str(int(self.portrange[i]))])

	def start_peer(self):

		for i in range(0,self.peercount):

			# set name for logger- and filenamesyntax
			filename_stderror = "PEER_ERR_"+str(self.cmd_lst[i][2])+".txt"
			filename_stdout   = "PEER_OUT_"+str(self.cmd_lst[i][2])+".txt"

			# init loggers
			logger_stdout = Logger(filename_stdout).logger
			logger_stderr = Logger(filename_stderror).logger

			logger_stdout.info("Starting job "+str(i)+" with Peer ID"+str(self.cmd_lst[i][2]))
			logger_stderr.info("Starting job "+str(i)+" with Peer ID"+str(self.cmd_lst[i][2]))


			x = threading.Thread(target=cThread, args=(" ".join(self.cmd_lst[i]), str(i), logger_stdout, logger_stderr ))
			print(self.cmd_lst[i])
			x.start()

			time.sleep(2) # some time to make sure the peer is started
			print('started peer : ' + str(i))
class Client:

	def __init__(self,configname = 'client_setup.ini'):

		parser = ConfigParser()
		parser.read(configname)

		self.client_ip = parser.get('CLIENT_SETUP','client_ip')
		self.abs_build_folder = parser.get('CLIENT_SETUP','path_to_buildfolder')
		self.path = os.path.abspath(__file__)[:-len(__file__)]
		self.client_count = int(parser.get('CLIENT_SETUP','client_count'))
		self.client_name = parser.get('CLIENT_SETUP','client_file')
		self.data_folder = parser.get('CLIENT_SETUP', 'data_folder')+os.path.sep
		self.swap_folder = parser.get('CLIENT_SETUP','swap_folder')
		self.peer_pool_file = parser.get('CLIENT_SETUP','running_peer_list')
		self.messaged_pool = parser.get('CLIENT_SETUP','messaged_pool')

		self.peer_pool = []
		self.file_pool_path = self.path+self.swap_folder+os.path.sep+self.messaged_pool

		#TODO: totally unused :D
		self.use_nullbytes_in_keys = True


	def peerinfo_to_data(self,filename):
		if not os.path.exists(self.swap_folder):
			raise ValueError(self.swap_folder + " folder missing. Did you start any peers?")

		with open(self.path+self.swap_folder+os.path.sep+filename,'r') as pool:
			for line in pool.readlines():
				print(line)
				self.peer_pool.append(line.strip().split(','))

	"""
	Save all filepaths in a list that can be used for client-peer transmissions (SET/GET/DEL)
	"""
	def get_keys_and_data_path(self,path,folder):
		file_list = []
		for file in os.listdir(path+folder):
			file_list.append([file,path+folder+file])
		return file_list

	def setter_client_with_random_key(self,port,file_name = None, ip = None):
		#./client localhost 4711 SET /pics/cat.jpg < cat.jpg
		if ip == None:
			ip = self.client_ip

		if file_name == None:
			file_name = random.choice(self.keys_and_paths_pool)[1]

		key_name = []
		for k in range(0,random.randint(1,10)):
			key_name.extend(random.sample(string.ascii_letters, random.randint(0,len(string.ascii_letters)-1)))
		key_name = "".join(key_name).replace("\t","").replace("\n","").replace("\r","").replace(" ","").replace("\x0b","").replace("\x0c","").strip()

		self.keys_and_paths_pool.append([key_name, file_name])

		return ['..'+os.path.sep+self.abs_build_folder+os.path.sep+self.client_name, ip, str(port), "SET", str(key_name), "<", str(file_name)]

	def threaded_get_to_every_peer(self):

		print("STARTING GET-FLOOD IN:")
		for gamma in range(5,0,-1):
			print(str(gamma)+'...')
			time.sleep(1)

		peer_box = self.peer_pool.copy()

		self.cmd_clients = []
		for peers in range(self.client_count):
			if len(peer_box) > 0:
				peer = random.choice(peer_box)
				self.cmd_clients.append(self.setter_client_with_random_key(peer[1]))
				peer_box.remove(peer)
			else:
				peer_box = self.peer_pool.copy()
				peer = random.choice(peer_box)
				self.cmd_clients.append(self.setter_client_with_random_key(peer[1]))
				peer_box.remove(peer)
			#print(len(self.cmd_clients))

		del peer_box

	def init_set_client(self):

		self.client_args_used = []
		self.keys_and_paths_pool = self.get_keys_and_data_path(self.path,self.data_folder)
		self.peerinfo_to_data(self.peer_pool_file)
		self.threaded_get_to_every_peer()
		self.start_set_client()

	def start_set_client(self):

		for i in range(0,self.client_count):

			#print(i,self.client_count)

			# set name for logger- and filenamesyntax
			filename_stderror = "CLIENT_ERR_TO_"+str(self.cmd_clients[i][2])+".txt"
			filename_stdout   = "CLIENT_OUT_TO_"+str(self.cmd_clients[i][2])+".txt"

			# init loggers
			logger_stdout = Logger(filename_stdout).logger
			logger_stderr = Logger(filename_stderror).logger

			#logger_stdout.info("Starting job "+str(i)+" with Peer ID"+str(self.cmd_lst[i][2]))
			#logger_stderr.info("Starting job "+str(i)+" with Peer ID"+str(self.cmd_lst[i][2]))

			x = threading.Thread(target=cThread, args=(" ".join(self.cmd_clients[i]), str(i), logger_stdout, logger_stderr ))
			print(len(self.client_args_used))
			self.client_args_used.append([self.cmd_clients[i][4],self.cmd_clients[i][6]])
			x.start()

		with open(self.file_pool_path,'w') as pool:
			for element in self.client_args_used:
				pool.write(element[0]+","+element[1]+"\n")

		for k,i in enumerate(self.client_args_used):
			print(" ".join(self.cmd_clients[k]))
		#	print(len(self.client_args_used))
			print(k,"|KEY DATAPATH|:",i)

################################################################################################

	def getter_client(self,port,key_name,file_name,ip = None):
		#./client localhost 4711 GET /pics/cat.jpg > cat.jpg
		if ip == None:
			ip = self.client_ip
		return ['..'+os.path.sep+self.abs_build_folder+os.path.sep+self.client_name, ip, str(port), "GET", str(key_name), ">", str(file_name)]

	def get_client_args_used(self):
		self.client_args_used = []
		with open(self.file_pool_path,'r') as pool:
			for k in pool.readlines():
				self.client_args_used.append(k.rstrip().split(","))
		return self.client_args_used

	def init_get_client(self):
		self.cmpfolder = self.path+"logs"+os.path.sep
		self.cmd_clients = []

		self.peerinfo_to_data(self.peer_pool_file)

		peer_box = self.peer_pool.copy()
		for i,entry in enumerate(self.get_client_args_used()):
			if len(peer_box) > 0:
				peer = random.choice(peer_box)
				self.cmd_clients.append(self.getter_client(int(peer[1]),entry[0],self.cmpfolder+entry[1].split(os.path.sep)[-1]))
				peer_box.remove(peer)
			else:
				peer_box = self.peer_pool.copy()
				peer = random.choice(peer_box)
				self.cmd_clients.append(self.getter_client(int(peer[1]),entry[0],self.cmpfolder+entry[1].split(os.path.sep)[-1]))
				peer_box.remove(peer)

		for i in range(0,self.client_count):

			# set name for logger- and filenamesyntax
			filename_stderror = "CLIENT_ERR_TO_"+str(self.cmd_clients[i][2])+".txt"
			filename_stdout   = "CLIENT_OUT_TO_"+str(self.cmd_clients[i][2])+".txt"

			# init loggers
			logger_stdout = Logger(filename_stdout).logger
			logger_stderr = Logger(filename_stderror).logger

			#logger_stdout.info("Starting job "+str(i)+" with Peer ID"+str(self.cmd_lst[i][2]))
			#logger_stderr.info("Starting job "+str(i)+" with Peer ID"+str(self.cmd_lst[i][2]))

			x = threading.Thread(target=cThread, args=(" ".join(self.cmd_clients[i]), str(i), logger_stdout, logger_stderr ))
			x.start()
			self.client_args_used.append([self.cmd_clients[i][4],self.cmd_clients[i][6]])

		#TODO: IMPLEMENT FILE COMPARE HERE - IMPORT ALREADY ON TOP







"""
OLD CODE STARTING HERE:
TODO: EXTRACT DELETE FUNCTION


class Tests(unittest.TestCase):

	def __init__(self):
		super(Tests, self).__init__()
		# read config files
		
	# unused
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

	def del_client(self,port,key_name,ip = None):
		#./client localhost 4711 DELETE /pics/cat.jpg
		if ip == None:
			ip = self.client_ip
		return ['..'+os.path.sep+self.abs_build_folder+os.path.sep+self.client_name, ip, str(port), "DELETE", str(key_name)]



"""


if __name__ == '__main__':
	parser = argparse.ArgumentParser(formatter_class=argparse.RawDescriptionHelpFormatter, description=dedent('''\
        ***************************************************
        RNVS DHT TESTING SCRIPT

        Start peer_setup.ini with -p 
        or 
        the client_setup.ini with -s or -g or -d

        Usage Example:
            peer.py -s 
        Arguments:
            -p  : start peer-script
            -s  : start set client-script (alpha)
            -g  : start get client-script (prealpha)
            # TODO: -d  : start del client-script
        ***************************************************\
        '''))

	parser.add_argument('-p', '--peer', action='store_true')
	parser.add_argument('-s', '--client_set',action='store_true')
	parser.add_argument('-g', '--client_get',action='store_true')
	parser.add_argument('-d', '--client_del',action='store_true')

	#if len(sys.argv) != 1:
	#	print(parser.description)
	#	exit(1)

	args = parser.parse_args()
	print(args)

	if args.peer:
		Peer()
	elif args.client_set:
		c = Client()
		c.init_set_client()
	elif args.client_get:
		c = Client()
		c.init_get_client()
	elif args.client_del:
		# TOTALLY TODO HERE
		#c = Client()\
		#c.delet()
		pass
		print(parser.description)
		exit(1)
	else:
		print(parser.description)
		exit(1)

