#ifndef _VESCUARTAPI_H_
#define _VESCUARTAPI_H_

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

#if defined(ARDUINO) && ARDUINO >= 100
#include <Arduino.h>
#elif defined(AVRBUILD)
# include <string.h>
# include "avr_hwserial.h"

#elif defined(LINUXBUILD)
# include <cstring>
# include <cstdint>
//# include <cstdlib>
//# include <cstdio>
# include "linux_hwserial.h"

#else
# error "You have to specify a build type, use one of -DARDUINO -DAVRBUILD -DLINUXBUILD"
//include "WProgram.h"



#endif


#include "datatypes.h"
//#include <cstddef>

const int8_t MIN_RX_PACKET_SIZE = 6; // 1B fmt, 1B size, 1B payload, 2B crc, 1B end

class HardwareSerial;

struct ValuesData {
  float temp_fet;
  float temp_motor;
  float avg_motor_current;
  float avg_input_current; 
// float avg_id;
// float avg_iq;
  float duty_cycle_now;
  float rpm;
  float input_voltage;
  float amp_hours;
  float amp_hours_charged;
  int32_t tachometer_value;
  int32_t tachometer_abs_value;
  int8_t fault;
  float pid_pos;
  int8_t controller_id;
};

class VescUartApi {
  private:
    uint8_t *buf;
    const int16_t bufsize;
    HardwareSerial *uart;
    int16_t buflast;   // last valid byte or -1 if buffer empty
    void(*getValuesCB)(VescUartApi *);
    
    void rcvd_GET_VALUES(const uint8_t *data, uint16_t packetsize);
    void rcvd_FW_VERSION(const uint8_t *data, uint16_t packetsize);
    int16_t sendCommandInplace(uint8_t *buf, int16_t cmdlen);
    
  public:
    ValuesData values_data;
    uint8_t fw_version[2];
    VescUartApi(uint8_t *buf, const int bufsize, HardwareSerial *uart) : buf(buf), bufsize(bufsize), uart(uart), buflast(-1), getValuesCB(nullptr), fw_version{0,0}
    {
      
    }
    void begin(int32_t baudrate) { uart->begin(baudrate); }
    int16_t checkPayloadCRC(uint8_t *packet, int16_t packetsize, uint8_t *payload, int16_t payloadsize);
    void loopstep();
    void consumePacket(const uint8_t *packet, uint16_t packetsize);
    void setRxDataCB(COMM_PACKET_ID packet_id, void(*cb)(VescUartApi *));
    int16_t sendCommand(uint8_t *cmd, int16_t cmdlen);
    void askValues();
    void askFwVersion(); //COMM_FW_VERSION
    void pingAmAlive();//COMM_ALIVE
    void setCurrent(int32_t miliamps);
    void setCurrentBrake(int32_t miliamps);
    void setDuty(int32_t duty); //uses [-1e5, 1e5] interval for value
    void setRPM(int32_t rpm);
    //void setPod(int32_t pos); 1e6
    //void setHandbrake(float hb); // 1e3
    //void getDecodedPPM();
    //void getDecodedADC();
};

#endif // _VESCUARTAPI_H_
