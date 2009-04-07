/* CVS client logging buffer.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.  */


#ifndef LOG_BUFFER_H__
#define LOG_BUFFER_H__

void setup_logfiles (char *var, struct buffer** to_server_p,
                     struct buffer** from_server_p);

struct buffer *
log_buffer_initialize (struct buffer *buf, FILE *fp,
# ifdef PROXY_SUPPORT
		       bool fatal_errors,
		       size_t max,
# endif /* PROXY_SUPPORT */
                       bool input,
		       void (*memory) (struct buffer *));

# ifdef PROXY_SUPPORT
struct buffer *log_buffer_rewind (struct buffer *buf);
void log_buffer_closelog (struct buffer *buf);
# endif /* PROXY_SUPPORT */

#endif /* LOG_BUFFER_H__ */
