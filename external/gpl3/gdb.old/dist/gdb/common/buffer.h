/* A simple growing buffer for GDB.
  
   Copyright (C) 2009-2017 Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#ifndef BUFFER_H
#define BUFFER_H

struct buffer
{
  char *buffer;
  size_t buffer_size; /* allocated size */
  size_t used_size; /* actually used size */
};

/* Append DATA of size SIZE to the end of BUFFER.  Grows the buffer to
   accommodate the new data.  */
void buffer_grow (struct buffer *buffer, const char *data, size_t size);

/* Append C to the end of BUFFER.  Grows the buffer to accommodate the
   new data.  */

static inline void
buffer_grow_char (struct buffer *buffer, char c)
{
  buffer_grow (buffer, &c, 1);
}

/* Release any memory held by BUFFER.  */
void buffer_free (struct buffer *buffer);

/* Initialize BUFFER.  BUFFER holds no memory afterwards.  */
void buffer_init (struct buffer *buffer);

/* Return a pointer into BUFFER data, effectivelly transfering
   ownership of the buffer memory to the caller.  Calling buffer_free
   afterwards has no effect on the returned data.  */
char* buffer_finish (struct buffer *buffer);

/* Simple printf to buffer function.  Current implemented formatters:
   %s - grow an xml escaped text in BUFFER.
   %d - grow an signed integer in BUFFER.
   %u - grow an unsigned integer in BUFFER.
   %x - grow an unsigned integer formatted in hexadecimal in BUFFER.
   %o - grow an unsigned integer formatted in octal in BUFFER.  */
void buffer_xml_printf (struct buffer *buffer, const char *format, ...)
  ATTRIBUTE_PRINTF (2, 3);

#define buffer_grow_str(BUFFER,STRING)		\
  buffer_grow (BUFFER, STRING, strlen (STRING))
#define buffer_grow_str0(BUFFER,STRING)			\
  buffer_grow (BUFFER, STRING, strlen (STRING) + 1)

#endif
