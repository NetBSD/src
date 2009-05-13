/* CVS socket client stuff.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.  */

#ifndef SOCKET_CLIENT_H__
#define SOCKET_CLIENT_H__ 1

#if defined SOCK_ERRNO || defined SOCK_STRERROR
# include <sys/socket.h>
# include <netinet/in.h>
# include <arpa/inet.h>
# include <netdb.h>
#endif

struct buffer *socket_buffer_initialize
  (int, int, void (*) (struct buffer *));

/* If SOCK_ERRNO is defined, then send()/recv() and other socket calls
   do not set errno, but that this macro should be used to obtain an
   error code.  This probably doesn't make sense unless
   NO_SOCKET_TO_FD is also defined. */
#ifndef SOCK_ERRNO
# define SOCK_ERRNO errno
#endif

/* If SOCK_STRERROR is defined, then the error codes returned by
   socket operations are not known to strerror, and this macro must be
   used instead to convert those error codes to strings. */
#ifndef SOCK_STRERROR
# define SOCK_STRERROR strerror

# include <string.h>
# ifndef strerror
extern char *strerror (int);
# endif
#endif /* ! SOCK_STRERROR */

#endif /* SOCKET_CLIENT_H__ */
