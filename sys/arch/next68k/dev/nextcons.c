/*	$NetBSD: nextcons.c,v 1.6 2003/07/15 02:59:31 lukem Exp $	*/

/*
 * Copyright (c) 1999 Darrin B. Jewell
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
 *      This product includes software developed by Darrin B. Jewell
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: nextcons.c,v 1.6 2003/07/15 02:59:31 lukem Exp $");

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/errno.h>
#include <sys/queue.h>
#include <sys/lock.h>

#include <machine/autoconf.h>
#include <machine/bus.h>
#include <machine/cpu.h>
#include <machine/intr.h>

#include <dev/cons.h>
#include <dev/wscons/wskbdvar.h>
#include <dev/wscons/wsdisplayvar.h>

#include <next68k/dev/intiovar.h>
#include <next68k/dev/nextdisplayvar.h>
#include <next68k/dev/nextkbdvar.h>

#include <next68k/next68k/nextrom.h>

void nextcnprobe __P((struct consdev *));
void nextcninit __P((struct consdev *));
int nextcngetc __P((dev_t));
void nextcnputc __P((dev_t, int));
void nextcnpollc __P((dev_t, int));


void
nextcnprobe(struct consdev *cp)
{

	if ((rom_machine_type == NeXT_WARP9)
	    || (rom_machine_type == NeXT_X15)
	    || (rom_machine_type == NeXT_WARP9C)
	    || (rom_machine_type == NeXT_TURBO_MONO)
	    || (rom_machine_type == NeXT_TURBO_COLOR))
		cp->cn_pri = CN_INTERNAL;
	else 
		cp->cn_pri = CN_DEAD;

	cp->cn_dev = NODEV;
}

void
nextcninit(struct consdev *cp)
{
	nextkbd_cnattach(NEXT68K_INTIO_BUS_SPACE);
	nextdisplay_cnattach();
}

int
nextcngetc (dev_t dev)
{
	return wskbd_cngetc(dev);
}

void
nextcnputc(dev_t dev, int c)
{
	wsdisplay_cnputc(dev,c);	
}

void
nextcnpollc(dev_t dev, int on)
{
	wskbd_cnpollc(dev,on);
}
