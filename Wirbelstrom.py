#! python
# -*- coding: utf-8 -*-

#ERRORS:
# Ohne RS232 converter, close wirdn nicht beendet, thread still alive
# Chatch Errors funktiniert nicht fuer Thread-Serial

from Tkinter import *
import tkFileDialog as filedialog
import tkMessageBox
from Uart import *
from threading import *
import atexit
import time
import logging
import pickle
import sys

data = ""

class Gui():
    def __init__(self, root):
        self.root = root
        self.root.title ("Wirbelstrom-Control")           #Titel de Fensters
        self.root.geometry("1100x700+0+0")

        self.menu =     Menu(root)
        self.filemenu = Menu(self.menu)
        self.filemenu.add_command(label="Sichern", command = self.store_setting)
        self.filemenu.add_command(label="Laden", command = self.load_setting)
        self.menu.add_cascade(label="Datei",menu = self.filemenu)
        self.root.config(menu=self.menu)

        self.button_connect =        Button(root, text="Connect",    width = 16, command=self.connect)
        self.button_disconnect =     Button(root, text="Disconnect", width = 16, command=self.disconnect, state=DISABLED)
        
        self.entry_t_time =          Entry(root)
        self.entry_t_lever =         Entry(root)
        self.entry_t_lever_delay =   Entry(root)
        self.entry_t_count =         Entry(root)
        self.entry_part =            Entry(root)
        self.entry_part_bit =        Entry(root)

        self.entry_t_time.insert(0,"1")
        self.entry_t_lever.insert(0,"1")
        self.entry_t_lever_delay.insert(0,"1")
        self.entry_t_lever_delay.config(state='disabled')
        self.entry_t_count.insert(0,"1")
        self.entry_part.insert(0,"1")
        self.entry_part_bit.insert(0,"1")

        self.label_t_plus_time =     Label(root, text="B_plus_time")
        self.label_lever_long_time = Label(root, text="Lever_ON_time")
        self.label_t_lever_delay =   Label(root, text="Lever_Delay_time")
        self.label_t_count =         Label(root, text="count")
        self.label_part =            Label(root, text="Parts")
        self.label_eject =            Label(root, text="Safe Eject")
        self.eject_state = IntVar()
        self.radiobutton_eject =     Checkbutton(root, text = "ON", variable = self.eject_state)

        self.label_com =             Label(root, text="SCHNITTSTELLEN Status")
        self.label_dev_ok =          Label(root, text="ELOTEST Bereit")  
        self.label_dev_ready =       Label(root, text="ELOTEST Fehlerfrei")
        self.label_config_ok =       Label(root, text="CONFIG erhalten")
        self.label_picfault_ok =     Label(root, text="CONTROLLER Fehlerfrei")

        self.label_com_status =          Label(root, text="False", relief = GROOVE, fg = "red", width = 6)
        self.label_dev_status =          Label(root, text="False", relief = GROOVE, fg = "red", width = 6)
        self.label_dev_fault_status =    Label(root, text="False", relief = GROOVE, fg = "red", width = 6)
        self.label_config_status =       Label(root, text="False", relief = GROOVE, fg = "red", width = 6)
        self.label_picfault_status =     Label(root, text="False", relief = GROOVE, fg = "red", width = 6)

        self.label_shift =           Label(root, text="Lever-FIFO")
        self.label_count =           Label(root, text="Counts")

        self.button_send =           Button(root, text="Send Config", fg="blue",command=lambda: self.get_gui_command(0,0), width = 38)
        self.label_counts =          Label(root, text="00", font=("Calibri",20), relief = GROOVE, width = 10)
        self.label_input =           Label(root, text="Keine Daten bis jetzt", relief = GROOVE, width = 38)
        self.label_input_shift =     Label(root, text="-000000-", font=("Calibri",20), relief = GROOVE, width = 10)

        self.button_connect.place        (x= 100, y=50)
        self.button_disconnect.place     (x= 250, y=50)

        self.label_t_plus_time.place     (x= 100, y = 100)
        self.label_lever_long_time.place (x= 100, y = 150)
        self.label_t_lever_delay.place   (x= 100, y = 200)
        self.label_t_count.place         (x= 100, y = 250)
        self.label_part.place            (x= 100, y = 300)
        self.label_eject.place           (x= 100, y = 350)

        self.label_com.place            (x= 650, y = 100)  
        self.label_dev_ok.place         (x= 650, y = 150)    
        self.label_dev_ready.place      (x= 650, y = 200)  
        self.label_config_ok.place      (x= 650, y = 250)
        self.label_picfault_ok.place    (x= 650, y = 300)
        
        self.label_com_status.place         (x= 850, y = 100)
        self.label_dev_status.place         (x= 850, y = 150)
        self.label_dev_fault_status.place   (x= 850, y = 200)
        self.label_config_status.place      (x= 850, y = 250)
        self.label_picfault_status.place    (x= 850, y = 300)
        
        self.entry_t_time.place         (x= 250, y = 100)
        self.entry_t_lever.place        (x= 250, y = 150)
        self.entry_t_lever_delay.place  (x= 250, y = 200)
        self.entry_t_count.place        (x= 250, y = 250)
        self.entry_part.place           (x= 250, y = 300)

        self.radiobutton_eject.place    (x= 250, y = 350)

        self.button_send.place          (x = 100, y = 400)
        
        self.label_count.place          (x = 650, y = 400)
        self.label_counts.place         (x = 850, y = 400)
        self.label_shift.place          (x = 650, y = 450)
        self.label_input_shift.place    (x = 850, y = 450)
        self.label_input.place          (x = 100, y = 450)

        self.old_counts = 0
        self.show_counts = 0
        
        self.uart = Uart()
        self.uart.attach(Uart.EVT_CONNECTION_STATUS, self.connection_status_changed)
        self.uart.attach(Uart.EVT_DATA, self.data_received)
        
    def cleanup(self):
        """Close UART and Close GUI"""
        print("Clean-Up")
        self.uart.close()
        self.root.destroy()

    def store_setting(self):
        """Get Str vom Entry's and Pickle to file"""
        self.stored_file = filedialog.asksaveasfile(filetypes = (("txt files","*.txt"),("all files","*.*")),                                                    defaultextension = ".txt")
        if self.stored_file:
            print("Write data...")       
            data = [self.entry_t_time.get(),
                    self.entry_t_lever.get(),
                    self.entry_t_lever_delay.get(),
                    self.entry_t_count.get(),        
                    self.entry_part.get(),
                    self.entry_part_bit.get()]
            
            pickle.dump(data, self.stored_file)
            self.stored_file.close()

    def load_setting(self):
        """Get data from file and update Entry's"""
        self.loaded_file = filedialog.askopenfile(filetypes = (("txt files","*.txt"),("all files","*.*")))
        if self.loaded_file:
            print("Loading...")
            try:                
                data = pickle.load(self.loaded_file)

                self.entry_t_time.delete(0, END)
                self.entry_t_time.insert(0, data[0])
                self.entry_t_lever.delete(0, END)
                self.entry_t_lever.insert(0,data[1])
                self.entry_t_lever_delay.delete(0, END)
                self.entry_t_lever_delay.insert(0, data[2])
                self.entry_t_count.delete(0, END)
                self.entry_t_count.insert(0, data[3])
                self.entry_part.delete(0, END)
                self.entry_part.insert(0, data[4])
                self.entry_part_bit.delete(0, END)
                self.entry_part_bit.insert(0, data[5])
            except:
                print("Fehler beim Laden der Daten")
            

    def connect(self):
        """Connect to COM Port"""
        self.uart.open_uart("COM4",9600)

    def disconnect(self):
        self.uart.close()

    def connection_status_changed(self, status):
        """Set Connect/Disconect Button State"""
        if status:
            self.button_connect.configure(state = DISABLED)
            self.button_disconnect.configure(state = NORMAL)
            self.label_com_status.configure(text = str(status), fg = "darkgreen")
        else:
            self.button_connect.configure(state = NORMAL)
            self.button_disconnect.configure(state = DISABLED)
            self.label_com_status.configure(text = str(status), fg = "red")

    def data_received(self, data):
        
        print("Data-In: "+ str(data))
        if len(data) > 8:            
            self.label_input.configure(text = data[1:-1])                               #Statustext Serial input

            if int(data[8]) & 0b00000001:                                                    #Status Dev_ready
                self.label_dev_status.configure( text = "False", fg= "red")
            else:
                self.label_dev_status.configure( text = "True", fg= "darkgreen")
                
            if int(data[8])  & 0b00000010:                                                    #Status Dev_fault
                self.label_dev_fault_status.configure( text = "False", fg= "red")
            else:
                self.label_dev_fault_status.configure( text = "True", fg= "darkgreen")

            if int(data[8])  & 0b00000100:                                                    #Status Config_done
                self.label_config_status.configure( text = "False", fg= "red")
            else:
                self.label_config_status.configure( text = "True", fg= "darkgreen")

            if int(data[8])  & 0b00011000:                                                    #Status uP fail
                self.label_picfault_status.configure( text = "False", fg= "red")
            else:
                self.label_picfault_status.configure( text = "True", fg= "darkgreen")

            #data[9] is for save_eject

            actual_counts = int(data[4])                                                #Prevent Count from Overflow            
            if actual_counts < self.old_counts:                                         #Update Count label
                self.show_counts = self.show_counts + 256 + actual_counts
                self.old_counts = actual_counts
                self.label_counts.configure(text = str(self.show_counts))
            else:              
                self.old_counts = actual_counts
                self.label_counts.configure(text = str(self.show_counts + actual_counts))

            fifo_text = '{0:08b}'.format(int(data[7]))                                  #Update FIFO (reversed L-R)
            fifo_reversed = ''.join(reversed(fifo_text))
            self.label_input_shift.configure(text = fifo_reversed)

    def get_part(self, text):
        """Read Str from Parts-Entry and generate Byte+Bit for uP / Part maximal 100"""
        if int(text) > 100:
            tkMessageBox.showerror(title="Error",message="Parts nur 1-100",parent=self.root)           
            return(str(254), str(254))
        else:
            part_byte = int(text) / 8
            part_bit  = int(text) % 8           
            return(str(part_byte), str(part_bit))
        
        
    def get_gui_command(self,x,y):
        """Read Str vom Entry's and send data to COM"""
        
        data = ""
        data = self.text_to_char(255)               #Set commands[arr] in PIC to zero
        
        text = self.entry_t_time.get()
        text = self.check_user_input(text)
        data = data + self.text_to_char(text)

        text = self.entry_t_lever.get()
        text = self.check_user_input(text)
        data = data + self.text_to_char(text)

        text = self.entry_t_lever_delay.get()
        text = self.check_user_input(text)
        data = data + self.text_to_char(text)

        text = self.entry_t_count.get()
        text = self.check_user_input(text)
        data = data + self.text_to_char(text)
        
        text = self.entry_part.get()                #Part anzahl in byte und bit
        text = self.check_user_input(text)
        part_byte, part_bit = self.get_part(text)  
        data = data + self.text_to_char(part_byte)
        data = data + self.text_to_char(part_bit)

        data = data + self.text_to_char("0")        #Send error_status Null -> all OK
       
        status_eject = str(self.eject_state.get())       #Send Eject state 1->ON 0->OFF
        data = data + self.text_to_char(status_eject)        

        if chr(254) not in data:                    #Send when all inputs "okay" 
            print("Data-Out: " +str(data))          #Send data to COM
            self.uart.send_commands(data)
            self.show_counts = 0                    #Counter to Zero
            self.old_counts = 0                     #Old Counter to Zero
        else:
            print("Fehler in User_Inputs")
            
    def text_to_char(self, data):
        """Convert a str[Number] to char"""
        char_to_send = chr(int(data))
        return(char_to_send)

    def check_user_input(self, text):
        "Checks text for range = 1-254 shows MessageBox if Not"""
        if text.isdigit() == False:
            text = "254"
            tkMessageBox.showerror(title="Error",message="Keine gültige Zahl",parent=self.root)
        if  int(text) < 0 or int(text) > 254:
            text = "254"
            tkMessageBox.showerror(title="Error",message="Zahl nur 1 - 254 ",parent=self.root)
        return (text)

if __name__ == "__main__":

    root=Tk()
    gui = Gui(root)
    atexit.register(gui.cleanup)
    root.mainloop()

####    logging.basicConfig(filename='Err_Log_wirbelstrom.log',
####                        format='%(asctime)s %(levelname)-8s %(message)s',
####                        level=logging.ERROR,
####                        datefmt='%Y-%m-%d %H:%M:%S')
##
##    try:    
##        root=Tk()
##        gui = Gui(root)
##        atexit.register(gui.cleanup)    
##        root.protocol("WM_DELETE_WINDOW", gui.cleanup)            
##        root.mainloop()
##    except:
##        logging.exception("Main Failed")
##        print("Fatal Error, Closing App")


