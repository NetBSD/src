/* $NetBSD: pccons_jazzio.c,v 1.1 2001/06/13 15:05:46 soda Exp $ */
/* NetBSD: vga_isa.c,v 1.4 2000/08/14 20:14:51 thorpej Exp  */

/*
 * Copyright (c) 1995, 1996 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 * 
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS" 
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND 
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <uvm/uvm_extern.h>

#include <machine/autoconf.h>
#include <machine/bus.h>

#include <mips/pte.h>

#include <arc/arc/wired_map.h>
#include <arc/dev/pcconsvar.h>
#include <arc/jazz/jazziovar.h>
#include <arc/jazz/pica.h>
#include <arc/jazz/pccons_jazziovar.h>

#define PCKBD_INTR 6	/* XXX - should be obtained from firmware */

int	pccons_jazzio_match __P((struct device *, struct cfdata *, void *));
void	pccons_jazzio_attach __P((struct device *, struct device *, void *));

struct cfattach pc_jazzio_ca = {
	sizeof(struct pc_softc),
	pccons_jazzio_match, pccons_jazzio_attach,
};

/*
 * chipset-dependent pccons configuration
 */

void pccons_jazzio_init __P((void));

struct pccons_config pccons_jazzio_conf = {
	0x3b4, 0xb0000,	/* mono: iobase, memaddr */
	0x3d4, 0xb8000,	/* cga:  iobase, memaddr */
	PICA_SYS_KBD + 0x61, PICA_SYS_KBD + 0x60, /* kbdc: cmdport, dataport */
	pccons_jazzio_init
};

void
pccons_jazzio_init()
{
	/* nothing to do */
}

int	pccons_jazzio_init_tag __P((char*, bus_space_tag_t*,bus_space_tag_t*));

int
pccons_jazzio_init_tag(name, iotp, memtp)
	char *name;
	bus_space_tag_t *iotp, *memtp;
{
	static int initialized = 0;
	static struct arc_bus_space vga_io, vga_mem;

	if (strcmp(name, "ALI_S3") != 0)
		return(ENXIO);

	if (!initialized) {
		initialized = 1;

		arc_bus_space_init(&vga_io, "vga_jazzio_io",
		    PICA_P_LOCAL_VIDEO_CTRL, PICA_V_LOCAL_VIDEO_CTRL,
		    0, PICA_S_LOCAL_VIDEO_CTRL);
		arc_bus_space_init(&vga_mem, "vga_jazzio_mem",
		    PICA_P_LOCAL_VIDEO, PICA_V_LOCAL_VIDEO,
		    0, PICA_S_LOCAL_VIDEO);

		arc_enter_wired(PICA_V_LOCAL_VIDEO_CTRL,
		    PICA_P_LOCAL_VIDEO_CTRL,
		    PICA_P_LOCAL_VIDEO_CTRL + PICA_S_LOCAL_VIDEO_CTRL/2,
		    MIPS3_PG_SIZE_1M);
		arc_enter_wired(PICA_V_LOCAL_VIDEO,
		    PICA_P_LOCAL_VIDEO,
		    PICA_P_LOCAL_VIDEO + PICA_S_LOCAL_VIDEO/2,
		    MIPS3_PG_SIZE_4M);
#if 0
		arc_enter_wired(PICA_V_EXTND_VIDEO_CTRL,
		    PICA_P_EXTND_VIDEO_CTRL,
		    PICA_P_EXTND_VIDEO_CTRL + PICA_S_EXTND_VIDEO_CTRL/2,
		    MIPS3_PG_SIZE_1M);
#endif
	}
	*iotp = &vga_io;
	*memtp = &vga_mem;
	return (0);
}

int
pccons_jazzio_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct jazzio_attach_args *ja = aux;
	bus_space_tag_t crt_iot, crt_memt;

	if (pccons_jazzio_init_tag(ja->ja_name, &crt_iot, &crt_memt))
		return (0);

	if (!pccons_common_match(crt_iot, crt_memt, ja->ja_bust,
	    &pccons_jazzio_conf))
		return (0);

	return (1);
}

void
pccons_jazzio_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct pc_softc *sc = (struct pc_softc *)self;
	struct jazzio_attach_args *ja = aux;
	bus_space_tag_t crt_iot, crt_memt;

	pccons_jazzio_init_tag(ja->ja_name, &crt_iot, &crt_memt);
	jazzio_intr_establish(PCKBD_INTR, pcintr, self);
	pccons_common_attach(sc, crt_iot, crt_memt, ja->ja_bust,
	    &pccons_jazzio_conf);
}

int
pccons_jazzio_cnattach(name, kbd_iot)
	char *name;
	bus_space_tag_t kbd_iot;
{
	bus_space_tag_t crt_iot, crt_memt;

	if (pccons_jazzio_init_tag(name, &crt_iot, &crt_memt))
		return (ENXIO);
	pccons_common_cnattach(crt_iot, crt_memt, kbd_iot,
	    &pccons_jazzio_conf);
	return (0);
}

