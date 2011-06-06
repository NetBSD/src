/* $NetBSD: vga_jazzio.c,v 1.15.32.1 2011/06/06 09:05:00 jruoho Exp $ */
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: vga_jazzio.c,v 1.15.32.1 2011/06/06 09:05:00 jruoho Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <uvm/uvm_extern.h>

#include <machine/autoconf.h>
#include <machine/bus.h>
#include <machine/wired_map.h>

#include <mips/pte.h>

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

int	vga_jazzio_init_tag(const char *, bus_space_tag_t *, bus_space_tag_t *);
paddr_t	vga_jazzio_mmap(void *, off_t, int);
int	vga_jazzio_match(device_t, cfdata_t, void *);
void	vga_jazzio_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(vga_jazzio, sizeof(struct vga_softc),
    vga_jazzio_match, vga_jazzio_attach, NULL, NULL);

const struct vga_funcs vga_jazzio_funcs = {
	NULL,
	vga_jazzio_mmap,
};

int
vga_jazzio_init_tag(const char *name, bus_space_tag_t *iotp,
    bus_space_tag_t *memtp)
{
	static int initialized = 0;
	static struct arc_bus_space vga_io, vga_mem;

	if (strcmp(name, "ALI_S3") != 0)
		return ENXIO;

	if (!initialized) {
		initialized = 1;

		arc_bus_space_init(&vga_io, "vga_jazzio_io",
		    PICA_P_LOCAL_VIDEO_CTRL, PICA_V_LOCAL_VIDEO_CTRL,
		    0, PICA_S_LOCAL_VIDEO_CTRL);
		arc_bus_space_init(&vga_mem, "vga_jazzio_mem",
		    PICA_P_LOCAL_VIDEO, PICA_V_LOCAL_VIDEO,
		    0, PICA_S_LOCAL_VIDEO);

		arc_wired_enter_page(PICA_V_LOCAL_VIDEO_CTRL,
		    PICA_P_LOCAL_VIDEO_CTRL,
		    PICA_S_LOCAL_VIDEO_CTRL / 2);
		arc_wired_enter_page(
		    PICA_V_LOCAL_VIDEO_CTRL + PICA_S_LOCAL_VIDEO_CTRL/2,
		    PICA_P_LOCAL_VIDEO_CTRL + PICA_S_LOCAL_VIDEO_CTRL/2,
		    PICA_S_LOCAL_VIDEO_CTRL / 2);

		arc_wired_enter_page(PICA_V_LOCAL_VIDEO,
		    PICA_P_LOCAL_VIDEO,
		    PICA_S_LOCAL_VIDEO / 2);
		arc_wired_enter_page(
		    PICA_V_LOCAL_VIDEO + PICA_S_LOCAL_VIDEO / 2,
		    PICA_P_LOCAL_VIDEO + PICA_S_LOCAL_VIDEO / 2,
		    PICA_S_LOCAL_VIDEO / 2);
#if 0
		arc_wired_enter_page(PICA_V_EXTND_VIDEO_CTRL,
		    PICA_P_EXTND_VIDEO_CTRL,
		    PICA_S_EXTND_VIDEO_CTRL / 2);
		arc_wired_enter_page(
		    PICA_V_EXTND_VIDEO_CTRL + PICA_S_EXTND_VIDEO_CTRL / 2,
		    PICA_P_EXTND_VIDEO_CTRL + PICA_S_EXTND_VIDEO_CTRL / 2,
		    PICA_S_EXTND_VIDEO_CTRL / 2);
#endif
	}
	*iotp = &vga_io;
	*memtp = &vga_mem;
	return 0;
}

paddr_t
vga_jazzio_mmap(void *v, off_t offset, int prot)
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
vga_jazzio_match(device_t parent, cfdata_t match, void *aux)
{
	struct jazzio_attach_args *ja = aux;
	bus_space_tag_t iot, memt;

	if (vga_jazzio_init_tag(ja->ja_name, &iot, &memt))
		return 0;

	if (!vga_is_console(iot, WSDISPLAY_TYPE_JAZZVGA) &&
	    !vga_common_probe(iot, memt))
		return 0;

	return 1;
}

void
vga_jazzio_attach(device_t parent, device_t self, void *aux)
{
	struct vga_softc *sc = device_private(self);
	struct jazzio_attach_args *ja = aux;
	bus_space_tag_t iot, memt;

	aprint_normal("\n");

	sc->sc_dev = self;
	vga_jazzio_init_tag(ja->ja_name, &iot, &memt);
	vga_common_attach(sc, iot, memt, WSDISPLAY_TYPE_JAZZVGA, 0,
	    &vga_jazzio_funcs);
}

int
vga_jazzio_cnattach(char *name)
{
	bus_space_tag_t iot, memt;

	if (vga_jazzio_init_tag(name, &iot, &memt))
		return ENXIO;
	return vga_cnattach(iot, memt, WSDISPLAY_TYPE_JAZZVGA, 1);
}
