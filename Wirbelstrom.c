/*
 * File:   Wirbelstrom.c
 * Author: Michael
 * Programm: Programm verwendet zwei Sonden auszuwerten
 * Outputs werden um XX ms verlängert
 * Output ist ON wenn SondeA == 1 and NICHT(SondeB)
 * Created on 7. März 2018, 20:48
 */
/* PIN BELEGUNG
 * 
 RB0 = Auswerfer
 RB1 = RX
 RB2 = TX
 RB3 = Sonde A 
 RB4 = Sonde B 
 RB5 = B out long
 RB6 = Counter
 RB7 = Serial Infos
 * 
 RA0 = Out Error
 RA1 = 
 RA2 = 
 RA3 = 
 
 */
// CONFIG
#pragma config FOSC = HS  // 4Mhz Oscillator Selection bits (INTOSC oscillator: I/O function on RA6/OSC2/CLKOUT pin, I/O function on RA7/OSC1/CLKIN)
#pragma config WDTE = OFF       // Watchdog Timer Enable bit (WDT disabled)
#pragma config PWRTE = ON      // Power-up Timer Enable bit (PWRT disabled)
#pragma config MCLRE = ON       // RA5/MCLR/VPP Pin Function Select bit (RA5/MCLR/VPP pin function is MCLR)
#pragma config BOREN = OFF      // Brown-out Detect Enable bit (BOD disabled)
#pragma config LVP = OFF         // Low-Voltage Programming Enable bit (RB4/PGM pin has PGM function, low-voltage programming enabled)
#pragma config CPD = OFF        // Data EE Memory Code Protection bit (Data memory code protection off)
#pragma config CP = OFF         // Flash Program Memory Code Protection bit (Code protection off)

#include <xc.h>
#include <stdlib.h>
#include <pic16f628a.h>
#define _XTAL_FREQ 8000000

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
char lever_on, lever_long_flag;
char position;
char commands[7] = {0, 0, 0, 0, 0, 0, 0};
char t_plus_time;
char lever1_long_time;
char lever_allready_set;
char dumy1, dumy2, dumy3;
char new_data_ready;
char Txdata[6];
char daten;
char send_actual_byte;

void init(void) {
    nRBPU = 0; //Enables PORTB internal pull up resistors   
    TRISB = 0b00011110; //PORTB as out/In
    TRISA = 0b11111110; //PORTA as out/in
    PORTA = 0x00; //All LEDs OFF
    PORTB = 0x00;
}

void init_comparator(void) {
    CMCON = 0b00000111; // Comparator OFF
    //CMCON = 0b00000100; //Zwei OP 
    //TRISA = 0b00001111; //Pin as Input
}

void init_serial(void) {
    SPBRG = 25; //9200 baud @4MHz
    TXSTA = 0b00100100; //  TX, 8bit,FRame e OverRun errors, high_speed
    RCSTA = 0b10010110; // USART,8bit,FRame e OverRun errors
    RCSTAbits.CREN = 1; // Receiver ON
    TRISBbits.TRISB1 = 1; // TX und SX auf Input (beide INPUT!)
    TRISBbits.TRISB2 = 1;
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

void interrupt read_serial()// In t e r r u p t r o u t i n e
{
    if (PIR1bits.RCIF == 1) { //Interupt von USART
        new_data = RCREG; // Daten  lesen

        if (new_data == 255) {
            PORTBbits.RB7 = 1;
            position = 0; //Reset DataCounter to "0" 
        } else {
            PORTBbits.RB7 = 0;
        }

        commands[position] = new_data; //new SerialByte to commands array
        position++;

        if (position == 7) {
            position = 0;
            new_data_ready = 1; //Read anzahl bytes then set new_data_ready flag
        }

        if (FERR) {
            PORTAbits.RA0 = 1;
        }

        if (OERR) {
            PORTAbits.RA0 = 1;
            RCSTAbits.CREN = 0; // CREN On -> OFF -> ON Clears Errors USART
            RCSTAbits.CREN = 1;
        }

        PIR1bits.RCIF = 0; //Interupt Flag löschen

    }
}

void send_command(void) {
    
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
            int_to_str(dumy2);
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
            send_serial(10); //Send NEWLINE
            send_serial(13);
            break;
        case 10:
            send_actual_byte = 0;
            break;
        default:
            send_actual_byte = 0;
         
    }
    
    
    
//    int_to_str(255); // Send Startbyte
//    int_to_str(t_plus_time);
//    int_to_str(lever1_long_time);
//    int_to_str(dumy2);
//    int_to_str(count);
//    int_to_str(part);
//    int_to_str(part_bit);
//    int_to_str(shift_data[0]); //(shift_data[0]);
//    send_serial(10); //Send NEWLINE
//    send_serial(13);
}

