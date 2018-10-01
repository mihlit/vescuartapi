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

#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include "linux_hwserial.h"
#include "vescuartapi.h"

bool gotvalues;
void valuesCB(VescUartApi *) { gotvalues = true; }

int main(int argc, char *argv[])
{
  int i;
  uint8_t vescbuffer[1024];

  // use serial port specified as a command line argument or use default
  HardwareSerial uart(argc > 1 ? argv[1] : "/dev/ttyUSB0");

  //initialize vesc api with buffer and uart
  VescUartApi vesc(vescbuffer, sizeof(vescbuffer), &uart);

  // initialize uart to VESC's default speed 115200 baud
  if (uart.begin(115200) < 0)
      exit(1);

#if 0
  // Loopback uart test. If you don't trust your adapter
  // Just connect usb-uart adapter and connect its RX to TX with jumper cable
  const char send[]="Hello world test\n";
  for(int j=0;j<100;j++)
  {
    uart.write((uint8_t*)send, strlen(send));
    for(i=200; i>0; --i)
    {
        usleep(1000);
        while(uart.available())
        {
          char c=uart.read();
          printf("%c", c);
        }
    }
  }
  exit(0);
#endif

  

  // ask 2 seconds for FW version
  for(i=200; i>0 && !vesc.fw_version[0]; --i)
  {
    // ask for FW version and repeat question every 500 ms
    if (!(i%50))
      vesc.askFwVersion();
    usleep(10000);
    vesc.loopstep();
  }

  // check FW version for compatibility
  if (!vesc.fw_version[0])
  {
    printf("Error: Can't get answer from VESC\n");
    exit(1);
  }
  else
    printf("Answer after %d ms, firmware %d.%d\n", (200-i)*10, vesc.fw_version[0], vesc.fw_version[1]);

  //TODO: check firmware compatibility

  // set callback for COMM_GET_VALUES - whenever we get new answer, call function "valuesCB"
  vesc.setRxDataCB(COMM_PACKET_ID::COMM_GET_VALUES, valuesCB);
  // valuesCB with set gotvalues flag, initialize it to false
  gotvalues = false;

  vesc.askValues();

  //for 5 seconds...
  for(i=50;i>0;--i)
  {
    // process incomming data, if there are any
    vesc.loopstep();

    // if we got new COMM_GET_VALUES answer...
    if (gotvalues)
    {
      gotvalues = false;
      printf("rpm: %d\nvoltage: %2.02f\ncurrent: %4.02f\nduty: %1.03f\n\n\n",
	     (int)vesc.values_data.rpm,
	     vesc.values_data.input_voltage,
	     vesc.values_data.avg_motor_current,
	     vesc.values_data.duty_cycle_now);
      vesc.askValues();
    }
    //note: we have to send any command within VESC's timeout limit, or it will stop the motor
//     vesc.setDuty(60000); // ~ 0.60
    vesc.setCurrent(1234);  // ~ 1.234 A

    // limit rate to something sane
    usleep(100000);
  }

  printf("Stopping motor, before exit...\n");
  vesc.setCurrent(0);
  sleep(1);

  return 0;
}
