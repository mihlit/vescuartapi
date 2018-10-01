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
#include <cstring>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include "linux_hwserial.h"

#include <time.h>

int HardwareSerial::begin(int baud)
{
  speed_t speed;
  switch(baud)
  {
    case 50:     speed = B50;     break;
    case 75:     speed = B75;     break;
    case 110:    speed = B110;    break;
    case 134:    speed = B134;    break;
    case 150:    speed = B150;    break;
    case 200:    speed = B200;    break;
    case 300:    speed = B300;    break;
    case 600:    speed = B600;    break;
    case 1200:   speed = B1200;   break;
    case 1800:   speed = B1800;   break;
    case 2400:   speed = B2400;   break;
    case 4800:   speed = B4800;   break;
    case 9600:   speed = B9600;   break;
    case 19200:  speed = B19200;  break;
    case 38400:  speed = B38400;  break;
    case 57600:  speed = B57600;  break;
    case 115200: speed = B115200; break;
    case 230400: speed = B230400; break;
    default:
      printf("error: Can't translate baud rate to standard speed");
      return -EINVAL;
  }
    
  fd = open((char*)buf.buf, O_RDWR | O_NOCTTY | O_NONBLOCK | O_CLOEXEC); // | O_NDELAY);
  if(fd == -1) // if open is unsucessful
  {
    int saveerr = errno;
    printf("open_port: Unable to open %s : %m\n", buf.buf);
    return -saveerr;
  }
  struct termios serial_config;      // structure to store the port settings in

  //Configure port for 8N1 transmission
  tcgetattr(fd, &serial_config);	//Gets the current options for the port
  cfmakeraw(&serial_config);

  cfsetispeed(&serial_config, speed);	// set baud rates
  cfsetospeed(&serial_config, speed);

  serial_config.c_cflag &= ~PARENB;	// set no parity, stop bits, data bits
  serial_config.c_cflag &= ~CSTOPB;	// 1 stop bit

  serial_config.c_cflag &= ~CSIZE;	// clear size bits
  serial_config.c_cflag |= CS8;		// set 8bit size bit

  serial_config.c_cc[VMIN] = 0;		// block read until at least 0 bytes read (aka return immediately if nothing to read, dont block)
  serial_config.c_cc[VTIME] = 0;	// wait N decicesonds for data if nothing to read

  serial_config.c_cflag &= ~CRTSCTS;	// disabhle hw flow control
  serial_config.c_cflag |= CREAD | CLOCAL;	//enable receiver, ignore modem control lines
  serial_config.c_iflag &= ~(IXON | IXOFF | IXANY);	// disable flow control on input, output and no restart stopped output
  serial_config.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); //non-canonical, no echo, no backspace echo, don't react to interrupt characters
  serial_config.c_oflag &= ~OPOST;	// disable output processing

  tcsetattr(fd, TCSANOW, &serial_config); 
  // drop all data in IO buffers wating to be read/sent
  tcflush(fd, TCIOFLUSH);
  return fd;
}

void savePacket(bool in, const uint8_t *buf, int size)
{
  return;
  struct timespec now;
  clock_gettime(CLOCK_BOOTTIME, &now);
  char fpath[256];
  snprintf(fpath, 250, "vua.%s.%ld.%06ld", (in?"in":"out"), now.tv_sec, now.tv_nsec/1000);
  int fd=open(fpath, O_CREAT|O_WRONLY|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
  write(fd, buf, size);
  close(fd);
}

int HardwareSerial::available()
{
  uint8_t rxbuf[100];
  int maxlen;
  int got;
  while((maxlen=buf.freeSpace()))
  {
    if (maxlen > (int)sizeof(rxbuf)) maxlen = sizeof(rxbuf);
    got = ::read(fd, rxbuf, maxlen);
    if (got > 0)
    {
      savePacket(true, rxbuf, got);
      buf.store(rxbuf, got);
    }
    else break;
  }
  return buf.length();
}

uint8_t HardwareSerial::read()
{
  return buf.pop();
}

void HardwareSerial::write(const uint8_t *buf, int len)
{
  savePacket(false, buf, len);
  while(::write(fd, buf, len) == -1 && errno==EINTR) {}
}

