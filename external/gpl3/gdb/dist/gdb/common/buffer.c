/* A simple growing buffer for GDB.
  
   Copyright (C) 2009-2013 Free Software Foundation, Inc.

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

#ifdef GDBSERVER
#include "server.h"
#else
#include "defs.h"
#endif

#include "xml-utils.h"
#include "buffer.h"
#include "inttypes.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

void
buffer_grow (struct buffer *buffer, const char *data, size_t size)
{
  char *new_buffer;
  size_t new_buffer_size;

  if (size == 0)
    return;

  new_buffer_size = buffer->buffer_size;

  if (new_buffer_size == 0)
    new_buffer_size = 1;

  while (buffer->used_size + size > new_buffer_size)
    new_buffer_size *= 2;
  new_buffer = xrealloc (buffer->buffer, new_buffer_size);
  memcpy (new_buffer + buffer->used_size, data, size);
  buffer->buffer = new_buffer;
  buffer->buffer_size = new_buffer_size;
  buffer->used_size += size;
}

void
buffer_free (struct buffer *buffer)
{
  if (!buffer)
    return;

  xfree (buffer->buffer);
  buffer->buffer = NULL;
  buffer->buffer_size = 0;
  buffer->used_size = 0;
}

void
buffer_init (struct buffer *buffer)
{
  memset (buffer, 0, sizeof (*buffer));
}

char*
buffer_finish (struct buffer *buffer)
{
  char *ret = buffer->buffer;
  buffer->buffer = NULL;
  buffer->buffer_size = 0;
  buffer->used_size = 0;
  return ret;
}

void
buffer_xml_printf (struct buffer *buffer, const char *format, ...)
{
  va_list ap;
  const char *f;
  const char *prev;
  int percent = 0;

  va_start (ap, format);

  prev = format;
  for (f = format; *f; f++)
    {
      if (percent)
	{
	  char buf[32];
	  char *p;
	  char *str = buf;
	  const char *f_old = f;
	  
	  switch (*f)
	    {
	    case 's':
	      str = va_arg (ap, char *);
	      break;
	    case 'd':
	      sprintf (str, "%d", va_arg (ap, int));
	      break;
	    case 'u':
	      sprintf (str, "%u", va_arg (ap, unsigned int));
	      break;
	    case 'x':
	      sprintf (str, "%x", va_arg (ap, unsigned int));
	      break;
	    case 'o':
	      sprintf (str, "%o", va_arg (ap, unsigned int));
	      break;
	    case 'l':
	      f++;
	      switch (*f)
		{
		case 'd':
		  sprintf (str, "%ld", va_arg (ap, long));
		  break;
		case 'u':
		  sprintf (str, "%lu", va_arg (ap, unsigned long));
		  break;
		case 'x':
		  sprintf (str, "%lx", va_arg (ap, unsigned long));
		  break;
		case 'o':
		  sprintf (str, "%lo", va_arg (ap, unsigned long));
		  break;
		case 'l':
		  f++;
		  switch (*f)
		    {
		    case 'd':
		      sprintf (str, "%" PRId64,
			       (int64_t) va_arg (ap, long long));
		      break;
		    case 'u':
		      sprintf (str, "%" PRIu64,
			       (uint64_t) va_arg (ap, unsigned long long));
		      break;
		    case 'x':
		      sprintf (str, "%" PRIx64,
			       (uint64_t) va_arg (ap, unsigned long long));
		      break;
		    case 'o':
		      sprintf (str, "%" PRIo64,
			       (uint64_t) va_arg (ap, unsigned long long));
		      break;
		    default:
		      str = 0;
		      break;
		    }
		  break;
		default:
		  str = 0;
		  break;
		}
	      break;
	    default:
	      str = 0;
	      break;
	    }

	  if (str)
	    {
	      buffer_grow (buffer, prev, f_old - prev - 1);
	      p = xml_escape_text (str);
	      buffer_grow_str (buffer, p);
	      xfree (p);
	      prev = f + 1;
	    }
	  percent = 0;
	}
      else if (*f == '%')
	percent = 1;
    }

  buffer_grow_str (buffer, prev);
  va_end (ap);
}

