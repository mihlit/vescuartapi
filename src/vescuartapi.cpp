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

#include "vescuartapi.h"
#include "crc.h"
#include "datatypes.h"
#include "buffer.h"

// class HardwareSerial {
//   public:
//     int8_t available() { return 0; }
//     int8_t read() { return 0; }
// };

int16_t VescUartApi::checkPayloadCRC(uint8_t *packet, int16_t packetsize, uint8_t *payload, int16_t payloadsize)
{
  uint16_t expectedCRC, computedCRC;
  
  expectedCRC = packet[packetsize-3] << 8;
  //expectedCRC &= 0xff00;
  expectedCRC += packet[packetsize-2];

  computedCRC = crc16(payload, payloadsize);
  
  return (expectedCRC == computedCRC);
}        

bool shiftBufferToNewStart(uint8_t *buf, int16_t *buflast)
{
  int16_t i=1;
  for(; i<*buflast; ++i)
  {
    //find new possible packet start
    if (buf[i] == 2 || buf[i] == 3) 
    {
      *buflast -= i;
      memmove(buf, buf+i, *buflast);
      return true;
    }
  }
  *buflast = -1;
  return false;
}

void VescUartApi::loopstep()
{
  int16_t packetsize = 0;
  uint8_t payloadstart = 0;
  int16_t payloadsize = 0;
  while (uart->available())
  {
    uint8_t b = uart->read();
    if (buflast < 0) //waiting for possible begin of a packet
    {
      //packets can start only with 2 or 3 value, if looking for begin, throw away everything else
      if (b!=2 && b!=3) continue;  
      buf[0] = b;
      buflast = 0;
    }
    else
    {
      buf[++buflast] = b;
    }

  again:
    if (buflast >= MIN_RX_PACKET_SIZE && buflast+1 >= packetsize)
    {
      // for packet format, see https://github.com/vedderb/bldc/blob/master/packet.c#L45
      if (buf[0] == 2)
      {
	payloadsize = buf[1];
	payloadstart = 2;
	packetsize = payloadsize + 5; // 1B fmt 0x02, 1B size, ...., 2B CRC16, 1B end 0x03
      }
      else if (buf[0] == 3)
      {
	payloadsize = buf[1]<<8;
	payloadsize += buf[2];
	payloadstart = 3;
	packetsize = payloadsize + 6; // 1B fmt 0x03, 2B size, ...., 2B CRC16, 1B end 0x03

	//throw away, not supported for memory reasons
	if (packetsize > bufsize)
	{
	  // as we did not do any CRC checking and 0x03 2Byte size format could be just a uart garbage, 
	  // throw away and try to find new packet, don't risk waiting for 64kB of valid packet data to be
	  // thrown away after termination/crc check fail
	  buflast = -1;
	  packetsize = payloadsize = payloadstart = 0;
	  continue;
	}
      }
      else
      {
	// this should not happen with above checks, but anyway...
	buflast = -1;
	packetsize = payloadsize = payloadstart = 0;
	continue;
      }

      if (buflast+1 >= packetsize)
      {
	//we have enough data to parse the packet   
	if (buf[packetsize-1] != 3)
	{
	  packetsize = payloadsize = payloadstart = 0;
	  //wrong packet termination, garbage
	  if (shiftBufferToNewStart(buf, &buflast))
	    goto again;
	  //no packet begin found in buffer, search for it in future data
	  continue;
	}
	if (!checkPayloadCRC(buf, packetsize, buf+payloadstart, payloadsize))
	{
	  // CRC check failed, garbage
	  // and the question is: Garbage in data? crc? packet length?
	  packetsize = payloadsize = payloadstart = 0;
	  if (shiftBufferToNewStart(buf, &buflast))
	    goto again;
	  continue;
	}
	consumePacket(buf+payloadstart, payloadsize);
	
	//if there was more data in buffer than we used, shift buffer
	if (buflast >= packetsize)
	{
	  memmove(buf, buf+packetsize, buflast-packetsize+1);
	  buflast = 0;
	}
	else buflast = -1;
	packetsize = payloadsize = payloadstart = 0;
      }
    }
  }
}
// Added by AC to store measured values
struct bldcMeasure {
	//7 Values int16_t not read(14 byte)
	float avgMotorCurrent;
	float avgInputCurrent;
	float dutyCycleNow;
	long rpm;
	float inpVoltage;
	float ampHours;
	float ampHoursCharged;
	//2 values int32_t not read (8 byte)
	long tachometer;
	long tachometerAbs;
};

