import socket
import cv2
import numpy as np
from Crypto.Cipher import AES 
from Crypto.Util.Padding import unpad 
import binascii

# Simple demo server implementation which can be used for testing

hostname = socket.gethostname()
localIP = socket.gethostbyname(hostname)

print("Your Computer Name is:" + hostname)
print("Your Computer IP Address is:" + localIP)

localPort   = 20001

bufferSize  = 3000
# Create a datagram socket

UDPServerSocket = socket.socket(family=socket.AF_INET, type=socket.SOCK_DGRAM)

# Bind to address and ip

UDPServerSocket.bind((localIP, localPort))

print("UDP server up and listening")

key = 'YOUR_KEY'.encode('ascii')
iv = 'YOUR_IV'.encode('ascii')
cipher = AES.new(key, AES.MODE_CBC, iv) 

# Listen for incoming datagrams
buffer = np.array([],dtype=np.uint8)
while True:
    # Receive data from the client
    data, addr = UDPServerSocket.recvfrom(bufferSize)

    np_data = np.frombuffer(data, dtype=np.uint8)
    # Gets rid of metadata
    buffer = np.append(buffer, np_data[:-3])
    # Each fragmented image frame has some metadata ID, ORDER and COMPLETE which is not encrypted
    print("ID: %x ORDER: %x COMPLETE: %x" % (np_data[-3], np_data[-2], np_data[-1]))
    if(np_data[-1] == 0):
        continue
    
    try: 
        buffer = np.frombuffer(unpad(cipher.decrypt(bytearray(buffer.tobytes())), 16), dtype=np.uint8)
        frame = cv2.imdecode(buffer, cv2.IMREAD_COLOR)
    except:
        print("Failed to decrypt\n")
    
    if frame is not None:
        #print("Showing")
        cv2.imshow('MJPEG Stream', frame)
    
    if cv2.waitKey(1) & 0xFF == ord('q'):
        break
    buffer = np.array([], dtype=np.uint8)

UDPServerSocket.close()
cv2.destroyAllWindows()