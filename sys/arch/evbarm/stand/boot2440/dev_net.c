/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Fleischer <paul@xpg.dk>
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
#include <sys/param.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>

#include <lib/libsa/stand.h>
#include <lib/libsa/net.h>
#include <lib/libsa/bootp.h>
#include <lib/libsa/nfs.h>

#include <lib/libkern/libkern.h>


extern int netif_open(void*);
extern int netif_init(unsigned int tag);

extern struct in_addr servip;

static int netdev_sock = -1;

int
net_open(struct open_file *f, ...)
{
	va_list ap;
	char *path, *proto;
	int error = 0;

	if( netif_init(0) == 0 ) {
		error = ENODEV;
		goto out;
	}

	va_start(ap, f);
	path = va_arg(ap, char *);
	proto = va_arg(ap, char *);
	va_end(ap);
	if ((netdev_sock = netif_open(path)) < 0) {
		error = errno;
		goto out;
	}

	/* send DHCP request */
	bootp(netdev_sock);

	/* IP address was not found */
	if (myip.s_addr == 0) {
		error = ENOENT;
		goto out;
	}

	if (path[0] != '\0') {
		char *c;
		char ip_addr[200];
		/* Parse path specification as IP-ADDR:PATH and overwrite
		   rootip and rootpath with those
		 */

		for(c=path; *c != '\0' && *c != ':'; c++);

		if (*c == ':') {
			char *filename;
			strncpy(ip_addr, path, (c-path));
			ip_addr[(c-path)] = '\0';
			printf("IP addr: %s\n", ip_addr);
			rootip.s_addr = inet_addr(ip_addr);

			c++;
			/* Finally, strip away file-component and copy it to
			   bootfile */
			for(filename = c+strlen(c); *filename != '/' && filename > c; filename--);
			strncpy(rootpath, c, (filename-c));
			rootpath[(filename-c)] = '\0';
			printf("Root path: %s\n", rootpath);
			strcpy(bootfile, ++filename);
			printf("Bootfile: %s\n", bootfile);
		} else {
			/* No ':' found, assume it's just a filename */
			strcpy(bootfile, path);
		}
	}

	/* boot filename was not found */
	if (bootfile[0] == '\0')
		strcpy(bootfile, "netbsd");

	if (strcmp(proto, "nfs") == 0
	    && (nfs_mount(netdev_sock, rootip, rootpath) != 0)) {
		error = errno;
		goto out;
	}

	f->f_devdata = &netdev_sock;
out:
	return (error);
}

int
net_close(struct open_file *f)

{
	return (0);
}

int
net_strategy(void *d, int f, daddr_t b, size_t s, void *buf, size_t *r)
{

	return (EIO);
}