void VescUartApi::rcvd_FW_VERSION(const uint8_t *data, uint16_t datasize)
{
  if (datasize < 2) return;
  fw_version[0] = data[0];
  fw_version[1] = data[1];
}

void VescUartApi::rcvd_GET_VALUES(const uint8_t *data, uint16_t datasize, uint8_t selective)
{
  int32_t i = 0;
  uint32_t mask = 0xffffffff;
  if (selective)
    mask = buffer_get_uint32(data, &i);
  else if (datasize < 54) return;
  // for data format see https://github.com/vedderb/bldc/blob/master/commands.c
  if (mask & ((uint32_t)1 << 0)) {
    values_data.temp_fet = buffer_get_float16(data, 10.0, &i);
  }
  if (mask & ((uint32_t)1 << 1)) {
    values_data.temp_motor = buffer_get_float16(data, 10.0, &i);
  }
  if (mask & ((uint32_t)1 << 2)) {
    values_data.avg_motor_current = buffer_get_float32(data, 100.0, &i);
  }
  if (mask & ((uint32_t)1 << 3)) {
    values_data.avg_input_current = buffer_get_float32(data, 100.0, &i);
  }
  if (mask & ((uint32_t)1 << 4)) {
    // values_data.avg_id = buffer_get_float32(data, 100.0, &i);
    buffer_get_float32(data, 100.0, &i);
  }
  if (mask & ((uint32_t)1 << 5)) {
    // values_data.avg_iq = buffer_get_float32(data, 100.0, &i);
    buffer_get_float32(data, 100.0, &i);
  }
  if (mask & ((uint32_t)1 << 6)) {
    values_data.duty_cycle_now = buffer_get_float16(data, 1000.0, &i);
  }
  if (mask & ((uint32_t)1 << 7)) {
    values_data.rpm = buffer_get_float32(data, 1.0, &i);
  }
  if (mask & ((uint32_t)1 << 8)) {
  values_data.input_voltage = buffer_get_float16(data, 10.0, &i);
  }
  if (mask & ((uint32_t)1 << 9)) {
  values_data.amp_hours = buffer_get_float32(data, 10000.0, &i);
  }
  if (mask & ((uint32_t)1 << 10)) {
  values_data.amp_hours_charged = buffer_get_float32(data, 10000.0, &i);
  }
  if (mask & ((uint32_t)1 << 11)) {
    // values_data.watt_hours = buffer_get_int32(data, &i);
    buffer_get_int32(data, &i);
  }
  if (mask & ((uint32_t)1 << 12)) {
    // values_data.watt_hours_charged = buffer_get_int32(data, &i);
    buffer_get_int32(data, &i);
  }
  if (mask & ((uint32_t)1 << 13)) {
    values_data.tachometer_value = buffer_get_int32(data, &i);
  }
  if (mask & ((uint32_t)1 << 14)) {
    values_data.tachometer_abs_value = buffer_get_int32(data, &i);
  }
  if (mask & ((uint32_t)1 << 15)) {
    values_data.fault = buffer_get_int8(data, &i);
  }
  if (mask & ((uint32_t)1 << 16)) {
    values_data.pid_pos = buffer_get_float32(data, 1000000.0, &i);
  }
  if (mask & ((uint32_t)1 << 17)) {
    values_data.controller_id = buffer_get_int8(data, &i);
  }
  if (mask & ((uint32_t)1 << 18)) {
    // values_data.ntc_temp_mos1 = buffer_get_float16(data, 10.0, &i);
    // values_data.ntc_temp_mos2 = buffer_get_float16(data, 10.0, &i);
    // values_data.ntc_temp_mos3 = buffer_get_float16(data, 10.0, &i);
    buffer_get_float16(data, 10.0, &i);
    buffer_get_float16(data, 10.0, &i);
    buffer_get_float16(data, 10.0, &i);
  }
 
  if (getValuesCB) getValuesCB(this);
}

