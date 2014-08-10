/*	$NetBSD: boot.c,v 1.5.60.1 2014/08/10 06:54:03 tls Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Steve C. Woodford.
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

/*
 * Copyright (c) 1982, 1986, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * 	@(#)boot.c	8.1 (Berkeley) 6/10/93
 */

#include <sys/param.h>
#include <sys/reboot.h>
#include <sys/termios.h>
#include <sys/ttydefaults.h>
#include <machine/bootinfo.h>

#include <lib/libkern/libkern.h>

#include "stand.h"
#include "libsa.h"
#include "bugsyscalls.h"

struct bug_bootinfo	bug_bootinfo;
struct mvmeppc_bootinfo	bootinfo;

static u_int32_t ioctrl2cflag(u_int32_t);

void main(void);

void
main(void)
{
	struct bug_buginfo *bbi;
	struct bug_boardid *bid;
	struct bug_ioinquiry *ioi, ioinq;
	struct bug_ioctrl ioctrl;
	char consname[CONSOLEDEV_LEN];
	char line[80];
	const char *file;
	int ask = 0, howto, part;

	if (bug_bootinfo.bbi_bugmode == 0)
		panic("mvmeppc-boot: PReP boot mode not supported!");

	bbi = &bug_bootinfo.bbi_bi.bbi;

	if ((bid = bugsys_brdid()) == NULL)
		panic("mvmeppc-boot: bugsys_brdid() failed!");

	ioinq.ii_boardname = consname;
	ioinq.ii_ioctrl = &ioctrl;
	ioinq.ii_portnum = BUG_IOINQ_PORT_CONSOLE;
	if ((ioi = bugsys_ioinq(&ioinq)) == NULL)
		panic("mvmeppc-boot: bugsys_ioinq() failed!");

	if (bid->bi_devtype > 9)
		panic("mvmeppc-boot: Bogus boot device type (%d)",
		    bid->bi_devtype);

	printf(">> MVMEPPC boot on MVME%x\n", bid->bi_bnumber);

	parse_args(bbi->bbi_argstart, bbi->bbi_argend, &file, &howto, &part);

	for (;;) {
		if (ask) {
			printf("boot: ");
			gets(line);
			if (strcmp(line, "halt") == 0)
				break;

			if (line[0]) {
				char *cp = line;

				while (cp < (line + sizeof(line) - 1) && *cp) 
					cp++;

				bbi->bbi_argstart = line;
				bbi->bbi_argend = cp;
				parse_args(bbi->bbi_argstart, bbi->bbi_argend,
				    &file, &howto, &part);
			}
		}

		bootinfo.bi_boothowto = howto;
		bootinfo.bi_bootaddr = bbi->bbi_devaddr;
		bootinfo.bi_bootclun = bbi->bbi_clun;
		bootinfo.bi_bootclun = bbi->bbi_dlun;
		strncpy(bootinfo.bi_bootline, bbi->bbi_argstart,
		    MIN(BOOTLINE_LEN, bbi->bbi_argend - bbi->bbi_argstart));
		strncpy(bootinfo.bi_consoledev, consname, CONSOLEDEV_LEN);
		bootinfo.bi_consoleaddr = ioi->ii_devaddr;
		bootinfo.bi_consolechan = ioi->ii_channel;
		bootinfo.bi_consolespeed = ioctrl.ic_baud;
		bootinfo.bi_consolecflag = ioctrl2cflag(ioctrl.ic_ctrlbits);
		bootinfo.bi_modelnumber = bid->bi_bnumber;

		exec_mvme(file, howto, part);
		printf("boot: %s: %s\n", file, strerror(errno));
		ask = 1;
	}
}

static u_int32_t
ioctrl2cflag(u_int32_t ctrlbits)
{
	u_int32_t rv;

	rv = TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB | PARODD);

	/* Convert parity */
	if (ctrlbits & IOCTRL_PARITY_ODD)
		rv |= PARENB | PARODD;
	else
	if (ctrlbits & IOCTRL_PARITY_EVEN)
		rv |= PARENB;

	/* Convert character length */
	if (ctrlbits & IOCTRL_BITS_8)
		rv |= CS8;
	else
	if (ctrlbits & IOCTRL_BITS_7)
		rv |= CS7;
	else
	if (ctrlbits & IOCTRL_BITS_6)
		rv |= CS6;
	else
	if (ctrlbits & IOCTRL_BITS_5)
		rv |= CS5;
	else
		panic("ioctrl2cflag: Bad character length: 0x%x", ctrlbits);

	/* Convert number of stop bits */
	if (ctrlbits & IOCTRL_STOP_2)
		rv |= CSTOPB;
	else
	if ((ctrlbits & IOCTRL_STOP_1) == 0)
		panic("ioctrl2cflag: Bad number of stop bits: 0x%x", ctrlbits);

	return (rv);
}
