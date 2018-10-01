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
#include <util/delay.h>

#include "avr_hwserial.h"
#include "vescuartapi.h"

// If you are using arduno board, select which one
// #define ARDUINO_AVR_NANO
#define ARDUINO_AVR_MINI
// #define ARDUINO_AVR_MEGA

/* function for long delay */
void delay_ms(uint16_t ms)
{
  while ( ms ) 
  {
    _delay_ms(1);
    ms--;
  }
}
#ifdef ARDUINO_AVR_MINI
//arduino MINI led on PB5
#define LEDPORT PORTB
#define LEDDDR	DDRB
#define LEDPIN	PB5

#elif ARDUINO_AVR_MEGA
//arduino MEGA led on PB5
#define LEDPORT PORTB
#define LEDDDR	DDRB
#define LEDPIN	PB7
#endif

void ledDebug(uint8_t long_, uint8_t short_, bool halt)
{
  int8_t i;
  while(true)
  {
    for(i=0;i<long_;++i)
    {
      LEDPORT |= _BV(LEDPIN);   // turn the LED on (HIGH is the voltage level)
      delay_ms(800);                       // wait for a second
      LEDPORT &= ~_BV(LEDPIN);    // turn the LED off by making the voltage LOW
      if (i+1<long_) delay_ms(800);                       // wait for a second
    }
    if (long_ && short_)
      delay_ms(400);
    for(i=0;i<short_;++i)
    {
      LEDPORT |= _BV(LEDPIN);   // turn the LED on (HIGH is the voltage level)
      delay_ms(200);                       // wait for a second
      LEDPORT &= ~_BV(LEDPIN);    // turn the LED off by making the voltage LOW
      if (i+1<short_) delay_ms(200);                       // wait for a second
    }
    if (!halt) { delay_ms(2000); return;}
    delay_ms(3000);
  }
}


uint8_t uartrxbuf[100];
uint8_t uarttxbuf[100];
HardwareSerial vescuart(uartrxbuf, sizeof(uartrxbuf), uarttxbuf, sizeof(uarttxbuf));

bool gotvalues;
void valuesCB(VescUartApi *) { gotvalues = true; }

int main()
{
  int i;
  uint8_t vescbuffer[128];

  LEDDDR |= _BV(LEDPIN);

  ledDebug(1,1,0);

  //initialize vesc api with buffer and uart
  VescUartApi vesc(vescbuffer, sizeof(vescbuffer), &vescuart);

  // initialize uart to VESC's default speed 115200 baud
  vesc.begin(115200);

again:


  // check FW version for compatibility
  // up to 5 seconds try to get answer, repeat question every 500 ms
  // because VESC has a long boot time and AVR without bootlader can boot pretty fast
  for(i=500; i>0 && !vesc.fw_version[0]; --i)
  {
    if (!(i%50))
      vesc.askFwVersion();
    delay_ms(10);
    vesc.loopstep();
  }

  if (!vesc.fw_version[0])
  {
//  printf("Error: Can't get answer from VESC\n");
    ledDebug(2,2,0);
    goto again;
  }
//else
//  printf("Answer after %d ms, firmware %d.%d\n", (200-i)*10, vesc.fw_version[0], vesc.fw_version[1]);

  ledDebug(0,3,false);

  //TODO: check firmware compatibility

  // set callback for COMM_GET_VALUES - whenever we get new answer, call function "valuesCB"
  vesc.setRxDataCB(COMM_PACKET_ID::COMM_GET_VALUES, valuesCB);
  // valuesCB with set gotvalues flag, initialize it to false
  gotvalues = false;

  vesc.askValues();

  //for 5 seconds...
  for(i=50; i>0; --i)
  {
    // process incomming data, if there are any
    vesc.loopstep();

    // if we got new COMM_GET_VALUES answer...
    if (gotvalues)
    {
      // NOTE: it's better to trigger askValues() on timer as if any answer get corrupted 
      // because of some noise on uart wires, this chain would stop
      gotvalues = false;
//       printf("rpm: %d\nvoltage: %2.02f\ncurrent: %4.02f\nduty: %1.03f\n\n\n",
// 	     (int)vesc.values_data.rpm,
// 	     vesc.values_data.input_voltage,
// 	     vesc.values_data.avg_motor_current,
// 	     vesc.values_data.duty_cycle_now);
      vesc.askValues();
    }
    //note: we have to send any command within VESC's timeout limit, or it will stop the motor
//  vesc.setDuty(60000); // ~ 0.60
    vesc.setCurrent(600);  // ~ 0.6 A

    // limit rate to something sane
    delay_ms(100);
  }

//printf("Stopping motor, before exit...\n");
  vesc.setCurrent(0);
  delay_ms(1000);

  // that's all for now...
  ledDebug(1,0,true);
  for(;;);
  
  return 0;

  
  
}
