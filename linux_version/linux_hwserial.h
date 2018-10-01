#ifndef _LINUX_HWSERIAL_H_
#define _LINUX_HWSERIAL_H_

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

#include <cstdint>
#include <cstring>
#include <unistd.h>
#include "ringbuffer.h"


/* NOTE: do not use it for different projects, this is not complete drop in replacement
   for Arduino's HardwareSerial. This is bare minimum to allow same API and it implements
   only what this project needs. Also its not guaranteed that even those things, this class
   does implement is 100 % compatible with the original.
*/

class HardwareSerial {
  RingBuffer buf;
  int fd;
public:
  HardwareSerial(const char *path) : buf(256), fd(-1)
  {
      /* before we open port, use buffer for storing port path */
      strncpy((char*)buf.buf, path, sizeof(buf.buf));
  }

  ~HardwareSerial()
  {
      if (fd>=0) close(fd);
  }

  int begin(int baud);
  int available();
  uint8_t read();
  void write(const uint8_t *buf, int len);
};

#endif /* _LINUX_HWSERIAL_H_ */

