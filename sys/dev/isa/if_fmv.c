/*	$NetBSD: if_fmv.c,v 1.23.20.1 2000/06/22 17:07:10 minoura Exp $	*/

/*
 * All Rights Reserved, Copyright (C) Fujitsu Limited 1995
 *
 * This software may be used, modified, copied, distributed, and sold, in
 * both source and binary form provided that the above copyright, these
 * terms and the following disclaimer are retained.  The name of the author
 * and/or the contributor may not be used to endorse or promote products
 * derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND THE CONTRIBUTOR ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR THE CONTRIBUTOR BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION.
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Portions copyright (C) 1993, David Greenman.  This software may be used,
 * modified, copied, distributed, and sold, in both source and binary form
 * provided that the above copyright and these terms are retained.  Under no
 * circumstances is the author responsible for the proper functioning of this
 * software, nor does the author assume any responsibility for damages
 * incurred with its use.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/socket.h>
#include <sys/syslog.h>

#include <net/if.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/ic/mb86960reg.h>
#include <dev/ic/mb86960var.h>

#include <dev/isa/isavar.h>
#include <dev/isa/if_fereg.h>	/* XXX */

int	fmv_match __P((struct device *, struct cfdata *, void *));
void	fmv_attach __P((struct device *, struct device *, void *));

struct fmv_softc {
	struct	mb86960_softc sc_mb86960;	/* real "mb86960" softc */

	/* ISA-specific goo. */
	void	*sc_ih;				/* interrupt cookie */
};

struct cfattach fmv_ca = {
	sizeof(struct fmv_softc), fmv_match, fmv_attach
};

#if NetBSD <= 199712
struct cfdriver fmv_cd = {
	NULL, "fmv", DV_IFNET
};
#endif

struct fe_simple_probe_struct {
	u_char port;	/* Offset from the base I/O address. */
	u_char mask;	/* Bits to be checked. */
	u_char bits;	/* Values to be compared against. */
};

static __inline__ int fe_simple_probe __P((bus_space_tag_t, 
    bus_space_handle_t, struct fe_simple_probe_struct const *));
static int fmv_find __P((bus_space_tag_t, bus_space_handle_t, int *,
    int *));
static int fmv_detect __P((bus_space_tag_t, bus_space_handle_t,
    u_int8_t enaddr[ETHER_ADDR_LEN]));

static int const fmv_iomap[8] = {
	0x220, 0x240, 0x260, 0x280, 0x2A0, 0x2C0, 0x300, 0x340
};
#define NFMV_IOMAP (sizeof (fmv_iomap) / sizeof (fmv_iomap[0]))
#define FMV_NPORTS 0x20

/*
 * Hardware probe routines.
 */

/*
 * Determine if the device is present.
 */
int
fmv_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct isa_attach_args *ia = aux;
	bus_space_tag_t iot = ia->ia_iot;
	bus_space_handle_t ioh;
	int i, iobase, irq, rv = 0;
	u_int8_t myea[ETHER_ADDR_LEN];

	/* Disallow wildcarded values. */
	if (ia->ia_iobase == ISACF_PORT_DEFAULT)
		return (0);

	/*
	 * See if the sepcified address is valid for FMV-180 series.
	 */
	for (i = 0; i < NFMV_IOMAP; i++)
		if (fmv_iomap[i] == ia->ia_iobase)
			break;
	if (i == NFMV_IOMAP) {
#ifdef FMV_DEBUG
		printf("fmv_match: unknown iobase 0x%x\n", ia->ia_iobase);
#endif
		return (0);
	}

	/* Map i/o space. */
	if (bus_space_map(iot, ia->ia_iobase, FMV_NPORTS, 0, &ioh)) {
#ifdef FMV_DEBUG
		printf("fmv_match: couldn't map iospace 0x%x\n",
		    ia->ia_iobase);
#endif
		return (0);
	}

	if (fmv_find(iot, ioh, &iobase, &irq) == 0) {
#ifdef FMV_DEBUG
		printf("fmv_match: fmv_find failed\n");
#endif
		goto out;
	}

	if (iobase != ia->ia_iobase) {
#ifdef FMV_DEBUG
		printf("fmv_match: unexpected iobase in board: 0x%x\n",
		    ia->ia_iobase);
#endif
		goto out;
	}

	if (fmv_detect(iot, ioh, myea) == 0) { /* XXX necessary? */
#ifdef FMV_DEBUG
		printf("fmv_match: fmv_detect failed\n");
#endif
		goto out;
	}

	if (ia->ia_irq != ISACF_IRQ_DEFAULT) {
		if (ia->ia_irq != irq) {
			printf("fmv_match: irq mismatch; "
			    "kernel configured %d != board configured %d\n",
			    ia->ia_irq, irq);
			goto out;
		}
	} else
		ia->ia_irq = irq;

	ia->ia_iosize = FMV_NPORTS;
	ia->ia_msize = 0;
	rv = 1;

 out:
	bus_space_unmap(iot, ioh, FMV_NPORTS);
	return (rv);
}

