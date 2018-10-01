#ifndef _RINGBUFFER_H_
#define _RINGBUFFER_H_

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

#include <string.h>
#include <stdint.h>
#include <stdlib.h>

class RingBuffer {
public:
  uint8_t *buf;
  int16_t bufsize;
  int16_t start;
  int16_t len;

  RingBuffer(uint8_t *buf, int16_t bufsize) : buf(buf), bufsize(bufsize), start(0), len(0) { }

  RingBuffer(int16_t bufsize) : buf(NULL), bufsize(bufsize), start(0), len(0)
  {
      buf = (uint8_t *)malloc(bufsize);
  }

  void store(const uint8_t *data, int16_t datasize)
  {
    if (datasize>freeSpace())
    {
        //should not happen, not supported
        // reset buffer to prevent deadlock
        start = len = 0;
    }
    int16_t endchunksize = bufsize-(start+len);
    if (datasize<=endchunksize)
    {
      memcpy(buf+start+len, data, datasize);
      len += datasize;
      return;
    }
    else
    {
      if (endchunksize > 0)
      {
          memcpy(buf+start+len, data, endchunksize);
          data += endchunksize;
          datasize -= endchunksize;
          len += endchunksize;
          endchunksize = 0;
      }
      //negative endchunksize says how much abs(...) bytes at the beggining of the buffer is used, so shift '-endchunksize'
      memcpy(buf-endchunksize/*endchunksize is negative*/, data, datasize);
      len += datasize;
    }
  }
  inline int16_t length() { return len; }
  inline int16_t freeSpace() { return bufsize-len-1; }
  uint8_t pop();
  void push(uint8_t);

};

#endif
