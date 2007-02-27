/*	$NetBSD: cons_machdep.c,v 1.1.28.1 2007/02/27 16:50:19 yamt Exp $	*/

/*-
 * Copyright (c) 2004, 2005 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cons_machdep.c,v 1.1.28.1 2007/02/27 16:50:19 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>

#include <dev/cons.h>

#include <machine/sbdvar.h>

#include <ews4800mips/ews4800mips/cons_machdep.h>

cons_decl(rom_);

struct cons cons;
struct consdev *cn_tab = NULL;
struct consdev consdev_rom = {
	rom_cnprobe,
	rom_cninit,
	rom_cngetc,
	rom_cnputc,
	rom_cnpollc,
	NULL,
	NULL,
	NULL,
	NODEV,
	CN_DEAD
};

void
consinit(void)
{
	static int initted;

	if (initted)
		return;

	if (platform.consinit) {
		(*platform.consinit)();
		initted = 1;
	} else
		rom_cons_init();	/* XXX */
}

void
rom_cons_init(void)
{

	cons.type = CONS_ROM;
	cn_tab = &consdev_rom;
	(*cn_tab->cn_init)(cn_tab);
}

void
rom_cnprobe(struct consdev *cn)
{

	cn->cn_pri = CN_INTERNAL;
}

void
rom_cninit(struct consdev *cn)
{
	static int initted;

	if (initted)
		return;

	cons.x = X_INIT;
#if 0
	cons.y = Y_INIT;
#else
	cons.y = 20;	/* XXX no way to get the previous cursor position */
#endif
	initted = 1;
}

void
rom_cnputc(dev_t dev, int c)
{
	int i;

	switch (c) {
	default:
		ROM_PUTC(cons.x * ROM_FONT_WIDTH, cons.y * ROM_FONT_HEIGHT, c);
		if (++cons.x == CONS_WIDTH) {
			cons.x = X_INIT;
			cons.y++;
		}
		break;
	case '\b':
		ROM_PUTC(cons.x * ROM_FONT_WIDTH, cons.y * ROM_FONT_HEIGHT, c);
		cons.x = cons.x == X_INIT ? X_INIT : cons.x - 1;
		break;
	case '\t':
		for (i = cons.x % 8; i < 8; i++) {
			ROM_PUTC(cons.x * ROM_FONT_WIDTH,
			    cons.y * ROM_FONT_HEIGHT, ' ');
			if (++cons.x == CONS_WIDTH) {
				cons.x = X_INIT;
				if (++cons.y == CONS_HEIGHT)
					cons.y = Y_INIT;
			}
		}
		break;
	case '\r':
		ROM_PUTC(cons.x * ROM_FONT_WIDTH, cons.y * ROM_FONT_HEIGHT, c);
		cons.x = X_INIT;
		break;
	case '\n':
		ROM_PUTC(cons.x * ROM_FONT_WIDTH, cons.y * ROM_FONT_HEIGHT,
		    '\r');
		ROM_PUTC(cons.x * ROM_FONT_WIDTH, cons.y * ROM_FONT_HEIGHT, c);
		cons.x = X_INIT;
		cons.y++;
		break;
	}

	if (cons.y == CONS_HEIGHT)
		cons.y = Y_INIT;
}

int
rom_cngetc(dev_t dev)
{

	return ROM_GETC();
}

void
rom_cnpollc(dev_t dev, int on)
{
	static bool __polling = false;
	static int s;

	if (on && !__polling) {
		s = splhigh();	/* Disable interrupt driven I/O */
		__polling = true;
	} else if (!on && __polling) {
		__polling = false;
		splx(s);	/* Enable interrupt driven I/O */
	}
}
