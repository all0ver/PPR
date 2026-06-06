#!/usr/bin/env python3
import socket
import hashlib

TCP_IP = '127.0.0.1'
TCP_PORT = 5000

UDP_IP = '127.0.0.1'
UDP_PORT = 6000

# rozmiar pojedynczego kawalka informacji dla udp
CHUNK_SIZE = 1024
# czas oczekiwania na potwierdzenie od otrzymania od perl
TIMEOUT = 0.5

# utworzenie gniazda tcp (ipv4)
tcp_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM) # ipv4, tcp
tcp_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
tcp_sock.bind((TCP_IP, TCP_PORT)) # przypisanie utworzonego gniazda do przypisanego adresu i portu
tcp_sock.listen(1) # włączenie nasłuchiwania, 1 - długość kolejki

# utworzenie gniazda udp (ipv4)
udp_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM) # utworzenie gniazda ivp4 udp
udp_sock.settimeout(TIMEOUT) # czas oczekiwania na potwierdzenie od perl

seq_num = 0 # numer paczki
while True:
	conn, addr = tcp_sock.accept() # akceptacja połączenia i zapisanie nowego gniazda tcp (conn) i jego adresu (ipv4 + port)
	
	# odbiór informacji z tcp (c)
	data = b"" # zapisanie informacji w bajtach - b""
	while True:
		packet = conn.recv(1024)
		if not packet:
			break # c zakonczylo wyslanie danych
		data += packet # dopisanie odebranych danych
	conn.close()

	if not data:
		continue
		
	print(f"\n[+] TCP: Odebrano {len(data)} bajtow Base64. Wysylam UDP...")
	md5_hash = hashlib.md5(data).hexdigest()
	print(md5_hash)
	
	message = f"{seq_num}|".encode('utf-8')+data #dopisanie nagłówka z numerem 
	
	ack_recv = False
	
	while not ack_recv:
		try:
			udp_sock.sendto(message, (UDP_IP, UDP_PORT)) # wysylanie informacji - socket/udp
			
			ack, udp_add = udp_sock.recvfrom(1024)
			if ack.decode('utf-8') == f"ACK|{seq_num}":
				ack_recv = True
				print("Potwierdzono obior - perl")
				seq_num += 1
		except socket.timeout:
			pass
			
	if data == b"EOF":
		print("[Python] EOF\n")
		seq_num = 0
