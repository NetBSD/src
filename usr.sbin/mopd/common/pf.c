/*	$NetBSD: pf.c,v 1.13 2016/06/08 01:11:49 christos Exp $	*/

/*
 * Copyright (c) 1993-95 Mats O Jansson.  All rights reserved.
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is partly derived from rarpd.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "port.h"
#ifndef lint
__RCSID("$NetBSD: pf.c,v 1.13 2016/06/08 01:11:49 christos Exp $");
#endif

#include "os.h"

#include <paths.h>
#include <sys/uio.h>
#include <net/bpf.h>

#include "mopdef.h"
#include "pf.h"
#include "log.h"

/*
 * Variables
 */

extern int promisc;

/*
 * Return information to device.c how to open device.
 * In this case the driver can handle both Ethernet type II and
 * IEEE 802.3 frames (SNAP) in a single pfOpen.
 */

int
pfTrans(const char *interface)
{
	return TRANS_ETHER+TRANS_8023+TRANS_AND;
}

/*
 * Open and initialize packet filter.
 */

int
pfInit(const char *interface, int mode, u_short protocol, int typ)
{
	int	fd;
	struct ifreq ifr;
	u_int	dlt;
	int	immediate;
	u_int	bufsize;
	const char *device = _PATH_BPF;

	static struct bpf_insn insns[] = {
		BPF_STMT(BPF_LD | BPF_H | BPF_ABS, 12),
		BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, 0x4711, 4, 0),
		BPF_STMT(BPF_LD | BPF_H | BPF_ABS, 20),
		BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, 0x4711, 0, 3),
		BPF_STMT(BPF_LD | BPF_H | BPF_ABS, 14),
		BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, 0xaaaa, 0, 1),
		BPF_STMT(BPF_RET | BPF_K, 1520),
		BPF_STMT(BPF_RET | BPF_K, 0),
	};
	static struct bpf_program filter = {
		sizeof insns / sizeof(insns[0]),
		insns
	};
	
	fd = open(device, mode);
	if (fd < 0) {
      		mopLogWarn("pfInit: open %s", device);
		return(-1);
	}
  
	/* Set immediate mode so packets are processed as they arrive. */
	immediate = 1;
	if (ioctl(fd, BIOCIMMEDIATE, &immediate) < 0) {
      		mopLogWarn("pfInit: BIOCIMMEDIATE");
		return(-1);
	}
	bufsize = 32768;
	if (ioctl(fd, BIOCSBLEN, &bufsize) < 0) {
      		mopLogWarn("pfInit: BIOCSBLEN(%d)", bufsize);
	}
	(void) strncpy(ifr.ifr_name, interface, sizeof ifr.ifr_name);
	if (ioctl(fd, BIOCSETIF, (caddr_t) & ifr) < 0) {
      		mopLogWarn("pfInit: BIOCSETIF");
		return(-1);
	}
	/* Check that the data link layer is an Ethernet; this code won't work
	 * with anything else. */
	if (ioctl(fd, BIOCGDLT, (caddr_t) & dlt) < 0) {
      		mopLogWarn("pfInit: BIOCGDLT");
		return(-1);
	}
	if (dlt != DLT_EN10MB) {
      		mopLogWarnX("pfInit: %s is not ethernet", device);
		return(-1);
	}
	if (promisc) {
		/* Set promiscuous mode. */
		if (ioctl(fd, BIOCPROMISC, (caddr_t)0) < 0) {
      			mopLogWarn("pfInit: BIOCPROMISC");
			return(-1);
		}
	}
	/* Set filter program. */
	insns[1].k = protocol;
	insns[3].k = protocol;

	if (ioctl(fd, BIOCSETF, (caddr_t) & filter) < 0) {
      		mopLogWarn("pfInit: BIOCSETF");
		return(-1);
	}
	return(fd);
}

/*
 * Add a Multicast address to the interface
 */

int
pfAddMulti(int s, const char *interface, const char *addr)
{
	struct ifreq ifr;
	int	fd;
	
	strncpy(ifr.ifr_name, interface, sizeof(ifr.ifr_name));

	ifr.ifr_addr.sa_family = AF_UNSPEC;
	memmove(ifr.ifr_addr.sa_data, addr, 6);
	
	/*
	 * open a socket, temporarily, to use for SIOC* ioctls
	 *
	 */
	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		mopLogWarn("pfAddMulti: socket");
		return(-1);
	}
	if (ioctl(fd, SIOCADDMULTI, (caddr_t)&ifr) < 0) {
		mopLogWarn("pfAddMulti: SIOCADDMULTI");
		close(fd);
		return(-1);
	}
	close(fd);
	
	return(0);
}

/*
 * Delete a Multicast address from the interface
 */

int
pfDelMulti(int s, const char *interface, const char *addr)
{
	struct ifreq ifr;
	int	fd;
	
	strncpy(ifr.ifr_name, interface, sizeof(ifr.ifr_name));
	
	ifr.ifr_addr.sa_family = AF_UNSPEC;
	memmove(ifr.ifr_addr.sa_data, addr, 6);
	
	/*
	 * open a socket, temporarily, to use for SIOC* ioctls
	 *
	 */
	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		mopLogWarn("pfDelMulti: socket");
		return(-1);
	}
	if (ioctl(fd, SIOCDELMULTI, (caddr_t)&ifr) < 0) {
		mopLogWarn("pfAddMulti: SIOCDELMULTI");
		close(fd);
		return(-1);
	}
	close(fd);
	
	return(0);
}

/*
 * read a packet
 */

int
pfRead(int fd, u_char *buf, int len)
{
	return(read(fd, buf, len));
}

/*
 * write a packet
 */

int
pfWrite(int fd, const u_char *buf, int len, int trans)
{
	
	struct iovec iov[2];
	
	switch (trans) {
	case TRANS_8023:
		iov[0].iov_base = (caddr_t)__UNCONST(buf);
		iov[0].iov_len = 22;
		iov[1].iov_base = (caddr_t)__UNCONST(buf+22);
		iov[1].iov_len = len-22;
		break;
	default:
		iov[0].iov_base = (caddr_t)__UNCONST(buf);
		iov[0].iov_len = 14;
		iov[1].iov_base = (caddr_t)__UNCONST(buf+14);
		iov[1].iov_len = len-14;
		break;
	}

	if (writev(fd, iov, 2) == len)
		return(len);
	
	return(-1);
}

