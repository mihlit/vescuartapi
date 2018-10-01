/*  Copyright (c) 2018 Michal Hlavinka

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <avr/io.h>
#include <avr/interrupt.h>
#include "avr_hwserial.h"

#ifndef F_CPU
# error "Don't forget to specify F_CPU speed"
#endif


#if defined(__AVR_ATmega328P__)

# define VESC_UCSRB	UCSR0B
# define VESC_UCSRC	UCSR0C
# define VESC_UBRRH	UBRR0H
# define VESC_UBRRL	UBRR0L
# define VESC_RXCIE	RXCIE0
# define VESC_UDRIE	UDRIE0
# define VESC_RXEN	RXEN0
# define VESC_TXEN	TXEN0
# define VESC_UDR	UDR0
# define VESC_USART_UDRE_vect	USART_UDRE_vect
# define VESC_USART_RXC_vect	USART_RX_vect
# define VESC_UCSZ0	UCSZ00
# define VESC_UCSZ1	UCSZ01
# define VESC_UCSZ2	UCSZ02
# define VESC_UPM0	UPM00
# define VESC_UPM1	UPM01
#elif defined(__AVR_ATmega2560__)

#if 0
// SERIAL 0
# define VESC_UCSRB	UCSR0B
# define VESC_UCSRC	UCSR0C
# define VESC_UBRRH	UBRR0H
# define VESC_UBRRL	UBRR0L
# define VESC_RXCIE	RXCIE0
# define VESC_UDRIE	UDRIE0
# define VESC_RXEN	RXEN0
# define VESC_TXEN	TXEN0
# define VESC_UDR	UDR0
# define VESC_USART_UDRE_vect	USART0_UDRE_vect
# define VESC_USART_RXC_vect	USART0_RX_vect
# define VESC_UCSZ0	UCSZ00
# define VESC_UCSZ1	UCSZ01
# define VESC_UCSZ2	UCSZ02
# define VESC_UPM0	UPM00
# define VESC_UPM1	UPM01
#else
// SERIAL1
# define VESC_UCSRB	UCSR1B
# define VESC_UCSRC	UCSR1C
# define VESC_UBRRH	UBRR1H
# define VESC_UBRRL	UBRR1L
# define VESC_RXCIE	RXCIE1
# define VESC_UDRIE	UDRIE1
# define VESC_RXEN	RXEN1
# define VESC_TXEN	TXEN1
# define VESC_UDR	UDR1
# define VESC_USART_UDRE_vect	USART1_UDRE_vect
# define VESC_USART_RXC_vect	USART1_RX_vect
# define VESC_UCSZ0	UCSZ10
# define VESC_UCSZ1	UCSZ11
# define VESC_UCSZ2	UCSZ12
# define VESC_UPM0	UPM10
# define VESC_UPM1	UPM11

#endif
#else

#error "unknown MCU"

#endif

int HardwareSerial::begin(uint32_t baud)
{
   // usual equation as from datasheet
   uint32_t baud_prescale = ((F_CPU / (baud * 16UL))-1);
   // but sometimes, it's better to +1 as we don't do rounding
   // that way, we will have lower baud rate error,
   // which is important especially for 16MHz and 115200 bd combination
   uint32_t back1 = F_CPU/16UL/(baud_prescale+1);
   uint32_t back2 = F_CPU/16UL/(baud_prescale+2);
   if (back1-baud > baud-back2) baud_prescale++;
   UCSR0A &= ~_BV(U2X0); //no double speed mode
   VESC_UBRRH = baud_prescale >> 8; // Load upper 8-bits of the baud rate value into the high byte of the UBRR register
   VESC_UBRRL = baud_prescale & 0xff; // Load lower 8-bits of the baud rate value into the low byte of the UBRR register

   //asynchronous operation
   VESC_UCSRC &= ~(_BV(UMSEL01)|_BV(UMSEL00));

   //no parity
   VESC_UCSRC &= ~(_BV(VESC_UPM1)|_BV(VESC_UPM0));

   //one stop bit
   VESC_UCSRC &= ~_BV(USBS0);

   // 8bit size
   VESC_UCSRB &= ~_BV(VESC_UCSZ2);
   VESC_UCSRC |= _BV(VESC_UCSZ1) | _BV(VESC_UCSZ0);

   VESC_UCSRB |= _BV(VESC_RXEN) | _BV(VESC_TXEN);   // Turn on the transmission and reception circuitry

   VESC_UCSRB |= _BV(VESC_RXCIE) | _BV(VESC_UDRIE); // Enable the USART Recieve Complete interrupt (USART_RXC)
   sei(); // Enable the Global Interrupt Enable flag so that interrupts can be processed
   return 0;
}



// there is a space in transmit buffer for another byte
ISR(VESC_USART_UDRE_vect)
{
  if (!vescuart.txbuf.length())
  {
    // If Buffer Empty
    VESC_UCSRB &= ~_BV(VESC_UDRIE); // disable output buff empty interrupt
  }
  else
  {
    VESC_UDR = vescuart.txbuf.pop();
  }
}

ISR(VESC_USART_RXC_vect)
{
   vescuart.rxbuf.push(VESC_UDR);
}

int HardwareSerial::available()
{
  return rxbuf.length();
}

uint8_t HardwareSerial::read()
{
    return rxbuf.pop();
}

void HardwareSerial::write(const uint8_t *buf, int len)
{
    uint8_t start = !txbuf.length();
    txbuf.store(buf, len);
    if (start)
      VESC_UCSRB |= (1 << VESC_UDRIE); 
}

