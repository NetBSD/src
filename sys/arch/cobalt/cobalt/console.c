/*	$NetBSD: console.c,v 1.7 2004/01/07 12:43:43 cdi Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: console.c,v 1.7 2004/01/07 12:43:43 cdi Exp $");

#include <sys/param.h>
#include <sys/user.h>
#include <sys/uio.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/proc.h>
#include <sys/tty.h>
#include <sys/termios.h>

#include <machine/bus.h>
#include <machine/nvram.h>
#include <machine/bootinfo.h>

#include <dev/cons.h>

#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>

#include "com.h"
#include "nullcons.h"

dev_type_cnprobe(comcnprobe);
dev_type_cninit(comcninit);

int	console_present = 0;	/* Do we have a console? */

struct	consdev	constab[] = {
#if NCOM > 0
	{ comcnprobe, comcninit, },
#endif
#if NNULLCONS > 0
	{ nullcnprobe, nullcninit },
#endif
	{ 0 }
};

#if NCOM > 0
#define CONMODE ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8) /* 8N1 */

void
comcnprobe(cn)
	struct consdev *cn;
{
	struct btinfo_flags *bi_flags;

	/*
	 * Linux code has a comment that serial console must be probed
	 * early, otherwise the value which allows to detect serial port
	 * could be overwritten. Okay, probe here and record the result
	 * for the future use.
	 *
	 * Note that if the kernel was booted with a boot loader,
	 * the latter *has* to provide a flag indicating whether console
	 * is present or not due to the reasons outlined above.
	 */
	if ( (bi_flags = lookup_bootinfo(BTINFO_FLAGS)) == NULL) {
		/* No boot information, probe console now */
		console_present = *(volatile u_int32_t *)
					MIPS_PHYS_TO_KSEG1(0x0020001c);
	} else {
		/* Get the value determined by the boot loader. */
		console_present = bi_flags->bi_flags & BI_SERIAL_CONSOLE;
	}

	cn->cn_pri = (console_present != 0) ? CN_NORMAL : CN_DEAD;
}

void
comcninit(cn)
	struct consdev *cn;
{

	comcnattach(0, 0x1c800000, 115200, COM_FREQ * 10, COM_TYPE_NORMAL,
	    CONMODE);
}
#endif

void
consinit()
{

	cninit();
}