/*
 * Check for specific bits in specific registers have specific values.
 */
static __inline__ int
fe_simple_probe (iot, ioh, sp)
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	struct fe_simple_probe_struct const *sp;
{
	u_int8_t val;
	struct fe_simple_probe_struct const *p;

	for (p = sp; p->mask != 0; p++) {
		val = bus_space_read_1(iot, ioh, p->port);
		if ((val & p->mask) != p->bits) {
#ifdef FMV_DEBUG
			printf("fe_simple_probe: %x & %x != %x\n",
			    val, p->mask, p->bits);
#endif
			return (0);
		}
	}

	return (1);
}

/*
 * Hardware (vendor) specific probe routines.
 */

/*
 * Probe Fujitsu FMV-180 series boards and get iobase and irq from
 * board.
 */
static int
fmv_find(iot, ioh, iobase, irq)
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	int *iobase, *irq;
{
	u_int8_t config;
	static int const fmv_irqmap[4] = { 3, 7, 10, 15 };
	static struct fe_simple_probe_struct const probe_table[] = {
		{ FE_DLCR2, 0x70, 0x00 },
		{ FE_DLCR4, 0x08, 0x00 },
	    /*	{ FE_DLCR5, 0x80, 0x00 },	Doesn't work. */

		{ FE_FMV0, FE_FMV0_MAGIC_MASK,  FE_FMV0_MAGIC_VALUE },
		{ FE_FMV1, FE_FMV1_CARDID_MASK, FE_FMV1_CARDID_ID   },
		{ FE_FMV3, FE_FMV3_EXTRA_MASK,  FE_FMV3_EXTRA_VALUE },
#if 1
	/*
	 * Test *vendor* part of the station address for Fujitsu.
	 * The test will gain reliability of probe process, but
	 * it rejects FMV-180 clone boards manufactured by other vendors.
	 * We have to turn the test off when such cards are made available.
	 */
		{ FE_FMV4, 0xFF, 0x00 },
		{ FE_FMV5, 0xFF, 0x00 },
		{ FE_FMV6, 0xFF, 0x0E },
#else
	/*
	 * We can always verify the *first* 2 bits (in Ehternet
	 * bit order) are "no multicast" and "no local" even for
	 * unknown vendors.
	 */
		{ FE_FMV4, 0x03, 0x00 },
#endif
		{ 0 }
	};

	/* Simple probe. */
	if (!fe_simple_probe(iot, ioh, probe_table))
		return (0);

	/* Check if our I/O address matches config info on EEPROM. */
	config = bus_space_read_1(iot, ioh, FE_FMV2);
	*iobase = fmv_iomap[(config & FE_FMV2_ADDR) >> FE_FMV2_ADDR_SHIFT];

	/*
	 * Determine which IRQ to be used.
	 *
	 * In this version, we always get an IRQ assignment from the
	 * FMV-180's configuration EEPROM, ignoring that specified in
	 * config file.
	 */
	*irq = fmv_irqmap[(config & FE_FMV2_IRQ) >> FE_FMV2_IRQ_SHIFT];

	return (1);
}

