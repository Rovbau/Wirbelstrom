/*
 * File:   Wirbel18F2520.c
 * Author: Michael
 * Programm: Wirbelstromauswetung mit FIFO
 * 
 * Inputs:
 * Erwartet config an UART char(255)char(t_plus_time)char(lever1_long_time)....
 * Wertet die Signale von Sonde InSA und InSB aus. liest den Fehlerzustand InDEFx der übergeordneteten Steuerung ELOSTEST PL600
 * Verländert das Signal von SondeB um t_plus_time[ms*10]
 * Schift FIFO shift_data left wenn count an InSB detektiert
 * Schreibt "1" in FIFO -> wenn Output == ( SondeA == 1 and NICHT SondeB)
 * Funktion Save_eject, aktiv wenn flag_safe_eject == 1: Setzt bit1+bit3 in FIFO shift_data[0] wenn Output == 1 zum sicherern Auswerfen der Teile Vorher/Nacher
 * 
 * Outputs:
 * Setzt OutLeverR = Auswerfer wenn in FIFO an Position welche mit part und part_bit festgelegt wurde gesetzt ist. Position entspricht shift_data[part[part_bit]]
 * Setzt OutLeverR für die zeit von lever1_long_time[ms*10]
 * Setzt Hupe, Alarm LED, Relais OutSteuerungStop wenn Elotest nicht bereit, wenn UART error erkannt, wenn Timer1 overflow, wenn config noch nicht erhalten
 * Setzt LED für SondeA, SondeB, NOK 
 * 
 * Looptime:
 * Timer1 generiert eine looptime von ca. 9ms
 * 
 * Created on 10. Sep 2018
 * Modified on 20. Nov 2018 
 * 
 *  */


#define InDEV_F PORTAbits.RA2   //Elotest status Fehler
#define InDEV_R PORTAbits.RA3   //Elotest status Bereit
#define InSB    PORTAbits.RA4   //Elotest SondeA Kanal-1 alarm
#define InSA    PORTAbits.RA5   //Elotest SondeB Kanal-2 alarm

#define OutSteuerS  LATBbits.LATB0      //Relais Steuerung Halt/Störung
#define OutLeverR   LATBbits.LATB1      //Auswerfer 
#define OutHupe     LATBbits.LATB2      //Hupe Summer
#define OutAlarm    LATBbits.LATB3      //LED Rot Alarm
#define OutLeverLed LATBbits.LATB4      //LED Grün Auswerfer ON
#define OutNOK      LATBbits.LATB5      //LED Rot fehlerhaftes Bauteil
#define OutSB       LATBbits.LATB6      //LED grün SondeB status 
#define OutSA       LATBbits.LATB7      //LED grün SondeA status


#define InHupe      PORTCbits.RC0       //Schalter Hupe ON/OFF
#define InLeverTest PORTCbits.RC1       //Taste Auswerfer ON

// CONFIG1H
#pragma config OSC = INTIO7      // Oscillator Selection bits (INTOSC 1Mhz Standart)
#pragma config FCMEN = OFF      // Fail-Safe Clock Monitor Enable bit (Fail-Safe Clock Monitor disabled)
#pragma config IESO = OFF       // Internal/External Oscillator Switchover bit (Oscillator Switchover mode disabled)

// CONFIG2L
#pragma config PWRT = OFF       // Power-up Timer Enable bit (PWRT disabled)
#pragma config BOREN = OFF      // Brown-out Reset Enable bits (Brown-out Reset disabled in hardware and software)
#pragma config BORV = 3         // Brown Out Reset Voltage bits (Minimum setting)

// CONFIG2H
#pragma config WDT = OFF        // Watchdog Timer Enable bit (WDT disabled (control is placed on the SWDTEN bit))
#pragma config WDTPS = 32768    // Watchdog Timer Postscale Select bits (1:32768)