void read_command(void) {
    if (new_data_ready == 1) {
        dumy1 = commands[0];
        t_plus_time = commands[1];
        lever1_long_time = commands[2]; //Scommands[2];
        dumy2 = commands[3];
        count = commands[4];
        part = commands[5];
        part_bit = commands[6];
        new_data_ready = 0;
    }

}

void t_plus(void) {
    if (SondeB == 1) {
        out_long = t_plus_time;
        PORTBbits.RB5 = 1;
        SondeB_long = 1;
    } else {
        if (out_long > 1) {
            out_long = out_long - 1;
            PORTBbits.RB5 = 1;
            SondeB_long = 1;
        } else {
            PORTBbits.RB5 = 0;
            out_long = 1;
            SondeB_long = 0;
        }
    }
}

void counter(void) {
    if (SondeB == 1) {
        if (allready_set == 0) {
            count = count + 1;
            allready_set = 1;
            PORTBbits.RB6 = 1;
        }
    } else {
        allready_set = 0;
        PORTBbits.RB6 = 0;
    }

}

void shift_register(void) {
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
//Shift data left 1bit  
//    if (count != count_old){    
//        shift_data[3] <<= 1;
//        if(shift_data[2] & 0x80)
//            shift_data[3] |= 1;
//        shift_data[2] <<= 1;
//        if(shift_data[1] & 0x80)
//            shift_data[2] |= 1;
//        shift_data[1] <<= 1;   
//        if(shift_data[0] & 0x80)
//            shift_data[1] |= 1;
//        shift_data[0] <<= 1;  
//    count_old = count;

void fill_shift_register(char output) {
    if (output != 0) {
        if (out_allready == 0) {
            shift_data[0] = shift_data[0] | (1 << 0); //Set bit Null
            out_allready = 1;
        }
    } else {
        out_allready = 0;
    }
}

void info_error(void) {
    if (output > 0) {
        PORTBbits.RB7 = 1;
    } else {
        PORTBbits.RB7 = 0;
    }
}

void lever_output(void) {
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
        PORTBbits.RB0 = 1;
    }

    if (lever_long_flag > 1) {                      // Decrement Lever Time / Lever sort Parts OK/NOK
        lever_long_flag = lever_long_flag - 1;
    } else {
        PORTBbits.RB0 = 0;
    }
    
    if (count != count_old_lever) { //If next count -  lever to zero
        lever_allready_set = 0;
        count_old_lever = count;
    }

}

void main(void) {

    init(); //PORTS init
    init_comparator();
    init_serial();

    t_plus_time = 20; //Variable Init
    lever1_long_time = 20;
    part = 0;
    part_bit = 5;
    count = 0;

    while (1) {
        PORTAbits.RA0 = 1;
        SondeA = PORTBbits.RB3; // Read Sonden
        SondeB = PORTBbits.RB4;

        t_plus(); // Puls SondeB länger
        output = SondeA & (~SondeB_long); // Fehlersignal logic
        info_error(); //Output sets Info LED
        counter(); // Count Parts SondeB
        shift_register(); //Shift bytes
        fill_shift_register(output); //Set shift_data[0] bit0   
        lever_output(); // If Bit in ShiftRegister is Set -> LeverON for time lever_long_time    
        read_command(); // Read Command from UART 

        __delay_ms(5); // loop Delay  

        send_command();
        PORTAbits.RA0 = 0;

    }
}

