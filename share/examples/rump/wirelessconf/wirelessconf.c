/*	$NetBSD: wirelessconf.c,v 1.1 2009/10/05 13:07:28 pooka Exp $	*/

/*
 * Copyright (c) 2009 Antti Kantee.  All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/reboot.h>
#include <sys/socket.h>

#include <net/if.h>

#include <rump/rump.h>
#include <rump/rump_syscalls.h>

#include <err.h>
#include <stdio.h>
#include <string.h>

/*
 * Example program for raising rum0 interface under rump.  This
 * requires a rum-compatible usb device attached as ugen.
 */

#define RUMFW "/libdata/firmware/rum/rum-rt2573"
int
main()
{
	extern int rumpns_boothowto;
	struct ifreq ifr;
	int s;

	rumpns_boothowto = AB_VERBOSE;
	rump_init();
	printf("\ndevice autoconfiguration finished\n");

	printf("tira-if-su ...\n");
	if (rump_etfs_register(RUMFW, RUMFW, RUMP_ETFS_REG) != 0)
		errx(1, "firmware etfs registration failed");

	/* rum?  shouldn't that be marsala? */
	strcpy(ifr.ifr_name, "rum0");
	ifr.ifr_flags = IFF_UP;
	s = rump_sys_socket(AF_INET, SOCK_DGRAM, 0);
	if (s == -1)
		err(1, "socket");

	if (rump_sys_ioctl(s, SIOCSIFFLAGS, &ifr) == -1)
		err(1, "ioctl");
	printf("... done\n");

	return 0;
}