// CONFIG3H
#pragma config CCP2MX = PORTC   // CCP2 MUX bit (CCP2 input/output is multiplexed with RC1)
#pragma config PBADEN = OFF     // PORTB A/D Enable bit (PORTB<4:0> pins are configured as digital I/O on Reset)
#pragma config LPT1OSC = OFF    // Low-Power Timer1 Oscillator Enable bit (Timer1 configured for higher power operation)
#pragma config MCLRE = ON       // MCLR Pin Enable bit (MCLR pin enabled; RE3 input pin disabled)

// CONFIG4L
#pragma config STVREN = ON      // Stack Full/Underflow Reset Enable bit (Stack full/underflow will cause Reset)
#pragma config LVP = OFF        // Single-Supply ICSP Enable bit (Single-Supply ICSP disabled)
#pragma config XINST = OFF      // Extended Instruction Set Enable bit (Instruction set extension and Indexed Addressing mode disabled (Legacy mode))

// CONFIG5L
#pragma config CP0 = OFF        // Code Protection bit (Block 0 (000800-001FFFh) not code-protected)
#pragma config CP1 = OFF        // Code Protection bit (Block 1 (002000-003FFFh) not code-protected)
#pragma config CP2 = OFF        // Code Protection bit (Block 2 (004000-005FFFh) not code-protected)
#pragma config CP3 = OFF        // Code Protection bit (Block 3 (006000-007FFFh) not code-protected)

// CONFIG5H
#pragma config CPB = OFF        // Boot Block Code Protection bit (Boot block (000000-0007FFh) not code-protected)
#pragma config CPD = OFF        // Data EEPROM Code Protection bit (Data EEPROM not code-protected)

// CONFIG6L
#pragma config WRT0 = OFF       // Write Protection bit (Block 0 (000800-001FFFh) not write-protected)
#pragma config WRT1 = OFF       // Write Protection bit (Block 1 (002000-003FFFh) not write-protected)
#pragma config WRT2 = OFF       // Write Protection bit (Block 2 (004000-005FFFh) not write-protected)
#pragma config WRT3 = OFF       // Write Protection bit (Block 3 (006000-007FFFh) not write-protected)

// CONFIG6H
#pragma config WRTC = OFF       // Configuration Register Write Protection bit (Configuration registers (300000-3000FFh) not write-protected)
#pragma config WRTB = OFF       // Boot Block Write Protection bit (Boot block (000000-0007FFh) not write-protected)
#pragma config WRTD = OFF       // Data EEPROM Write Protection bit (Data EEPROM not write-protected)

// CONFIG7L
#pragma config EBTR0 = OFF      // Table Read Protection bit (Block 0 (000800-001FFFh) not protected from table reads executed in other blocks)
#pragma config EBTR1 = OFF      // Table Read Protection bit (Block 1 (002000-003FFFh) not protected from table reads executed in other blocks)
#pragma config EBTR2 = OFF      // Table Read Protection bit (Block 2 (004000-005FFFh) not protected from table reads executed in other blocks)
#pragma config EBTR3 = OFF      // Table Read Protection bit (Block 3 (006000-007FFFh) not protected from table reads executed in other blocks)

// CONFIG7H
#pragma config EBTRB = OFF      // Boot Block Table Read Protection bit (Boot block (000000-0007FFh) not protected from table reads executed in other blocks)


#include <xc.h>
#include <stdlib.h>
#include <pic18f2520.h>
#define _XTAL_FREQ 4000000