/*
 * Determine type and ethernet address.
 */
static int
fmv_detect(iot, ioh, enaddr)
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	u_int8_t enaddr[ETHER_ADDR_LEN];
{

	/* Get our station address from EEPROM. */
	bus_space_read_region_1(iot, ioh, FE_FMV4, enaddr, ETHER_ADDR_LEN);

	/* Make sure we got a valid station address. */
	if ((enaddr[0] & 0x03) != 0x00 ||
	    (enaddr[0] == 0x00 && enaddr[1] == 0x00 && enaddr[2] == 0x00)) {
#ifdef FMV_DEBUG
		printf("fmv_detect: invalid ethernet address\n");
#endif
		return (0);
	}

	/* Determine the card type. */
	switch (bus_space_read_1(iot, ioh, FE_FMV0) & FE_FMV0_MODEL) {
	case FE_FMV0_MODEL_FMV181:
		return (FE_TYPE_FMV181);
	case FE_FMV0_MODEL_FMV182:
		return (FE_TYPE_FMV182);
	}

#ifdef FMV_DEBUG
	printf("fmv_detect: unknown card\n");
#endif
	/* Unknown card type: maybe a new model, but... */
	return (0);
}

void
fmv_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct fmv_softc *isc = (struct fmv_softc *)self;
	struct mb86960_softc *sc = &isc->sc_mb86960;
	struct isa_attach_args *ia = aux;
	bus_space_tag_t iot = ia->ia_iot;
	bus_space_handle_t ioh;
	u_int8_t myea[ETHER_ADDR_LEN];
	const char *typestr;
	int type;

	printf("\n");

	/* Map i/o space. */
	if (bus_space_map(iot, ia->ia_iobase, FMV_NPORTS, 0, &ioh)) {
		printf("%s: can't map i/o space\n", sc->sc_dev.dv_xname);
		return;
	}

	sc->sc_bst = iot;
	sc->sc_bsh = ioh;

	/* Determine the card type. */
	type = fmv_detect(iot, ioh, myea);
	switch (type) {
	case FE_TYPE_FMV181:
		typestr = "FMV-181";
		break;
	case FE_TYPE_FMV182:
		typestr = "FMV-182";
		break;
	default:
	  	/* Unknown card type: maybe a new model, but... */
		printf("%s: where did the card go?!\n", sc->sc_dev.dv_xname);
		panic("unknown card");
	}

	printf("%s: %s Ethernet\n", sc->sc_dev.dv_xname, typestr);

	/* This interface is always enabled. */
	sc->sc_flags |= FE_FLAGS_ENABLED;

	/*
	 * Minimum initialization of the hardware.
	 * We write into registers; hope I/O ports have no
	 * overlap with other boards.
	 */

	/* Initialize ASIC. */
	bus_space_write_1(iot, ioh, FE_FMV3, 0);
	bus_space_write_1(iot, ioh, FE_FMV10, 0);

	/* Wait for a while.  I'm not sure this is necessary.  FIXME */
	delay(200);

	/*
	 * Do generic MB86960 attach.
	 */
	mb86960_attach(sc, MB86960_TYPE_86960, myea);

	/* Is this really needs to be done here? XXX */
	/* Turn the "master interrupt control" flag of ASIC on. */
	bus_space_write_1(iot, ioh, FE_FMV3, FE_FMV3_ENABLE_FLAG);

	mb86960_config(sc, NULL, 0, 0);

	/* Establish the interrupt handler. */
	isc->sc_ih = isa_intr_establish(ia->ia_ic, ia->ia_irq, IST_EDGE,
	    IPL_NET, mb86960_intr, sc);
	if (isc->sc_ih == NULL)
		printf("%s: couldn't establish interrupt handler\n",
		    sc->sc_dev.dv_xname);
}
