/*	$NetBSD: linux_sockio.h,v 1.18.12.1 2013/01/23 00:06:02 yamt Exp $	*/

/*-
 * Copyright (c) 1995, 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Frank van der Linden and Eric Haszlakiewicz.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _LINUX_SOCKIO_H
#define _LINUX_SOCKIO_H

#define	LINUX_SIOCGIFNAME	_LINUX_IO(0x89, 0x10)
#define	LINUX_SIOCGIFCONF	_LINUX_IO(0x89, 0x12)
#define	LINUX_SIOCGIFFLAGS	_LINUX_IO(0x89, 0x13)
#define	LINUX_SIOCSIFFLAGS	_LINUX_IO(0x89, 0x14)
#define	LINUX_SIOCGIFADDR	_LINUX_IO(0x89, 0x15)
#define	LINUX_SIOCSIFADDR	_LINUX_IO(0x89, 0x16)
#define	LINUX_SIOCGIFDSTADDR	_LINUX_IO(0x89, 0x17)
#define	LINUX_SIOCGIFBRDADDR	_LINUX_IO(0x89, 0x19)
#define	LINUX_SIOCGIFNETMASK	_LINUX_IO(0x89, 0x1b)
#define	LINUX_SIOCGIFMTU	_LINUX_IO(0x89, 0x21)
#define LINUX_SIOCADDMULTI	_LINUX_IO(0x89, 0x31)
#define LINUX_SIOCDELMULTI	_LINUX_IO(0x89, 0x32)
#define LINUX_SIOCGIFHWADDR	_LINUX_IO(0x89, 0x27)
#define LINUX_SIOCDEVPRIVATE	_LINUX_IO(0x89, 0xf0)
#define LINUX_SIOCGIFBR		_LINUX_IO(0x89, 0x40)
#define LINUX_SIOCSIFBR		_LINUX_IO(0x89, 0x41)

#define LINUX_IFNAMSIZ	16

struct linux_ifmap {
	unsigned long mem_start;
	unsigned long mem_end;
	unsigned short base_addr; 
	unsigned char irq;
	unsigned char dma;
	unsigned char port;
};

struct linux_ifreq {
	union {
		char ifrn_name[LINUX_IFNAMSIZ];	/* if name, e.g. "en0" */
	} ifr_ifrn;
	union {
		struct osockaddr ifru_addr;
		struct osockaddr ifru_hwaddr;
		struct linux_ifmap ifru_map;
		int ifru_ifindex;
	} ifr_ifru;
#define ifr_name	ifr_ifrn.ifrn_name	/* interface name       */
#define ifr_addr	ifr_ifru.ifru_addr	/* address              */
#define ifr_hwaddr	ifr_ifru.ifru_hwaddr	/* MAC address          */
#define ifr_map		ifr_ifru.ifru_map	/* device map           */
};

struct linux_ifconf {
        int ifc_len;
	union {
		char *ifcu_buf;
		struct linux_ifreq *ifcu_req;
	} ifc_ifcu;
};

#define ifc_buf ifc_ifcu.ifcu_buf
#define ifc_req ifc_ifcu.ifcu_req

#endif /* !_LINUX_SOCKIO_H */