char SondeA, SondeB;
char SondeB_long;
char output;
char out_long, count, count_old, count_old_lever;
char new_data;
char new_data1;
char allready_set;
char out_allready;
char shift_data[13] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
char part;
char part_bit;
char aktual;
char lever_on, lever_long_flag, lever_relais_flag;
char position;
char commands[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
char t_plus_time;
char lever1_long_time;
char lever_delay_time;
char time_NOK;
char lever_allready_set;
char dumy1, dumy3;
char new_data_ready;
char Txdata[6];
char daten;
char send_actual_byte;
char error_status;
char hold_bit_for_safe, flag_safe_eject, hold_allready_set, relase_allready_set;


void init(void) {
    nRBPU = 1;              //Disable (1) PORTB internal pull up resistors
    OSCCON = 0b01100000;    // Set to 4Mhz intern Clock
    ADCON1 = 0b00001111;    // A/D off
    TRISA =  0b11111111;    //PORTA as out/in
    TRISB =  0b00000000;    //PORTB as out/In
    TRISC =  0b11111111;    //PORTC
    PORTA =  0x00;          //All OFF
    PORTB =  0x00;
    PORTC =  0x00;
    
}

void init_comparator(void) {
    CMCON = 0b00000111; // Comparator OFF
    //CMCON = 0b00000100; //Zwei OP 
    //TRISA = 0b00001111; //Pin as Input
}

void init_timer1(void){
T1CON = 0b000001;
}

void wait_timer1(void) {
    if (PIR1bits.TMR1IF == 1) {
        error_status = error_status | 0b00010000; // Set Error_bit 4 timer_overflow
    } else {
        error_status = error_status & ~0b00010000; // Clear Error_bit 4 timer_overflow
    }
            
    while (PIR1bits.TMR1IF == 0) {
    }
    TMR1H = 220;                                //Setze Timer1 auf 9ms @ 4Mhz
    TMR1L = 0;
    TMR1IF = 0;
}

void init_serial(void) {
    SPBRG = 25; //9200 baud @4MHz
    TXSTA = 0b00100100; //  TX, 8bit,FRame e OverRun errors, high_speed
    RCSTA = 0b10010110; // USART,8bit,FRame e OverRun errors
    RCSTAbits.CREN = 1; // Receiver ON
    TRISCbits.TRISC6 = 1; // TX und SX auf Input (beide INPUT!)
    TRISCbits.TRISC7 = 1;
    PIE1bits.RCIE = 1; // USART Interupt EIN
    PIR1 = 0;
    INTCONbits.PEIE = 1;
    INTCONbits.GIE = 1; //Global interrupt ein
}

void send_serial(char databyte) {
    while (!PIR1bits.TXIF); // Warten bis Sendepuffer leer
    TXREG = databyte; // transmit byte
}

void int_to_str(char daten) {
    //Sende 3xDigit und "Komma" 
    Txdata[0] = 0;
    Txdata[1] = 0;
    Txdata[2] = 0;
    Txdata[3] = 0;
    Txdata[4] = 0;
    Txdata[5] = 0;

    itoa(Txdata, daten, 10); //daten to 3xDigits => Txdata

    if (Txdata[0] > 47) {
        send_serial(Txdata[0]); //Send only numbers 0-9
    }
    
    if (Txdata[1] > 47) {
        send_serial(Txdata[1]);
    }
    
    if (Txdata[2] > 47) {
        send_serial(Txdata[2]);
    }
    
    send_serial(44); //Send Komma
}

void __interrupt () my_isr_routine (void)  // In t e r r u p t r o u t i n e
{
    if (PIR1bits.RCIF == 1) { //Interupt von USART
        new_data = RCREG; // Daten  lesen

        if (new_data == 255) {
            position = 0; //Reset DataCounter to "0" 
        } 

        commands[position] = new_data; //new SerialByte to commands array
        position++;

        if (position == 9) {
            position = 0;
            new_data_ready = 1; //Read anzahl bytes then set new_data_ready flag
        }

        if (FERR) {
            error_status = error_status | 0b00001000; // Set error_bit 3
        }

        if (OERR) {
            error_status = error_status | 0b00001000; // Set error_bit 3
            RCSTAbits.CREN = 0; // CREN On -> OFF -> ON Clears Errors USART
            RCSTAbits.CREN = 1;
        }

        PIR1bits.RCIF = 0; //Interupt Flag löschen

    }
}

void send_command(void) {
    //Send data only one byte per function call
    send_actual_byte = send_actual_byte + 1;

    switch (send_actual_byte) {
        case 1:
            int_to_str(255); // Send Startbyte
            break;
        case 2:
            int_to_str(t_plus_time);
            break;
        case 3:
            int_to_str(lever1_long_time);
            break;
        case 4:
            int_to_str(lever_delay_time);
            break;
        case 5:
            int_to_str(count);
            break;
        case 6:
            int_to_str(part);
            break;
        case 7:
            int_to_str(part_bit);
            break;
        case 8:
            int_to_str(shift_data[0]); //(shift_data[0]);
            break;
        case 9:
            int_to_str(error_status);
            break;
        case 10:
            int_to_str(flag_safe_eject);
            break;
        case 11:
            send_serial(10); //Send NEWLINE
            send_serial(13);
            break;
        case 12:
            send_actual_byte = 0;
            break;
        default:
            send_actual_byte = 0;
         
    }
}


void read_command(void) {
    //Read data from commands and update variables and error_status[2]
    if (new_data_ready == 1) {
        dumy1 = commands[0];
        t_plus_time = commands[1];
        lever1_long_time = commands[2]; //Scommands[2];
        lever_delay_time = commands[3];
        count = commands[4];
        part = commands[5];
        part_bit = commands[6];
        dumy3 = commands[7];
        flag_safe_eject = commands[8];
        new_data_ready = 0;
        error_status = error_status & ~0b00000100; // Clear Error_bit 2 >> Config_stored 
 
    }

}

void t_plus(void) {
    //Signal on SondeB longer for time t_plus_time
    if (SondeB == 1) {
        out_long = t_plus_time;
        OutSB = 1;
        SondeB_long = 1;
    } else {
        if (out_long > 1) {
            out_long = out_long - 1;
           OutSB = 1;
            SondeB_long = 1;
        } else {
            OutSB = 0;
            out_long = 1;
            SondeB_long = 0;
        }
    }
}

void counter(void) {
    //Counter if SondeB from zero to one
    if (SondeB == 1) {
        if (allready_set == 0) {
            count = count + 1;
            allready_set = 1;
            hold_allready_set = 0;
            relase_allready_set = 0;
        }
    } else {
        allready_set = 0;
    }

}

void shift_register(void) {
    //For any count pulse / Shift data in Schift_data left one bit
    char i;
    if (count != count_old) {
        for (i = sizeof (shift_data); i-- > 0;) { //Shift left 1bit for sizeof-array "shift_data"
            shift_data[i] <<= 1;
            if (shift_data[i - 1] & 0x80)
                shift_data[i] |= 1;
        }
        count_old = count;
    }
}


void fill_shift_register(char output) {
    //If false Part detected set FIFO schift_data[0,0]
    if (output != 0) {
        if (out_allready == 0) {
            shift_data[0] = shift_data[0] | (1 << 0); //Set bit Null
            out_allready = 1;
        }
    } else {
        out_allready = 0;
    }
}

void save_eject(void) {
    //Save_eject sets bit1+bit3 in FIFO for a more secure detection of false Parts
    if (flag_safe_eject == 1) {
        
        if (shift_data[0] & 0x01) {
            if (hold_allready_set == 0){
            hold_bit_for_safe = hold_bit_for_safe + 1;
            hold_allready_set = 1;
            }
        }
        
        if (shift_data[0] & 0x04) {                   // No more false_parts fill Bit0 
            if (hold_bit_for_safe >= 1) {
                shift_data[0] = shift_data[0] | 0b00001110; // Fill FIFO Bit0 for safety eject 
                if (relase_allready_set == 0){
                    hold_bit_for_safe = hold_bit_for_safe - 1;
                    relase_allready_set = 1;
                }
                
            }
        }
    }
}

void show_output(void) {
    //Set LED if false Part detected 
    if (output > 0) {
        time_NOK = 5;
        OutNOK = 1;
    } else {
        time_NOK = time_NOK - 1;
        if (time_NOK < 1) {
            OutNOK = 0;
            time_NOK = 1;
        }


    }
}

void show_sonden(void) {
    //Set LED if SondeA is On
    if (SondeA == 1) {
        OutSA = 1;
    } else {
        OutSA = 0;
    }
}

void lever_output(void) {
    //If bit on specific Position in FIFI is set (part, part_bit) lever is ON for time lever_long_time
    if (((1 << part_bit) & shift_data[part]) != 0) { //Check Schiftregister [Byte = part][Bit = part_bit]
        lever_on = 1;
    }
    else {
        lever_on = 0;
        lever_allready_set = 0;
    }

    if (lever_on == 1 & lever_allready_set == 0) { //Set Lever_long_time
        lever_allready_set = 1;
        lever_long_flag = lever1_long_time;
        lever_relais_flag = 1;  // Set the Relais via Flag
    }

    if (lever_long_flag >= 1) {                      // Decrement Lever Time / Lever sort Parts OK/NOK
        lever_long_flag = lever_long_flag - 1;
    } else {
        lever_relais_flag = 0;  // Reset the Relais via Flag
    }
    
    if (count != count_old_lever) { //If next count -  lever to zero
        lever_allready_set = 0;
        count_old_lever = count;
    }

}

void lever_man_auto(void) {
    //Switch between Automatic and manual if button is pressed
    if (InLeverTest == 0) {
        OutLeverR = 1;
        OutLeverLed = 1;
    } else {
        if (lever_relais_flag == 1) {
            OutLeverR = 1;
            OutLeverLed = 1;
        } else {
            OutLeverR = 0;
            OutLeverLed = 0;

        }

    }
}

void invert_sonden(void) {
    if (InSA == 1) {    // Invert Sonde A
        SondeA = 0;
    } else {
        SondeA = 1;
    }

    if (InSB == 1) {    // Invert Sonde A
        SondeB = 0;
    } else {
        SondeB = 1;
    }
}

void check_device(void) {
    //Check status of Elotes PL 600 
    if (InDEV_R == 0) {
        error_status = error_status | 0b00000001; // Set Error_bit 0 Dev NOT Ready
    } else {
        error_status = error_status & ~0b00000001; // Clear Error_bit 0
    }

    if (InDEV_F == 0) {
        error_status = error_status | 0b00000010; // Set Error_bit 1 Dev faut
    } else {
        error_status = error_status & ~0b00000010; // Clear Error_bit 1
    }

}

void show_alarm(void) {
    if (error_status > 0) {
        OutSteuerS = 0;
        OutAlarm = 1;
    } else {
        OutAlarm = 0;
        OutSteuerS = 1;
    }

}

void show_hupe(void) {
    if (InHupe == 1) {
        if (error_status > 0) {
            OutHupe = 1;
        } else {
            OutHupe = 0;
        }
    } else {
        OutHupe = 0;
    }
}


void main(void) {

    init(); //PORTS init
    init_comparator();
    init_timer1();
    init_serial();

    t_plus_time = 1; //Variable Init
    lever1_long_time = 2;
    part = 0;
    part_bit = 5;
    count = 0;
    error_status = 0b00000111;
    


    while (1) {
        invert_sonden();                // Read Sonden
   
        show_sonden();                  // Show Sonden Input on LED  
        lever_man_auto();               // Lever Manual control
        check_device();                 // Send errror if Device not ready
        show_alarm();                   // Alarm LED on/off
        show_hupe();                    // Hupe on /off
        

        t_plus();                       // Puls SondeB länger
        output = SondeA & (~SondeB_long); // Fehlersignal logic
        show_output();                  //Show output NOK
        counter();                      // Count Parts SondeB
        shift_register();               //Shift bytes
        fill_shift_register(output);    //Set shift_data[0] bit0 
        save_eject();                   // Set bit before and after for safe_eject
        lever_output();                 // If Bit in ShiftRegister is Set -> LeverON for time lever_long_time    
        read_command();                 // Read Command from UART 
        send_command();
        
        wait_timer1();                  // __delay_ms(9); // loop Delay 
    }
}

