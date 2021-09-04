/*	$NetBSD: console.c,v 1.13 2021/09/04 02:19:56 tsutsui Exp $	*/

/*
 * Copyright (c) 2000 Soren S. Jorvang.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: console.c,v 1.13 2021/09/04 02:19:56 tsutsui Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/conf.h>

#include <sys/bus.h>
#include <machine/nvram.h>
#include <machine/bootinfo.h>

#include <dev/cons.h>

#include <cobalt/dev/com_mainbusvar.h>
#include <machine/z8530var.h>

#include "com_mainbus.h"
#include "zsc.h"
#include "nullcons.h"

int	console_present = 0;	/* Do we have a console? */

struct	consdev	constab[] = {
#if NCOM_MAINBUS > 0
	{
		.cn_probe = com_mainbus_cnprobe,
		.cn_init  = com_mainbus_cninit,
		.cn_getc  = NULL,
		.cn_putc  = NULL,
		.cn_pollc = NULL,
		.cn_bell  = NULL,
		.cn_halt  = NULL,
		.cn_flush = NULL,
		.cn_dev   = 0,
		.cn_pri   = CN_DEAD
	},
#endif
#if NZSC > 0
	{
		.cn_probe = zscnprobe,
		.cn_init  = zscninit,
		.cn_getc  = zscngetc,
		.cn_putc  = zscnputc,
		.cn_pollc = nullcnpollc,
		.cn_bell  = NULL,
		.cn_halt  = NULL,
		.cn_flush = NULL,
		.cn_dev   = NODEV,
		.cn_pri   = CN_DEAD
	},
#endif
#if NNULLCONS > 0
	{
		.cn_probe = nullcnprobe,
		.cn_init  = nullcninit,
		.cn_getc  = NULL,
		.cn_putc  = NULL,
		.cn_pollc = NULL,
		.cn_bell  = NULL,
		.cn_halt  = NULL,
		.cn_flush = NULL,
		.cn_dev   = 0,
		.cn_pri   = CN_DEAD
	},
#endif
	{
		.cn_probe = NULL,
		.cn_init  = NULL,
		.cn_getc  = NULL,
		.cn_putc  = NULL,
		.cn_pollc = NULL,
		.cn_bell  = NULL,
		.cn_halt  = NULL,
		.cn_flush = NULL,
		.cn_dev   = 0,
		.cn_pri   = CN_DEAD
	}
};

#define CONSOLE_PROBE	0x0020001c	/* console flag passed by firmware */

void
consinit(void)
{
	struct btinfo_flags *bi_flags;
	static int initted;

	if (initted)
		return;
	initted = 1;

	/*
	 * Linux code has a comment that serial console must be probed
	 * early, otherwise the value which allows to detect serial port
	 * could be overwritten. Okay, probe here and record the result
	 * for the future use.
	 *
	 * Note that if the kernel was booted with a boot loader,
	 * the latter *has* to provide a flag indicating whether console
	 * is present or not because the value might be overwritten by
	 * the loaded kernel.
	 */
	bi_flags = lookup_bootinfo(BTINFO_FLAGS);
	if (bi_flags == NULL) {
		/* No boot information, probe console now */
		console_present =
		    *(volatile uint32_t *)MIPS_PHYS_TO_KSEG1(CONSOLE_PROBE);
	} else {
		/* Get the value determined by the boot loader. */
		console_present = bi_flags->bi_flags & BI_SERIAL_CONSOLE;
	}

	cninit();
}