void VescUartApi::consumePacket(const uint8_t *packet, uint16_t packetsize)
{
  COMM_PACKET_ID packetType = (COMM_PACKET_ID)packet[0];
  packet++;
  packetsize--;
  switch(packetType)
  {
    case COMM_GET_VALUES:
    case COMM_GET_VALUES_SELECTIVE:
	rcvd_GET_VALUES(packet, packetsize, packetType==COMM_GET_VALUES_SELECTIVE);
      break;

    case COMM_FW_VERSION:
        rcvd_FW_VERSION(packet, packetsize);
      break;
    default:
      // default A.K.A. unsupported:
      // note, expect only COMM_GET_... packets here (those who have answer)
      // COMM_GET_MCCONF,
      // COMM_GET_MCCONF_DEFAULT,
      // COMM_GET_APPCONF,
      // COMM_GET_APPCONF_DEFAULT,
      // COMM_GET_DECODED_PPM,
      // COMM_GET_DECODED_ADC,
      // COMM_GET_DECODED_CHUK,
      break;
  }
  
  
}

void VescUartApi::askValues()
{
  uint8_t buf[7];
  buf[3] = COMM_GET_VALUES;
  sendCommandInplace(buf, 1);
}

void VescUartApi::askFwVersion()
{
  uint8_t buf[7];
  buf[3] = COMM_FW_VERSION;
  sendCommandInplace(buf, 1);
}

void VescUartApi::setCurrent(int32_t miliamps)
{
  int32_t index = 3;
  uint8_t buf[11];

  buf[index++] = COMM_SET_CURRENT;
  buffer_append_int32(buf, miliamps, &index);
  sendCommandInplace(buf, 5);
}

void VescUartApi::setCurrentBrake(int32_t miliamps)
{
  int32_t index = 3;
  uint8_t buf[11];

  buf[index++] = COMM_SET_CURRENT_BRAKE;
  buffer_append_int32(buf, miliamps, &index);
  sendCommandInplace(buf, 5);
}

// -1 .. 1 mapped to -100 000 .. 100 000
void VescUartApi::setDuty(int32_t duty)
{
  int32_t index = 3;
  uint8_t buf[11];

  buf[index++] = COMM_SET_DUTY;
  buffer_append_int32(buf, duty, &index);
  sendCommandInplace(buf, 5);
}

void VescUartApi::setRPM(int32_t rpm)
{
  int32_t index = 3;
  uint8_t buf[11];

  buf[index++] = COMM_SET_RPM;
  buffer_append_int32(buf, rpm, &index);
  sendCommandInplace(buf, 5);
}


int16_t VescUartApi::sendCommand(uint8_t *cmd, int16_t cmdlen)
{
	uint8_t packet[256];
	int16_t packetlen = 0;
	uint16_t crc = crc16(cmd, cmdlen);

	if (cmdlen <= 256)
	{
		packet[0] = 2;
		packet[1] = cmdlen;
		packetlen = 2;
	}
	else
	{
		// FIXME: not supported
		packet[0] = 3;
		packet[1] = (uint8_t)(cmdlen >> 8);
		packet[2] = (uint8_t)(cmdlen & 0xFF);
		packetlen = 3;
	}
	memcpy(packet+packetlen, cmd, cmdlen);

	packetlen += cmdlen;
	packet[packetlen++] = (uint8_t)(crc >> 8);
	packet[packetlen++] = (uint8_t)(crc & 0xFF);
	packet[packetlen++] = 3;

	uart->write(packet, packetlen);

	return packetlen;
}

int16_t VescUartApi::sendCommandInplace(uint8_t *buf, int16_t cmdlen)
{
	// buf has 3 bytes free before payload and 3 bytes free after payload, command payload starts on 4th byte
        // we can use that and not allocate another big array

	uint8_t *packet = buf;
	int16_t packetlen = 0;
	uint16_t crc = crc16(buf+3, cmdlen);

	if (cmdlen <= 256)
	{
		packet = buf+1;
		packet[0] = 2;
		packet[1] = cmdlen;
		packetlen = 2;
	}
	else
	{
		packet = buf;
		packet[0] = 3;
		packet[1] = (uint8_t)(cmdlen >> 8);
		packet[2] = (uint8_t)(cmdlen & 0xFF);
		packetlen = 3;
	}

	packetlen += cmdlen;
	packet[packetlen++] = (uint8_t)(crc >> 8);
	packet[packetlen++] = (uint8_t)(crc & 0xFF);
	packet[packetlen++] = 3;

	uart->write(packet, packetlen);

	return packetlen;
}

void VescUartApi::setRxDataCB(COMM_PACKET_ID packet_id, void(*cb)(VescUartApi *))
{
  switch(packet_id)
  {
    case COMM_GET_VALUES:
	getValuesCB=cb;
      break;

    default:
      // callback not supported (yet?)
      break;
  }
}
