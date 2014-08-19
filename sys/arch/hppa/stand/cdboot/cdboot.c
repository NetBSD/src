/*	$NetBSD: cdboot.c,v 1.1.10.2 2014/08/20 00:03:05 tls Exp $	*/

/*-
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
 *	@(#)boot.c	8.1 (Berkeley) 6/10/93
 */

/*
 * Copyright (c) 1998-2004 Michael Shalayeff
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR HIS RELATIVES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF MIND, USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <sys/param.h>
#include <sys/reboot.h>
#include <sys/boot_flag.h>

#include <arch/hppa/stand/common/libsa.h>
#include <lib/libsa/loadfile.h>

#include <machine/pdc.h>
#include <machine/vmparam.h>

#include <arch/hppa/stand/common/dev_hppa.h>

/*
 * Boot program... bits in `howto' determine whether boot stops to
 * ask for system name.	 Boot device is derived from ROM provided
 * information.
 */

void boot(dev_t boot_dev);

typedef void (*startfuncp)(int, int, int, int, int, int, void *)
    __attribute__ ((noreturn));

void
boot(dev_t boot_dev)
{
	u_long marks[MARK_MAX];
	u_long loadaddr = 0;
	char path[128];
	int fd;

	machdep();
#ifdef	DEBUGBUG
	debug = 1;
#endif
	devboot(boot_dev, path);

	strncpy(path + strlen(path), ":/netbsd", 9);
	printf(">> %s, Revision %s\n", bootprog_name, bootprog_rev);
	printf(">> Booting %s: ", path);

#define	round_to_size(x) \
	(((x) + sizeof(u_long) - 1) & ~(sizeof(u_long) - 1))

#define	BOOTARG_APIVER 2

	marks[MARK_START] = loadaddr;
	if ((fd = loadfile(path, marks, LOAD_KERNEL)) == -1)
		return;

	marks[MARK_END] = round_to_size(marks[MARK_END] - loadaddr);
	printf("Start @ 0x%lx [%ld=0x%lx-0x%lx]...\n",
	    marks[MARK_ENTRY], marks[MARK_NSYM],
	    marks[MARK_SYM], marks[MARK_END]);

#ifdef EXEC_DEBUG
	if (debug) {
		printf("ep=0x%x [", marks[MARK_ENTRY]);
		for (i = 0; i < 10240; i++) {
			if (!(i % 8)) {
				printf("\b\n%p:", &((u_int *)marks[MARK_ENTRY])[i]);
				if (getchar() != ' ')
					break;
			}
			printf("%x,", ((int *)marks[MARK_ENTRY])[i]);
		}
		printf("\b\b ]\n");
	}
#endif

	fcacheall();

	__asm("mtctl %r0, %cr17");
	__asm("mtctl %r0, %cr17");
	/* stack and the gung is ok at this point, so, no need for asm setup */
	(*(startfuncp)(marks[MARK_ENTRY])) ((int)pdc, 0, boot_dev, marks[MARK_END],
				       BOOTARG_APIVER, 0, NULL);
	/* not reached */
}
