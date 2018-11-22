import serial
import time
from threading import *
import atexit
import sys
import logging

class Uart():

    EVT_CONNECTION_STATUS = 'EVT_CONNECTION_STATUS'
    EVT_DATA = 'EVT_DATA'
    EVT_CONNECTION_INTERRUPTED = 'EVT_CONNECTION_INTERRUPTED'
    
    def __init__(self):
        self.connected = False
        self.observers = {}
        self.readingThreadRunning = False

    def attach(self, evt, observer):
        if not evt in self.observers:
            self.observers[evt] = set()
        self.observers[evt].add(observer)

    def detach(self, evt, observer):
        if not evt in self.observers:
            return
        self.observers[evt].discard(observer)

    def _notifyObservers(self, evt, data):
        if not evt in self.observers:
            return
        for observer in self.observers[evt]:
            observer(data)

    def setConnectionStatus(self, status):
        self.connected = status
        self._notifyObservers(Uart.EVT_CONNECTION_STATUS, status)

    def open_uart(self, port, baud):
        try:
            self.ser = serial.Serial(port, baud, timeout=4)
            if self.ser.isOpen():
                self.setConnectionStatus(True)
                print(self.ser.name + ' is open...')
                ThreadEncoder=Thread(target=self.reading)
                ThreadEncoder.daemon=True
                ThreadEncoder.start()   
        except:
            self.setConnectionStatus(False)
            print("COM not Ready")

    def send_commands(self, data):
        if not self.connected:
            return
        """Sende Data via COM, ACHTUNG: PIC max. 1 byte per serial.write()"""
        for single_command in data:
            self.ser.write(bytes(single_command))
        print("Sent to COM ")

    def reading(self):
        self.readingThreadRunning = True
        self.stopReading = False
        data_from_pic = ""
        noDataReceivedCounter = 0
        
        while not self.stopReading and self.connected:
            try:
                while self.ser.inWaiting() > 50:
                    data_from_pic = self.ser.readline()
                data = data_from_pic.split(",")
                data_from_pic = ""
                if data[0] == "\r255":
                    noDataReceivedCounter = 0
                    self._notifyObservers(Uart.EVT_DATA, data[:])
                else:
                    noDataReceivedCounter += 1
                if noDataReceivedCounter > 10:
                    raise Exception("No data received for at least 5 seconds; Aborting connection")
            except:
                self.setConnectionStatus(False)
                self.closeSerInternal()
                self._notifyObservers(Uart.EVT_CONNECTION_INTERRUPTED, None)
                logging.exception("Connection to Reddy interrupted")
            time.sleep(0.5)
        self.readingThreadRunning = False

    def close(self):
        self.stopReading = True
        while self.readingThreadRunning:
            pass

        self.closeSerInternal()
        self.setConnectionStatus(False)

    def closeSerInternal(self):
        try:
            self.ser.close()
            logging.info("Close COM")
        except:
            logging.exception("Could not close COM")

     
if __name__ == "__main__":
   
    uart = Uart()
    uart_status = uart.open_uart("COM4",9600)
    atexit.register(uart.close)

    ThreadEncoder=Thread(target=uart.reading, args=(uart_status,))
    ThreadEncoder.daemon=True
    ThreadEncoder.start()

    commands = [255,23,12,111,222,11,14]

    for dat in commands:
        uart.send_commands(chr(int(dat)))

    for i in range(200):
        time.sleep(1.5)
        data = uart.get_new_commands()
        print("data: "+str(data))

        if data:
            print('{0:08b}'.format(int(data[7])))

    uart.close_uart()

