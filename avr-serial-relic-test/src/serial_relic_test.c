/*
    This file is part of the ChipWhisperer Example Targets
    Copyright (C) 2014 NewAE Technology Inc.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>
#include <stdlib.h>

#define __AVR_ATmega328P__

#include "uart.h"

#define IDLE 0
#define KEY 1
#define PLAIN 2

char hex_lookup[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

uint8_t* hex_decode(const char *in, int len,uint8_t *out)
{
    unsigned int i, t, hn, ln;

    for (t = 0,i = 0; i < len; i+=2,++t) {

        hn = in[i] > '9' ? (in[i]|32) - 'a' + 10 : in[i] - '0';
        ln = in[i+1] > '9' ? (in[i+1]|32) - 'a' + 10 : in[i+1] - '0';
        out[t] = (hn << 4 ) | ln;
    }

    return out;
}

void hex_print(const uint8_t * in, int len, char *out)
{
    unsigned int i,j;
    j=0;
    for (i=0; i < len; i++) {
        out[j++] = hex_lookup[in[i] >> 4];
        out[j++] = hex_lookup[in[i] & 0x0F];
    }

    out[j] = 0;
}

#define KEY_LENGTH 16
#define BUFLEN (KEY_LENGTH*4)

//Buffers for decoding
char asciibuf[BUFLEN];
uint8_t pt[KEY_LENGTH];
uint8_t tmp[KEY_LENGTH];


//We want to use the AVR ADC-pins, since they have a seperate power rail
#define trigger_setup() DDRC |= 0x01
#define trigger_high()  PORTC |= 0x01
#define trigger_low()   PORTC &= ~(0x01)


int main (void) {
    init_uart0();

    /* For 2 MHz crystal use this hack */
    //BAUD0L_REG = 12;

    trigger_setup();

    /* Uncomment this to get a HELLO message for debug */
    /*
    output_ch_0('h');
    output_ch_0('e');
    output_ch_0('l');
    output_ch_0('l');
    output_ch_0('o');
    output_ch_0('\n');
    */

    /* Super-Crappy Protocol works like this:

    Send kKEY
    Send pPLAINTEXT
    *** Encryption Occurs ***
    receive rRESPONSE

    e.g.:

    kE8E9EAEBEDEEEFF0F2F3F4F5F7F8F9FA\n
    p014BAF2278A69D331D5180103643E99A\n
    r6743C3D1519AB4F2CD9A78AB09A511BD\n
    */

    char c;
    int ptr = 0;
    
    char state = 0;

#ifdef VARYING_CLOCK
    uint8_t newosc;
#endif

    while(1){

        c = input_ch_0();

        if (c == 'x') {
            ptr = 0;
            state = IDLE;
            continue;
        }

        if (c == 'k') {
            ptr = 0;
            state = KEY;
            continue;
        }

        else if (c == 'p') {
            ptr = 0;
            state = PLAIN;
            continue;
        }


        else if (state == KEY) {
            if ((c == '\n') || (c == '\r')) {
                asciibuf[ptr] = 0;
                hex_decode(asciibuf, ptr, tmp);

                //If you want to use 'key' for something do so here
                //Decoded value held in 'tmp' register
                
                state = IDLE;
            } else {
                asciibuf[ptr++] = c;
            }
        }

        else if (state == PLAIN) {
            if ((c == '\n') || (c == '\r')) {
                asciibuf[ptr] = 0;
                hex_decode(asciibuf, ptr, pt);

                
                /**********************************
                 * Start user-specific code here. */
                trigger_high();
                
                //16 hex bytes held in 'pt' were sent
                //from the computer. Store your response
                //back into 'pt', which will send 16 bytes
                //back to computer. Can ignore of course if
                //not needed

                int len = ptr;

                for (int i = 0; i < len; i++) {
                    pt[i]++;
                }

                trigger_low();
                /* End user-specific code here. *
                 ********************************/

                /* Print Results */
                hex_print(pt, KEY_LENGTH, asciibuf);

                output_ch_0('r');
                for(int i = 0; i < KEY_LENGTH*2; i++){
                    output_ch_0(asciibuf[i]);
                }
                output_ch_0('\n');

                state = IDLE;
            } else {
                if (ptr >= BUFLEN){
                    state = IDLE;
                } else {
                    asciibuf[ptr++] = c;
                }
            }
        }
    }

    return 1;
}


