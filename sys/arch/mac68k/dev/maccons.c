/*	$NetBSD: maccons.c,v 1.2.20.1 2002/05/19 07:56:38 gehenna Exp $	*/

/*
 * Copyright (C) 1999 Scott Reynolds.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

#include "wsdisplay.h"
#include "wskbd.h"
#include "zsc.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>

#include <machine/autoconf.h>
#include <machine/cpu.h>

#include <dev/cons.h>
#include <dev/wscons/wskbdvar.h>
#include <dev/wscons/wsdisplayvar.h>
#include <mac68k/dev/macfbvar.h>
#include <mac68k/dev/akbdvar.h>

void maccnprobe __P((struct consdev *));
void maccninit __P((struct consdev *));
int maccngetc __P((dev_t));
void maccnputc __P((dev_t, int));
void maccnpollc __P((dev_t, int));

static int	maccons_initted = (-1);

/* From Booter via locore */
extern u_int32_t	mac68k_vidphys;

void
maccnprobe(struct consdev *cp)
{
#if NWSDISPLAY > 0
	extern const struct cdevsw wsdisplay_cdevsw;
	int     maj, unit;
#endif

	cp->cn_dev = NODEV;
	cp->cn_pri = CN_NORMAL;

#if NWSDISPLAY > 0
	unit = 0;
	maj = cdevsw_lookup_major(&wsdisplay_cdevsw);
	if (maj != -1) {
		cp->cn_pri = CN_INTERNAL;
		cp->cn_dev = makedev(maj, unit);
	}
#endif
}

void
maccninit(struct consdev *cp)
{
	/*
	 * XXX evil hack; see consinit() for an explanation.
	 * note:  maccons_initted is initialized to (-1).
	 */
	if (++maccons_initted > 0) {
		macfb_cnattach(mac68k_vidphys);
		akbd_cnattach();
	}
}

int
maccngetc (dev_t dev)
{
#if NWSKBD > 0
	if (maccons_initted > 0)
		return wskbd_cngetc(dev);
#endif
	return 0;
}

void
maccnputc(dev_t dev, int c)
{
#if NZSC > 0
	extern dev_t mac68k_zsdev;
	extern int zscnputc __P((dev_t dev, int c));
#endif

#if NWSDISPLAY > 0
	if (maccons_initted > 0)
		wsdisplay_cnputc(dev,c);	
#endif
#if NZSC > 0
        if (mac68k_machine.serial_boot_echo) 
                zscnputc(mac68k_zsdev, c);
#endif 
}

void
maccnpollc(dev_t dev, int on)
{
#if NWSKBD > 0
	if (maccons_initted > 0)
		wskbd_cnpollc(dev,on);
#endif
}
