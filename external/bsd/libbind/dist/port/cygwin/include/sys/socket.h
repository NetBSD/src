/*	$NetBSD: socket.h,v 1.1.1.1.6.2 2011/01/09 20:43:05 riz Exp $	*/

#ifndef _cygwin_sys_socket_h
#define _cygwin_sys_socket_h

#include_next <sys/socket.h>

#ifndef IFF_POINTOPOINT
# define IFF_POINTOPOINT 0x10
#endif

#endif
