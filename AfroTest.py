#Talking to AFRO - the Arduino FRObbler
#
#v 0.1

import serial
import logging
import unittest
import time

class AfroUnit:
	port=None
	id=-1
	def __init__(self,port,id):
		self.port=port
		self.id=id
		
	def _send(self,string):
		#print string
		print ">", string
		self.port.write(string)

	def receive(self):
		print ">>", self.port.inWaiting() 
		r=self.port.read(self.port.inWaiting())
		return r
		
	def send(self,message,id="",protocol=0):
		if not(id):
			id=self.id
		if (protocol==0):
			self._send("AF"+id+message+"FA")
		else:
			logging.error("protocol not implemented")
	
	def digitalRead(self,pin):
		self.send("R"+chr(pin+65)+"xx")
		return self.result()
		
	def digitalWrite(self,pin,value):
		if (value):
			self.send("W"+chr(pin+65)+"xx")
		else:
			self.send("W"+chr(pin+65)+"\0\0")
	
	def analogRead(self,pin):
		self.send("A"+chr(pin+65)+"xx")
		return self.result()
	
	def analogWrite(self,pin,value):
		self.send("P"+chr(pin+65)+"\0"+chr(value))
		

	def result(self):
		r=self.receive()
		if (r[:2]=="SR" and r[-2:]=="RS"):
			payload=r[2:6]
			return (payload[0],payload[1],ord(payload[2])*256+ord(payload[3]))



class TestBasicAfro(unittest.TestCase):
	digitalPinsBottom=2
	digitalPinsTop=13
	analogPinsBottom=0
	analogPinsTop=7
	portName="COM62"
	baudRate=57600
	#pins that support PWM on your Arduino
	analogWritable=[2,3,5,6,10,11] 
	def setUp(self):
		self.port=serial.Serial(port=self.portName,baudrate=self.baudRate)
		self.afro1=AfroUnit(self.port,"O")
	def shortWait(self):
		time.sleep(0.1)
	def mediumWait(self):
		time.sleep(0.25)
	def tearDown(self):
		self.port.close()

	def testAllAnalogWrite(self):
		for power in range(0,255,10):
			for pin in self.analogWritable:
				self.afro1.analogWrite(pin,power)
				self.shortWait()

	def estAllDigitalRead(self):
		for i in range(self.digitalPinsBottom,self.digitalPinsTop+1):
			x=self.afro1.digitalRead(i)
			self.shortWait()
			print i,x
			
		for i in range(self.digitalPinsBottom,self.digitalPinsTop+1):
			x=self.afro1.digitalRead(i)
			self.shortWait()
			print i,x
			
	def estAllFlash(self):
		for i in range(0,6):
			for i in range(self.digitalPinsBottom,self.digitalPinsTop+1):
				self.afro1.digitalWrite(i,1)
			self.mediumWait()
			for i in range(self.digitalPinsBottom,self.digitalPinsTop+1):
				self.afro1.digitalWrite(i,0)
			self.mediumWait()
	
	def estAllOff(self):
		for i in range(self.digitalPinsBottom,self.digitalPinsTop+1):
			self.afro1.digitalWrite(i,0)

	def estAllOn(self):
		for i in range(self.digitalPinsBottom,self.digitalPinsTop+1):
			self.afro1.digitalWrite(i,1)



if __name__ == '__main__':
	unittest.main()