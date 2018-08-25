import serial
import time
from threading import *
import atexit


class Uart():

    EVT_CONNECTION_STATUS = 'EVT_CONNECTION_STATUS'
    EVT_DATA = 'EVT_DATA'
    
    def __init__(self):
        self.connected = False
        self.observers = {}

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
        self.data_from_pic = ""
        
        while not self.stopReading and self.connected:
            try:
                while self.ser.inWaiting() > 50:
                    self.data_from_pic = self.ser.readline()
                data = self.data_from_pic.split(",")
                if data[0] == "\r255":
                    print("Notify observers for EVT_DATA: " + str(data))
                    self._notifyObservers(Uart.EVT_DATA, data[:])
            except:
                self.setConnectionStatus(False)
            time.sleep(0.5)
        self.readingThreadRunning = False

    def close(self):
        self.stopReading = True
        while self.readingThreadRunning:
            pass
        
        try:
            self.ser.close()
            self.setConnectionStatus(False)
            print("Close COM")
        except Exception as e:
            print("Could not close COM" + str(e))
     
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

