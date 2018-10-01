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

#include "ringbuffer.h"

void RingBuffer::push(uint8_t b)
{
    if (!freeSpace())
    {
        //should not happen, not supported
        // reset buffer to prevent deadlock
        start = len = 0;
    }
    int16_t endchunksize = bufsize-(start+len);
    if (1 <= endchunksize)
    {
        buf[start+len] = b;
        len++;
    }
    else
    {
        //negative endchunksize says how much abs(...) bytes at the beggining of the buffer is used, so shift '-endchunksize'
        *(buf-endchunksize) = b;
        len++;
    }
}

uint8_t RingBuffer::pop()
{
    uint8_t ret;
    if (len)
    {
        ret = buf[start];
        len--;
        start++;
        if (start >= bufsize) start=0;
        return ret;
    }
    else
    {
        return -1;
    }
}

