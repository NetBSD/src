/*	$NetBSD: socket.h,v 1.1.1.1.2.2 2009/05/13 18:52:20 jym Exp $	*/

#ifndef _cygwin_sys_socket_h
#define _cygwin_sys_socket_h

#include_next <sys/socket.h>

#ifndef IFF_POINTOPOINT
# define IFF_POINTOPOINT 0x10
#endif

#endif
