/* $NetBSD: vga_jazzio.c,v 1.4 2000/12/24 09:25:30 ur Exp $ */
/* NetBSD: vga_isa.c,v 1.3 1998/06/12 18:45:48 drochner Exp  */

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
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <machine/autoconf.h>
#include <machine/bus.h>

#include <dev/ic/mc6845reg.h>
#include <dev/ic/pcdisplayvar.h>
#include <dev/ic/vgareg.h>
#include <dev/ic/vgavar.h>
#include <arc/jazz/jazziovar.h>
#include <arc/jazz/pica.h>
#include <arc/jazz/vga_jazziovar.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsdisplayvar.h>

#define WSDISPLAY_TYPE_JAZZVGA	WSDISPLAY_TYPE_PCIVGA	/* XXX not really */

struct vga_jazzio_softc {
	struct device sc_dev; 
#if 0
	struct vga_config *sc_vc;	/* VGA configuration */
#endif
};

void	vga_jazzio_init_tag __P((bus_space_tag_t *, bus_space_tag_t *));
paddr_t	vga_jazzio_mmap __P((void *, off_t, int));
int	vga_jazzio_match __P((struct device *, struct cfdata *, void *));
void	vga_jazzio_attach __P((struct device *, struct device *, void *));

struct cfattach vga_jazzio_ca = {
	sizeof(struct vga_jazzio_softc), vga_jazzio_match, vga_jazzio_attach,
};

void
vga_jazzio_init_tag(iotp, memtp)
	bus_space_tag_t *iotp, *memtp;
{
	static int initialized = 0;
	static struct arc_bus_space vga_io, vga_mem;

	if (!initialized) {
		initialized = 1;
		arc_bus_space_init(&vga_io, "vga_jazzio_io",
		    PICA_P_LOCAL_VIDEO_CTRL, PICA_V_LOCAL_VIDEO_CTRL,
		    0, PICA_S_LOCAL_VIDEO_CTRL);
		arc_bus_space_init(&vga_mem, "vga_jazzio_mem",
		    PICA_P_LOCAL_VIDEO, PICA_V_LOCAL_VIDEO,
		    0, PICA_S_LOCAL_VIDEO);
	}
	*iotp = &vga_io;
	*memtp = &vga_mem;
}

paddr_t
vga_jazzio_mmap(v, offset, prot)
	void *v;
	off_t offset;
	int prot;
{
	if (offset >= 0xa0000 && offset < 0xc0000)
		return mips_btop(PICA_P_LOCAL_VIDEO + offset);
	if (offset >= 0x0000 && offset < 0x10000)
		return mips_btop(PICA_P_LOCAL_VIDEO_CTRL + offset);
	if (offset >= 0x40000000 && offset < 0x40800000)
		return mips_btop(PICA_P_LOCAL_VIDEO + offset - 0x40000000);
	return -1;
}

int
vga_jazzio_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct jazzio_attach_args *ja = aux;
	bus_space_tag_t iot, memt;

	if(strcmp(ja->ja_name, "vga") != 0)
		return(0);

	vga_jazzio_init_tag(&iot, &memt);
	if (!vga_is_console(iot, WSDISPLAY_TYPE_JAZZVGA) &&
	    !vga_common_probe(iot, memt))
		return (0);

	return (1);
}

void
vga_jazzio_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
#if 0
	struct vga_jazzio_softc *sc = (struct vga_jazzio_softc *)self;
#endif
	bus_space_tag_t iot, memt;

	printf("\n");

	vga_jazzio_init_tag(&iot, &memt);
	vga_common_attach(self, iot, memt, WSDISPLAY_TYPE_JAZZVGA,
	    vga_jazzio_mmap);
}

int
vga_jazzio_cnattach()
{
	bus_space_tag_t iot, memt;

	vga_jazzio_init_tag(&iot, &memt);
	return (vga_cnattach(iot, memt, WSDISPLAY_TYPE_JAZZVGA, 1));
}
