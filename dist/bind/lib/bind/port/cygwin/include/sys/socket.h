/*	$NetBSD: socket.h,v 1.1.1.1 2004/05/17 23:44:46 christos Exp $	*/

#ifndef _cygwin_sys_socket_h
#define _cygwin_sys_socket_h

#include_next <sys/socket.h>

#ifndef IFF_POINTOPOINT
# define IFF_POINTOPOINT 0x10
#endif

#endif
