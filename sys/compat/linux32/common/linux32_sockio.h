/* $NetBSD: linux32_sockio.h,v 1.3.12.1 2013/01/23 00:06:02 yamt Exp $ */

/*
 * Copyright (c) 2008 Nicolas Joly
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS
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

#ifndef _LINUX32_SOCKIO_H
#define _LINUX32_SOCKIO_H

#define	LINUX32_IFNAMSIZ	16

struct linux32_ifmap {
	netbsd32_u_long mem_start;
	netbsd32_u_long mem_end;
	unsigned short base_addr;
	unsigned char irq;
	unsigned char dma;
	unsigned char port;
}; 

struct linux32_ifreq {
	union {
		char ifrn_name[LINUX32_IFNAMSIZ];
	} ifr_ifrn;
	union {
		struct osockaddr ifru_addr;
		struct osockaddr ifru_hwaddr;
		struct linux32_ifmap ifru_map;
		int ifru_ifindex;
	} ifr_ifru;
#define ifr_name	ifr_ifrn.ifrn_name	/* interface name       */
#define ifr_addr	ifr_ifru.ifru_addr	/* address              */
#define ifr_hwaddr	ifr_ifru.ifru_hwaddr	/* MAC address          */
#define ifr_map		ifr_ifru.ifru_map	/* device map           */
};

struct linux32_ifconf {
        int ifc_len;
	union {
		netbsd32_caddr_t ifcu_buf;      
		netbsd32_ifreq_tp_t ifcu_req;   
	} ifc_ifcu;
};

#define ifc_buf ifc_ifcu.ifcu_buf
#define ifc_req ifc_ifcu.ifcu_req

#endif /* !_LINUX32_SOCKIO_H */
