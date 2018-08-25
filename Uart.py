import serial
import time
from threading import *
import atexit


class Uart():
    def __init__(self):
        pass

    def open_uart(self, port, baud):
        try:
            self.ser = serial.Serial(port, baud, timeout=4)
            if self.ser.isOpen():
                print(self.ser.name + ' is open...')
                return(True)
        except:
            print("COM not Ready")
            return(False)

    def start_thread_reading(self, status):
        if status == True:
            ThreadEncoder=Thread(target=self.reading, args=(status,))
            ThreadEncoder.daemon=True
            ThreadEncoder.start()    

    def send_commands(self, data):
        """Sende Data via COM, ACHTUNG: PIC max. 1 byte per serial.write()"""
        for single_command in data:
            self.ser.write(bytes(single_command))
        print("Sent to COM ")

    def reading(self,reading):
        self.reading = reading
        self.thread_stopped = False
        self.data_from_pic = ""
        
        while self.reading == True:
                while self.ser.inWaiting() > 50:
                    self.data_from_pic = self.ser.readline()
                #print("readline " + self.data_from_pic)
                #self.ser.reset_input_buffer()
                time.sleep(0.5)
        self.thread_stopped = True
        return(False)

    def get_new_commands(self):
        data = self.data_from_pic.split(",")
        output = []

        if data[0] == "\r255":
            for single_command in data:
                output.append(single_command)
        return(output)

    def close(self):
        self.reading = False
        while self.thread_stopped == False:
            pass
        
        try:
            self.ser.close()
            print("Close COM")
        except:
            print("Could not close COM")
     
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

