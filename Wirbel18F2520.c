/*
 * File:   Wirbelstrom.c
 * Author: Michael
 * Programm: Programm verwendet zwei Sonden auszuwerten
 * Outputs werden um XX ms verlängert
 * Output ist ON wenn SondeA == 1 and NICHT(SondeB)
 * Created on 10. Sep 2018
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
#define InDEV_F PORTAbits.RA2
#define InDEV_R PORTAbits.RA3
#define InSB    PORTAbits.RA4
#define InSA    PORTAbits.RA5

#define OutLeverR   LATBbits.LATB0 // OHNE FUNKTION PB0 nicht funktionst
#define OutSteuerS  LATBbits.LATB1
#define OutHupe     LATBbits.LATB2
#define OutAlarm    LATBbits.LATB3
#define OutLeverLed LATBbits.LATB4
#define OutNOK      LATBbits.LATB5
#define OutSB       LATBbits.LATB6
#define OutSA       LATBbits.LATB7


#define InHupe      PORTCbits.RC0
#define InLeverTest PORTCbits.RC1

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
char lever_on, lever_long_flag, lever_relais_flag;
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
    nRBPU = 1; //Disable (1) PORTB internal pull up resistors
    OSCCON = 0b01100000;  // Set to 4Mhz intern Clock
    ADCON1 = 0b00001111; // A/D off
    TRISA =  0b11111111; //PORTA as out/in
    TRISB =  0b00000000; //PORTB as out/In
    TRISC =  0b11111111; //PORTC
    PORTA =  0x00; //All OFF
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
    while (PIR1bits.TMR1IF == 0) {
    }
    TMR1H = 220;
    TMR1L = 0;
    TMR1IF = 0;
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

void __interrupt () my_isr_routine (void)  // In t e r r u p t r o u t i n e
{
    if (PIR1bits.RCIF == 1) { //Interupt von USART
        new_data = RCREG; // Daten  lesen

        if (new_data == 255) {
            OutAlarm = 1;
            position = 0; //Reset DataCounter to "0" 
        } else {
            OutAlarm = 0;
        }

        commands[position] = new_data; //new SerialByte to commands array
        position++;

        if (position == 7) {
            position = 0;
            new_data_ready = 1; //Read anzahl bytes then set new_data_ready flag
        }

        if (FERR) {
            OutAlarm = 1;
        }

        if (OERR) {
            OutAlarm = 1;
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
    if (SondeB == 1) {
        if (allready_set == 0) {
            count = count + 1;
            allready_set = 1;
        }
    } else {
        allready_set = 0;
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

void show_output(void) {
    if (output > 0) {
        OutNOK = 1;
    } else {
        OutNOK = 0;
    }
}

void show_sonden(void) {
    if (SondeA == 1) {
        OutSA = 1;
    } else {
        OutSA = 0;
    }
    if (SondeB == 1) {
        OutSB = 1;
    } else {
        OutSB = 0;
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
        lever_relais_flag = 1;  // Set the Relais via Flag
        //OutLeverR = 1;
        //OutLeverLed = 1;
    }

    if (lever_long_flag > 1) {                      // Decrement Lever Time / Lever sort Parts OK/NOK
        lever_long_flag = lever_long_flag - 1;
    } else {
        lever_relais_flag = 0;  // Reset the Relais via Flag
        //OutLeverR = 0;
        //OutLeverLed = 0;
    }
    
    if (count != count_old_lever) { //If next count -  lever to zero
        lever_allready_set = 0;
        count_old_lever = count;
    }

}

void lever_man_auto(void) {
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


void main(void) {

    init(); //PORTS init
    init_comparator();
    init_timer1();
    init_serial();

    t_plus_time = 20; //Variable Init
    lever1_long_time = 20;
    part = 0;
    part_bit = 5;
    count = 0;
    


    while (1) {
        OutAlarm = 1;
        SondeA = InSA; // Read Sonden
        SondeB = InSB;
        
        show_sonden(); // Show Sonden Input on LED  
        lever_man_auto(); // Lever Manual control

        t_plus(); // Puls SondeB länger
        output = SondeA & (~SondeB_long); // Fehlersignal logic
        show_output(); //Show output NOK
        counter(); // Count Parts SondeB
        shift_register(); //Shift bytes
        fill_shift_register(output); //Set shift_data[0] bit0   
        lever_output(); // If Bit in ShiftRegister is Set -> LeverON for time lever_long_time    
        read_command(); // Read Command from UART 
        send_command();
        
        wait_timer1();  // __delay_ms(5); // loop Delay 
        OutAlarm = 0;
    }
}

