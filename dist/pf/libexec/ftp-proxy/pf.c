/*	$NetBSD: pf.c,v 1.1 2004/06/30 13:29:43 darrenr Exp $	*/
/*	$OpenBSD: util.c,v 1.18 2004/01/22 16:10:30 beck Exp $ */

/*
 * Copyright (c) 1996-2001
 *	Obtuse Systems Corporation.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Obtuse Systems nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE OBTUSE SYSTEMS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL OBTUSE
 * SYSTEMS CORPORATION OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <net/if.h>
#include <net/pfvar.h>

#include <arpa/inet.h>

#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <syslog.h>
#include <unistd.h>

#include "util.h"


int
pf_get_proxy_env(int connected_fd, struct sockaddr_in *real_server_sa_ptr,
    struct sockaddr_in *client_sa_ptr)
{
	struct pfioc_natlook natlook;
	socklen_t slen;
	int fd;

	slen = sizeof(*real_server_sa_ptr);
	if (getsockname(connected_fd, (struct sockaddr *)real_server_sa_ptr,
	    &slen) != 0) {
		syslog(LOG_ERR, "getsockname() failed (%m)");
		return(-1);
	}
	slen = sizeof(*client_sa_ptr);
	if (getpeername(connected_fd, (struct sockaddr *)client_sa_ptr,
	    &slen) != 0) {
		syslog(LOG_ERR, "getpeername() failed (%m)");
		return(-1);
	}

	/*
	 * Build up the pf natlook structure.
	 * Just for IPv4 right now
	 */
	memset((void *)&natlook, 0, sizeof(natlook));
	natlook.af = AF_INET;
	natlook.saddr.addr32[0] = client_sa_ptr->sin_addr.s_addr;
	natlook.daddr.addr32[0] = real_server_sa_ptr->sin_addr.s_addr;
	natlook.proto = IPPROTO_TCP;
	natlook.sport = client_sa_ptr->sin_port;
	natlook.dport = real_server_sa_ptr->sin_port;
	natlook.direction = PF_OUT;

	/*
	 * Open the pf device and lookup the mapping pair to find
	 * the original address we were supposed to connect to.
	 */
	fd = open("/dev/pf", O_RDWR);
	if (fd == -1) {
		syslog(LOG_ERR, "cannot open /dev/pf (%m)");
		exit(EX_UNAVAILABLE);
	}

	if (ioctl(fd, DIOCNATLOOK, &natlook) == -1) {
		syslog(LOG_INFO,
		    "pf nat lookup failed %s:%hu (%m)",
		    inet_ntoa(client_sa_ptr->sin_addr),
		    ntohs(client_sa_ptr->sin_port));
		close(fd);
		return(-1);
	}
	close(fd);

	/*
	 * Now jam the original address and port back into the into
	 * destination sockaddr_in for the proxy to deal with.
	 */
	memset((void *)real_server_sa_ptr, 0, sizeof(struct sockaddr_in));
	real_server_sa_ptr->sin_port = natlook.rdport;
	real_server_sa_ptr->sin_addr.s_addr = natlook.rdaddr.addr32[0];
	real_server_sa_ptr->sin_len = sizeof(struct sockaddr_in);
	real_server_sa_ptr->sin_family = AF_INET;
	return(0);
}
