/*	$NetBSD: cons.c,v 1.3 1998/10/28 02:47:35 sakamoto Exp $	*/

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
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
 * from: Utah Hdr: cons.c 1.7 92/02/28
 *
 *	@(#)cons.c	8.1 (Berkeley) 6/10/93
 */

#include <sys/param.h>
#include <bebox/include/bus.h>
#include "cons.h"

#ifdef CONS_BE
void becnprobe(), becninit(), becnputchar();
int becngetchar(), becnscan();
#endif

#ifdef CONS_VGA
void vgacnprobe(), vgacninit(), vgacnputchar();
int vgacngetchar(), vgacnscan();
#endif

#ifdef CONS_SERIAL
void siocnprobe(), siocninit(), siocnputchar();
int siocngetchar(), siocnscan();
#endif

struct consdev constab[] = {
#ifdef CONS_BE
	{ "be", becnprobe, becninit, becngetchar, becnputchar, becnscan },
#endif
#ifdef CONS_VGA
	{ "vga", vgacnprobe, vgacninit, vgacngetchar, vgacnputchar, vgacnscan },
#endif
#ifdef CONS_SERIAL
	{ "com", siocnprobe, siocninit, siocngetchar, siocnputchar, siocnscan },
#endif
	{ 0 }
};

struct consdev *cn_tab;
int noconsole;

char *
cninit()
{
	register struct consdev *cp;

	cn_tab = NULL;
	noconsole = 1;
	for (cp = constab; cp->cn_probe; cp++) {
		(*cp->cn_probe)(cp);
		if (cp->cn_pri > CN_DEAD &&
		    (cn_tab == NULL || cp->cn_pri > cn_tab->cn_pri))
			cn_tab = cp;
	}
	if (cn_tab) {
		(*cn_tab->cn_init)(cn_tab);
		noconsole = 0;
		return cn_tab->cn_name;
	}

	return NULL;
}

cngetc()
{
	if (cn_tab)
		return ((*cn_tab->cn_getc)(cn_tab->cn_dev));
	return (0);
}

cnputc(c)
	int c;
{
	if (cn_tab)
		(*cn_tab->cn_putc)(cn_tab->cn_dev, c);
}

cnscan()
{
	if (cn_tab)
		return ((*cn_tab->cn_scan)(cn_tab->cn_dev));
	return (0);
}

#ifdef CONS_BE
/*
 * BeBox default console
 */
void
becnprobe(cp)
	struct consdev *cp;
{
	cp->cn_pri = CN_INTERNAL;
}

void
becninit(cp)
	struct consdev *cp;
{
	unsigned char *display_memory = (unsigned char *)BEBOX_BUS_SPACE_MEM;

	video_init(display_memory);
	kbdreset();
}

becngetchar(cp)
	struct consdev *cp;
{
	return (kbd_getc());
}

void
becnputchar(cp, c)
	struct consdev *cp;
	register int c;
{
	video_putc(c);
}

becnscan(cp)
	struct consdev *cp;
{
	return (kbd(1));
}
#endif /* CONS_VGA */

#ifdef CONS_VGA
/*
 * VGA console
 */
void
vgacnprobe(cp)
	struct consdev *cp;
{
	cp->cn_pri = CN_NORMAL;
}

void
vgacninit(cp)
	struct consdev *cp;
{
	unsigned char *display_memory = (unsigned char *)BEBOX_BUS_SPACE_MEM;

	vga_reset(display_memory);
	vga_init(display_memory);
	kbdreset();
}

vgacngetchar(cp)
	struct consdev *cp;
{
	return (kbd_getc());
}

void
vgacnputchar(cp, c)
	struct consdev *cp;
	register int c;
{
	vga_putc(c);
}

vgacnscan(cp)
	struct consdev *cp;
{
	return (kbd(1));
}
#endif /* CONS_VGA */

#ifdef CONS_SERIAL
/*
 * serial console
 */
void
siocnprobe(cp)
	struct consdev *cp;
{
	cp->cn_pri = CN_REMOTE;
}

#include "ns16550.h"
struct NS16550 *sio;

void
siocninit(cp)
	struct consdev *cp;
{
	sio = (struct NS16550 *)NS16550_init();
}

siocngetchar(cp)
	struct consdev *cp;
{
	return (NS16550_getc(sio));
}

void
siocnputchar(cp, c)
	struct consdev *cp;
	register int c;
{
	if (c == '\n')
		NS16550_putc(sio, '\r');
	NS16550_putc(sio, c);
}

siocnscan(cp)
	struct consdev *cp;
{
	return (NS16550_scankbd(sio));
}
#endif /* CONS_SERIAL */
