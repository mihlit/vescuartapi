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

#include <vescuartapi.h>

// RX buffer, size affects the biggest packet it can receive
uint8_t vescbuf[128];
VescUartApi vesc(vescbuf, 128, &Serial);

// use this variable and callback, to set gotvalues = true,
// when new values are received
bool gotvalues;
void valuesCallback(VescUartApi *) { gotvalues = true; }

// last time, we asked fot values. It's to limit data rate without using delay
uint32_t lastmilis_values = 0;
// last time, we set motor parameters
uint32_t lastmilis_motor = 0;

void setup() {
	// default VESC baud rate is 115200
	vesc.begin(115200);

	// when there is new COMM_GET_VALUES answer, call valuesCallback function
	gotvalues = false;
	vesc.setRxDataCB(COMM_PACKET_ID::COMM_GET_VALUES, valuesCallback);
}

void loop() {
	// let vescuartapi do whatever it needs, like read and act on received data
	vesc.loopstep();

	// if we got new COMM_GET_VALUES answer...
	if (gotvalues)
	{
		gotvalues = false;
		// do something with those values
		// whatever you want :)
		// we won't use it for anything
	}

	if (millis() > lastmilis_values + 100)
	{
		//every 100 ms +-
		vesc.askValues();
		lastmilis_values = millis();
	}

	// let motor run a bit after 1 second, for 4 seconds
	uint32_t now = millis();
	if (now > 1000 && now > lastmilis_motor + 100)
	{
		if (now < 5000)
		{
			// note: we have to send any command within VESC's timeout limit,
			//       or it will stop the motor. That's why we set current repeatedly
			//       and not just once

			// set current to 600 mA
			vesc.setCurrent(600);
			// or set duty cycle to 30 %
			//vesc.setDuty(30000); // ~ 0.30
		}
		else
		{
			// stop motor
			vesc.setCurrent(0);
		}
		lastmilis_motor = now;
	}
}

