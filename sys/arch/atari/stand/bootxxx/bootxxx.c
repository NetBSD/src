/*	$NetBSD: bootxxx.c,v 1.8 2009/08/24 13:04:37 tsutsui Exp $	*/

/*
 * Copyright (c) 2001 Leo Weppelman.
 * Copyright (c) 1995 Waldi Ravens.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by Waldi Ravens.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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

#define	boot_BSD	bsd_startup

#include <lib/libsa/stand.h>
#include <atari_stand.h>
#include <libkern.h>
#include <sys/boot_flag.h>
#include <sys/reboot.h>
#include <machine/cpu.h>
#include <kparamb.h>
#include <libtos.h>
#include <tosdefs.h>

int	bootxxx(void *, void *, osdsc_t *);
void	boot_BSD(struct kparamb *)__attribute__((noreturn));

int
bootxxx(void *readsector, void *disklabel, osdsc_t *od)
{
	int		fd;
	char		*errmsg;
	extern char	end[], edata[];

	memset(edata, 0, end - edata);

	/* XXX: Limit should be 16MB */
	setheap(end, (void*)0x1000000);
	printf("\033v\nNetBSD/Atari tertiary bootloader "
					"($Revision: 1.8 $)\n\n");

	if (init_dskio(readsector, disklabel, od->rootfs))
		return -1;

	sys_info(od);
	if (!(od->cputype & ATARI_ANYCPU)) {
		printf("Unknown CPU-type.\n");
		return -2;
	}

	if ((fd = open(od->osname, 0)) < 0) {
		printf("Cannot open kernel '%s'\n", od->osname);
		return -3;
	}

#ifndef __ELF__		/* a.out */
	if (aout_load(fd, od, &errmsg, 1) != 0)
#else
	if (elf_load(fd, od, &errmsg, 1) != 0)
#endif
		return -4;

	boot_BSD(&od->kp);
	return -5;

	/* NOTREACHED */
}

void
_rtt(void)
{

	printf("Halting...\n");
	for (;;)
		;
}
