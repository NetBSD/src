/*
   ether.h - ethernet definitions for systems lacking those

   Copyright (C) 2008 Arthur de Jong

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301 USA
*/

#ifndef _COMPAT_ETHER_H
#define _COMPAT_ETHER_H 1

#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#ifdef HAVE_NETINET_ETHER_H
#include <netinet/ether.h>
#endif

#ifndef HAVE_STRUCT_ETHER_ADDR
struct ether_addr {
  uint8_t ether_addr_octet[6];
};
#endif /* not HAVE_STRUCT_ETHER_ADDR */

#ifndef HAVE_ETHER_NTOA_R
char *ether_ntoa_r(const struct ether_addr *addr,char *buf);
#endif /* not HAVE_ETHER_NTOA_R */

#ifndef HAVE_ETHER_ATON_R
struct ether_addr *ether_aton_r(const char *asc,struct ether_addr *addr);
#endif /* not HAVE_ETHER_ATON_R */

#endif /* not _COMPAT_ETHER_H */

