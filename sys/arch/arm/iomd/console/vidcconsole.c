/*	$NetBSD: vidcconsole.c,v 1.1 2001/10/05 22:27:45 reinoud Exp $	*/

/*
 * Copyright (c) 1996 Mark Brinicombe
 * Copyright (c) 1996 Robert Black
 * Copyright (c) 1994-1995 Melvyn Tang-Richardson
 * Copyright (c) 1994-1995 RiscBSD kernel team
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
 *	This product includes software developed by the RiscBSD kernel team
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE RISCBSD TEAM ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 * RiscBSD kernel project
 *
 * vidcconsole.c
 *
 * Console assembly functions
 *
 * Created      : 17/09/94
 * Last updated : 07/02/96
 */

/* woo */

#include <sys/cdefs.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
/*#include <sys/malloc.h>*/
#include <sys/ioctl.h>
#include <sys/conf.h>
#include <sys/tty.h>
#include <sys/device.h>
#include <sys/map.h>
#include <sys/proc.h>
/*#include <sys/user.h>*/
#include <sys/syslog.h>
/*#include <sys/resourcevar.h>*/

#include <machine/cpu.h>
#include <machine/param.h>
/*#include <machine/katelib.h>*/
/*#include <machine/cpu.h>*/
/*#include <machine/bootconfig.h>*/
/*#include <machine/iomd.h>*/
/*#include <machine/irqhandler.h>*/
/*#include <machine/pmap.h>*/
#include <arm/iomd/vidc.h>
#include <machine/vconsole.h>

extern int physcon_major;
extern struct vconsole *vconsole_default;
extern videomemory_t videomemory;
extern struct render_engine vidcrender;

struct vconsole *vconsole_spawn_re	__P((dev_t dev, struct vconsole *vc));

struct vidcconsole_softc {
	struct device device;
	int sc_opened;
};

int
vidcconsole_probe(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	return 1;
}

void
vidcconsole_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct vidcconsole_softc *vidcconsolesoftc = (void *)self;
	vidcconsolesoftc->sc_opened=0;

	printf(": refclk=%dMHz %dKB %s\n", (vidc_fref / 1000000),
	    videomemory.vidm_size / 1024,
	    (videomemory.vidm_type == VIDEOMEM_TYPE_VRAM) ? "VRAM" : "DRAM");
}

struct cfattach vidcconsole_ca = {
	sizeof (struct vidcconsole_softc), vidcconsole_probe, vidcconsole_attach
};

extern struct cfdriver vidcconsole_cd;

int
vidcconsoleopen(dev, flags, fmt, p)
	dev_t dev;
	int flags;
	int fmt;
	struct proc *p;
{
	struct vidcconsole_softc *sc;
	struct vconsole vconsole_new;
	int unit = minor(dev);
	int s;

	if (unit >= vidcconsole_cd.cd_ndevs)
		return ENXIO;
	sc = vidcconsole_cd.cd_devs[unit];
	if (!sc)
		return ENXIO;

	s = spltty();
/*	if (sc->sc_opened) {
		(void)splx(s);
		return(EBUSY);
	}*/
	++sc->sc_opened;
	(void)splx(s);

	if (sc->sc_opened == 1) {
		vconsole_new = *vconsole_default;
		vconsole_new.render_engine = &vidcrender;
		vconsole_spawn_re (
		makedev ( physcon_major, 64 + minor(dev) ),
		    &vconsole_new );
	} else {
		log(LOG_WARNING, "Multiple open of/dev/vidcconsole0 by proc %d\n", p->p_pid);
	}

	return 0;
}

int
vidcconsoleclose(dev, flags, fmt, p)
	dev_t dev;
	int flags;
	int fmt;
	struct proc *p;
{
	struct vidcconsole_softc *sc;
	int unit = minor(dev);
	int s;

	if ( unit >= vidcconsole_cd.cd_ndevs )
		return ENXIO;
	sc = vidcconsole_cd.cd_devs[unit];
	if (!sc)
		return ENXIO;

	s = spltty();
	--sc->sc_opened;
	(void)splx(s);

	return 0;
}

extern int physconioctl __P((dev_t, int, caddr_t, int,	struct proc *));

int
vidcconsoleioctl(dev, cmd, data, flag, p)
	dev_t dev;
	int cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	dev = makedev(physcon_major, 64 + minor(dev));
	return(physconioctl(dev, cmd, data, flag, p));
}

extern paddr_t physconmmap __P((dev_t, off_t, int));

paddr_t
vidcconsolemmap(dev, offset, prot)
	dev_t dev;
	off_t offset;
	int prot;
{
	dev = makedev(physcon_major, 64 + minor(dev));
	return(physconmmap(dev, offset, prot));
}
