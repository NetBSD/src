/* CVS client logging buffer.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.  */


#ifndef MS_BUFFER_H__
# define MS_BUFFER_H__
# ifdef PROXY_SUPPORT

struct buffer *
ms_buffer_initialize (void (*memory) (struct buffer *),
		      struct buffer *buf, struct buffer *buf2);
# endif /* PROXY_SUPPORT */
#endif /* MS_BUFFER_H__ */
