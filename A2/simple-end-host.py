import socket
import sys
import multiprocessing as mp

port_num = 0

def clean_exit(rc, sock, message):
	if(rc == -1 or sock == None):
		if sock != None:
			sock.close()
		print(message)
		exit(1)

def initialize():
	"""
	Broadcast end system's existence
	Send Broadcast to IP 255.255.255.255
	with TTL = 0
	"""
	return


def send(dest, msg, ttl=0):
	"""
	Send msg to ip dest with a max # of hops of ttl
	
	"""
	return

def receive():
	"""
	Look for incoming messages and print them 
	"""
	host_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	clean_exit(0, host_socket, "[Socket Creation Error]")

	router_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	clean_exit(0, router_socket, "[Router Socket Creation Error]")
	print("Im waiting\n")
	#while(1):
		




def simple_end_system():
	#requires a port number and IP to connect
	if len(sys.argv) < 2:
		print("[Usage]: simple-end-system.py PORT_NUM")
		exit(1)
	port_num = sys.argv[1]
	print(port_num)
	return
	



if __name__ == '__main__':
	#mp.set_start_method('spawn')
	simple_end_system()
	initialize()
	p = mp.Process(target=receive)
	p.start()
	print("Booting up host system\n")
	

