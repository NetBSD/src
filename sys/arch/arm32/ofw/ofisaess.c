/*
 * Copyright 1997
 * Digital Equipment Corporation. All rights reserved.
 *
 * This software is furnished under license and may be used and
 * copied only in accordance with the following terms and conditions.
 * Subject to these conditions, you may download, copy, install,
 * use, modify and distribute this software in source and/or binary
 * form. No title or ownership is transferred hereby.
 *
 * 1) Any source code used, modified or distributed must reproduce
 *    and retain this copyright notice and list of conditions as
 *    they appear in the source file.
 *
 * 2) No right is granted to use any trade name, trademark, or logo of
 *    Digital Equipment Corporation. Neither the "Digital Equipment
 *    Corporation" name nor any trademark or logo of Digital Equipment
 *    Corporation may be used to endorse or promote products derived
 *    from this software without the prior written permission of
 *    Digital Equipment Corporation.
 *
 * 3) This software is provided "AS-IS" and any express or implied
 *    warranties, including but not limited to, any implied warranties
 *    of merchantability, fitness for a particular purpose, or
 *    non-infringement are disclaimed. In no event shall DIGITAL be
 *    liable for any damages whatsoever, and in particular, DIGITAL
 *    shall not be liable for special, indirect, consequential, or
 *    incidental damages or damages for lost profits, loss of
 *    revenue or loss of use, whether such damages arise in contract,
 *    negligence, tort, under statute, in equity, at law or otherwise,
 *    even if advised of the possibility of such damage.
 */

/*
 *  OFW Glue for ES1887/ES888 Sound Driver
 */

#include <sys/param.h>
#include <sys/device.h>
#include <sys/socket.h>

#include <machine/intr.h>
#include <arm32/isa/isa_machdep.h>

#include <dev/ofw/openfirm.h>
#include <dev/isa/isavar.h>
#include <dev/isa/essreg.h>


int ofisaessprobe __P((struct device *, struct cfdata *, void *));
void ofisaessattach __P((struct device *, struct device *, void *));


struct cfattach ofisaess_ca = {
	sizeof(struct device), ofisaessprobe, ofisaessattach
};

struct cfdriver ofisaess_cd = {
	NULL, "ofisaess", DV_DULL
};


int
ofisaessprobe(parent, cf, aux)
    struct device *parent;
    struct cfdata *cf;
    void *aux;
{
    struct ofprobe *ofp = aux;
    char type[64];
    char name[64];
    char model[64];
    char compatible[64];

    /* At a minimum, must match type and name properties. */
    if (OF_getprop(ofp->phandle, "device_type", type, sizeof(type)) < 0 ||
	strcmp(type, "sound") != 0 ||
	OF_getprop(ofp->phandle, "name", name, sizeof(name)) < 0 ||
	strcmp(name, "sound") != 0)
	return 0;

    /* Full match on model. */
    if (OF_getprop(ofp->phandle, "model", model, sizeof(model)) > 0 &&
	(strcmp(model, "es1887-codec") == 0 ||
	 strcmp(model, "es888-codec") == 0 ||
	 strcmp(model, "ess1887-codec") == 0 ||
	 strcmp(model, "ess888-codec") == 0))
	return 3;

    /* Check for compatible match. */
    if (OF_getprop(ofp->phandle, "compatible", compatible, sizeof(compatible)) > 0 &&
	(strstr(compatible, "es1887-codec") != NULL ||
	 strstr(compatible, "es888-codec") != NULL ||
	 strstr(compatible, "ess1887-codec") != NULL ||
	 strstr(compatible, "ess888-codec") != NULL))
	return 2;

    /* No match. */
    return 0;
}


void
ofisaessattach(parent, dev, aux)
    struct device *parent, *dev;
    void *aux;
{
    struct ofprobe *ofp = aux;
    struct isa_attach_args ia;

    printf("\n");

    /* XXX - Hard-wire the ISA attach args for now. -JJK */
    ia.ia_iot = &isa_io_bs_tag;
    ia.ia_memt = &isa_mem_bs_tag;
    ia.ia_ic = NULL;			/* not used */
    ia.ia_iobase = 0x0220;
    ia.ia_iosize = ESS_NPORT;
    ia.ia_irq = IRQ_CODEC2;
    ia.ia_drq = 5;
    ia.ia_maddr = MADDRUNK;
    ia.ia_msize = 0;
    ia.ia_aux = (void *)ofp->phandle;
    ia.ia_delaybah = 0;			/* don't have this! */

    config_found(dev, &ia, NULL);
}
