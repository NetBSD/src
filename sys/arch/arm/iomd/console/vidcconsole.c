/*	$NetBSD: vidcconsole.c,v 1.10 2003/07/15 00:24:43 lukem Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: vidcconsole.c,v 1.10 2003/07/15 00:24:43 lukem Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
/*#include <sys/malloc.h>*/
#include <sys/ioctl.h>
#include <sys/conf.h>
#include <sys/tty.h>
#include <sys/device.h>
#include <sys/proc.h>
/*#include <sys/user.h>*/
#include <sys/syslog.h>
/*#include <sys/resourcevar.h>*/

#include <machine/cpu.h>
#include <machine/param.h>
#include <arm/iomd/vidc.h>
#include <arm/iomd/console/console.h>
#include <machine/vconsole.h>

extern const struct cdevsw physcon_cdevsw;
extern struct vconsole *vconsole_default;
extern videomemory_t videomemory;
extern struct render_engine vidcrender;

int	vidcconsole_probe(struct device *, struct cfdata *, void *);
void	vidcconsole_attach(struct device *, struct device *, void *);

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

CFATTACH_DECL(vidcconsole, sizeof (struct vidcconsole_softc),
    vidcconsole_probe, vidcconsole_attach, NULL, NULL);

extern struct cfdriver vidcconsole_cd;

dev_type_open(vidcconsoleopen);
dev_type_close(vidcconsoleclose);
dev_type_ioctl(vidcconsoleioctl);
dev_type_mmap(vidcconsolemmap);

const struct cdevsw vidcconsole_cdevsw = {
	vidcconsoleopen, vidcconsoleclose, noread, nowrite, vidcconsoleioctl,
	nostop, notty, nopoll, vidcconsolemmap, nokqfilter,
};

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
		makedev ( cdevsw_lookup_major(&physcon_cdevsw),
			  64 + minor(dev) ),
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

int
vidcconsoleioctl(dev, cmd, data, flag, p)
	dev_t dev;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	dev = makedev(cdevsw_lookup_major(&physcon_cdevsw),
		      64 + minor(dev));
	return((*physcon_cdevsw.d_ioctl)(dev, cmd, data, flag, p));
}

paddr_t
vidcconsolemmap(dev, offset, prot)
	dev_t dev;
	off_t offset;
	int prot;
{
	dev = makedev(cdevsw_lookup_major(&physcon_cdevsw),
		      64 + minor(dev));
	return((*physcon_cdevsw.d_mmap)(dev, offset, prot));
}
