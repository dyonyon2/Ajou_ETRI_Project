import socket 

bufferSize  = 128
UDPServerSocket = socket.socket(family=socket.AF_INET, type=socket.SOCK_DGRAM) 
UDPServerSocket.bind(("192.168.20.12",8888)) 

while(True): 
	bytesAddressPair = UDPServerSocket.recvfrom(bufferSize) 
	message = bytesAddressPair[0] 
	address = bytesAddressPair[1] 
	clientMsg = "Message from Client:{}".format(message) 
	clientIP = "Client IP Address:{}".format(address) 
	print(clientMsg) 
	print(clientIP) 

