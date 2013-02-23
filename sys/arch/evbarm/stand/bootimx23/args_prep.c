/* $Id: args_prep.c,v 1.1 2013/02/23 16:22:39 jkunz Exp $ */

/*
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Petri Laakso.
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

/* (c) for ngets() below:
 *
 * Copyright (c) 1993
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 *	@(#)gets.c	8.1 (Berkeley) 6/11/93
 */


#include <sys/param.h>
#include <sys/types.h>
#include <sys/cdefs.h>

#include <lib/libsa/stand.h>
#include <lib/libkern/libkern.h>

#include <arm/arm32/pte.h>
#define _LOCORE
#include <arm/imx/imx23var.h>
#undef _LOCORE
#include <evbarm/bootconfig.h>

#include <arm/imx/imx23_digctlreg.h>
#include <arm/imx/imx23_uartdbgreg.h>

#include "common.h"

static void ngets(char *, int);

#define L1_PAGE_TABLE (DRAM_BASE + MEMSIZE * 1024 * 1024 - L1_TABLE_SIZE)
#define BOOTIMX23_ARGS (L1_PAGE_TABLE - MAX_BOOT_STRING - 1)

#define PROMPT_DELAY 5000000 /* Wait 5 seconds user to press any key. */

int
args_prep(void)
{
	u_int prompt;
	char *boot_args = (char *)BOOTIMX23_ARGS;

	/* Copy default boot arguments. */
	memset((void *)boot_args, 0x00, MAX_BOOT_STRING);
	strcpy(boot_args, KERNEL_BOOT_ARGS);

	prompt = 0;

	/* Enable debug UART data reception which was not enabled by the ROM. */
	uint16_t cr = REG_RD_HW(HW_UARTDBG_BASE + HW_UARTDBGCR);
	cr |= HW_UARTDBGCR_RXE;
	REG_WR_HW(HW_UARTDBG_BASE + HW_UARTDBGCR, cr);

	printf("Press any key to drop into boot prompt...\n\r");

	REG_WR(HW_DIGCTL_BASE + HW_DIGCTL_MICROSECONDS_CLR, 0xFFFFFFFF);

	while (REG_RD(HW_DIGCTL_BASE + HW_DIGCTL_MICROSECONDS) < PROMPT_DELAY) {
		if (!(REG_RD_HW(HW_UARTDBG_BASE + HW_UARTDBGFR) &
		    HW_UARTDBGFR_RXFE)) {
			/* RX FIFO is not empty, some key was pressed. */
			REG_RD(HW_UARTDBG_BASE + HW_UARTDBGDR); /* Flush. */
			prompt = 1;
			break;
		}
	}
	
	if (prompt) {
		memset((void *)boot_args, 0x00, MAX_BOOT_STRING);
		printf("boot: ");
		ngets(boot_args, MAX_BOOT_STRING);
	}

	return 0;
}

/*
 * gets() with constrained input length.
 *
 * Copied from: sys/arch/ia64/stand/common/gets.c
 */
static void
ngets(char *buf, int n)
{
	int c;
	char *lp;

	for (lp = buf;;) {
		switch (c = getchar() & 0177) {
		case '\n':
		case '\r':
			*lp = '\0';
			putchar('\n');
			return;
		case '\b':
		case '\177':
			if (lp > buf) {
				lp--;
				putchar('\b');
				putchar(' ');
				putchar('\b');
			}
			break;
		case 'r'&037: {
			char *p;

			putchar('\n');
			for (p = buf; p < lp; ++p)
				putchar(*p);
			break;
		}
		case 'u'&037:
		case 'w'&037:
			lp = buf;
			putchar('\n');
			break;
		default:
			if ((n < 1) || ((lp - buf) < n)) {
				*lp++ = c;
				putchar(c);
			}
		}
	}
	/*NOTREACHED*/
}
