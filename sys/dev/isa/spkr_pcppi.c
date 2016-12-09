/*	$NetBSD: spkr_pcppi.c,v 1.1 2016/12/09 02:22:34 christos Exp $	*/

/*
 * Copyright (c) 1990 Eric S. Raymond (esr@snark.thyrsus.com)
 * Copyright (c) 1990 Andrew A. Chernov (ache@astral.msk.su)
 * Copyright (c) 1990 Lennart Augustsson (lennart@augustsson.net)
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
 *	This product includes software developed by Eric S. Raymond
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * spkr.c -- device driver for console speaker on 80386
 *
 * v1.1 by Eric S. Raymond (esr@snark.thyrsus.com) Feb 1990
 *      modified for 386bsd by Andrew A. Chernov <ache@astral.msk.su>
 *      386bsd only clean version, all SYSV stuff removed
 *      use hz value from param.c
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: spkr_pcppi.c,v 1.1 2016/12/09 02:22:34 christos Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/module.h>
#include <sys/uio.h>
#include <sys/proc.h>
#include <sys/ioctl.h>
#include <sys/conf.h>

#include <sys/bus.h>

#include <dev/isa/pcppivar.h>

#include <dev/isa/spkrio.h>

void spkrattach(device_t, device_t, void *);
int spkrdetach(device_t, int);
int spkrprobe(device_t, cfdata_t, void *);

#include "ioconf.h"

MODULE(MODULE_CLASS_DRIVER, spkr, NULL /* "pcppi" */);

#ifdef _MODULE
#include "ioconf.c"
#endif

CFATTACH_DECL_NEW(spkr_pcppi, 0, spkrprobe, spkrattach, spkrdetach, NULL);

static pcppi_tag_t ppicookie;

#define SPKRPRI (PZERO - 1)

void
spkr_tone(u_int xhz, u_int ticks)
/* emit tone of frequency hz for given number of ticks */
{
	pcppi_bell(ppicookie, xhz, ticks, PCPPI_BELL_SLEEP);
}

void
spkr_rest(int ticks)
/* rest for given number of ticks */
{
    /*
     * Set timeout to endrest function, then give up the timeslice.
     * This is so other processes can execute while the rest is being
     * waited out.
     */
#ifdef SPKRDEBUG
    printf("%s: %d\n", __func__, ticks);
#endif /* SPKRDEBUG */
    if (ticks > 0)
	    tsleep(spkr_rest, SPKRPRI | PCATCH, "rest", ticks);
}

extern int spkr_active;	/* exclusion flag */
extern const struct cdevsw spkr_cdevsw;

int spkr_attached = 0;

int
spkrprobe(device_t parent, cfdata_t match, void *aux)
{
	return (!spkr_attached);
}

void
spkrattach(device_t parent, device_t self, void *aux)
{
	aprint_naive("\n");
	aprint_normal("\n");
	ppicookie = ((struct pcppi_attach_args *)aux)->pa_cookie;
	spkr_attached = 1;
	if (!pmf_device_register(self, NULL, NULL))
		aprint_error_dev(self, "couldn't establish power handler\n");
}

int
spkrdetach(device_t self, int flags)
{

	pmf_device_deregister(self);
	spkr_attached = 0;
	ppicookie = NULL;

	return 0;
}


static int
spkr_modcmd(modcmd_t cmd, void *arg)
{
#ifdef _MODULE
	devmajor_t bmajor, cmajor;
#endif
	int error = 0;

#ifdef _MODULE
	switch(cmd) {
	case MODULE_CMD_INIT:
		bmajor = cmajor = -1;
		error = devsw_attach(spkr_cd.cd_name, NULL, &bmajor,
		    &spkr_cdevsw, &cmajor);
		if (error)
			break;

		error = config_init_component(cfdriver_ioconf_spkr,
			cfattach_ioconf_spkr, cfdata_ioconf_spkr);
		if (error) {
			devsw_detach(NULL, &spkr_cdevsw);
		}
		break;

	case MODULE_CMD_FINI:
		if (spkr_active)
			return EBUSY;
		error = config_fini_component(cfdriver_ioconf_spkr,
			cfattach_ioconf_spkr, cfdata_ioconf_spkr);
		devsw_detach(NULL, &spkr_cdevsw);
		break;
	default:
		error = ENOTTY;
		break;
	}
#endif

	return error;
}
