/*	$NetBSD: if_ate.c,v 1.23.20.1 2000/06/22 17:07:10 minoura Exp $	*/

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

int	ate_match __P((struct device *, struct cfdata *, void *));
void	ate_attach __P((struct device *, struct device *, void *));

struct ate_softc {
	struct	mb86960_softc sc_mb86960;	/* real "mb86960" softc */

	/* ISA-specific goo. */
	void	*sc_ih;				/* interrupt cookie */
};

struct cfattach ate_ca = {
	sizeof(struct ate_softc), ate_match, ate_attach
};

#if NetBSD <= 199712
struct cfdriver ate_cd = {
	NULL, "ate", DV_IFNET
};
#endif

struct fe_simple_probe_struct {
	u_char port;	/* Offset from the base I/O address. */
	u_char mask;	/* Bits to be checked. */
	u_char bits;	/* Values to be compared against. */
};

static __inline__ int fe_simple_probe __P((bus_space_tag_t, 
    bus_space_handle_t, struct fe_simple_probe_struct const *));
static __inline__ void ate_strobe __P((bus_space_tag_t, bus_space_handle_t));
static void ate_read_eeprom __P((bus_space_tag_t, bus_space_handle_t,
    u_char *));
static int ate_find __P((bus_space_tag_t, bus_space_handle_t, int *,
    int *));
static int ate_detect __P((bus_space_tag_t, bus_space_handle_t,
    u_int8_t enaddr[ETHER_ADDR_LEN]));

static int const ate_iomap[8] = {
	0x260, 0x280, 0x2A0, 0x240, 0x340, 0x320, 0x380, 0x300
};
#define NATE_IOMAP (sizeof (ate_iomap) / sizeof (ate_iomap[0]))
#define ATE_NPORTS 0x20

/*
 * Hardware probe routines.
 */

/*
 * Determine if the device is present.
 */
int
ate_match(parent, match, aux)
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
	 * See if the sepcified address is valid for MB86965A JLI mode.
	 */
	for (i = 0; i < NATE_IOMAP; i++)
		if (ate_iomap[i] == ia->ia_iobase)
			break;
	if (i == NATE_IOMAP) {
#ifdef ATE_DEBUG
		printf("ate_match: unknown iobase 0x%x\n", ia->ia_iobase);
#endif
		return (0);
	}

	/* Map i/o space. */
	if (bus_space_map(iot, ia->ia_iobase, ATE_NPORTS, 0, &ioh)) {
#ifdef ATE_DEBUG
		printf("ate_match: couldn't map iospace 0x%x\n",
		    ia->ia_iobase);
#endif
		return (0);
	}

	if (ate_find(iot, ioh, &iobase, &irq) == 0) {
#ifdef ATE_DEBUG
		printf("ate_match: ate_find failed\n");
#endif
		goto out;
	}

	if (iobase != ia->ia_iobase) {
#ifdef ATE_DEBUG
		printf("ate_match: unexpected iobase in board: 0x%x\n",
		    ia->ia_iobase);
#endif
		goto out;
	}

	if (ate_detect(iot, ioh, myea) == 0) { /* XXX necessary? */
#ifdef ATE_DEBUG
		printf("ate_match: ate_detect failed\n");
#endif
		goto out;
	}

	if (ia->ia_irq != ISACF_IRQ_DEFAULT) {
		if (ia->ia_irq != irq) {
			printf("ate_match: irq mismatch; "
			    "kernel configured %d != board configured %d\n",
			    ia->ia_irq, irq);
			goto out;
		}
	} else
		ia->ia_irq = irq;

	ia->ia_iosize = ATE_NPORTS;
	ia->ia_msize = 0;
	rv = 1;

 out:
	bus_space_unmap(iot, ioh, ATE_NPORTS);
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
#ifdef ATE_DEBUG
			printf("fe_simple_probe: %x & %x != %x\n",
			    val, p->mask, p->bits);
#endif
			return (0);
		}
	}

	return (1);
}

/*
 * Routines to read all bytes from the config EEPROM through MB86965A.
 * I'm not sure what exactly I'm doing here...  I was told just to follow
 * the steps, and it worked.  Could someone tell me why the following
 * code works?  (Or, why all similar codes I tried previously doesn't
 * work.)  FIXME.
 */

static __inline__ void
ate_strobe (iot, ioh)
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
{

	/*
	 * Output same value twice.  To speed-down execution?
	 */
	bus_space_write_1(iot, ioh, FE_BMPR16, FE_B16_SELECT);
	bus_space_write_1(iot, ioh, FE_BMPR16, FE_B16_SELECT);
	bus_space_write_1(iot, ioh, FE_BMPR16, FE_B16_SELECT | FE_B16_CLOCK);
	bus_space_write_1(iot, ioh, FE_BMPR16, FE_B16_SELECT | FE_B16_CLOCK);
	bus_space_write_1(iot, ioh, FE_BMPR16, FE_B16_SELECT);
	bus_space_write_1(iot, ioh, FE_BMPR16, FE_B16_SELECT);
}

static void
ate_read_eeprom(iot, ioh, data)
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	u_char *data;
{
	int n, count;
	u_char val, bit;

	/* Read bytes from EEPROM; two bytes per an iterration. */
	for (n = 0; n < FE_EEPROM_SIZE / 2; n++) {
		/* Reset the EEPROM interface. */
		bus_space_write_1(iot, ioh, FE_BMPR16, 0x00);
		bus_space_write_1(iot, ioh, FE_BMPR17, 0x00);
		bus_space_write_1(iot, ioh, FE_BMPR16, FE_B16_SELECT);

		/* Start EEPROM access. */
		bus_space_write_1(iot, ioh, FE_BMPR17, FE_B17_DATA);
		ate_strobe(iot, ioh);

		/* Pass the iterration count to the chip. */
		count = 0x80 | n;
		for (bit = 0x80; bit != 0x00; bit >>= 1) {
			bus_space_write_1(iot, ioh, FE_BMPR17,
			    (count & bit) ? FE_B17_DATA : 0);
			ate_strobe(iot, ioh);
		}
		bus_space_write_1(iot, ioh, FE_BMPR17, 0x00);

		/* Read a byte. */
		val = 0;
		for (bit = 0x80; bit != 0x00; bit >>= 1) {
			ate_strobe(iot, ioh);
			if (bus_space_read_1(iot, ioh, FE_BMPR17) &
			    FE_B17_DATA)
				val |= bit;
		}
		*data++ = val;

		/* Read one more byte. */
		val = 0;
		for (bit = 0x80; bit != 0x00; bit >>= 1) {
			ate_strobe(iot, ioh);
			if (bus_space_read_1(iot, ioh, FE_BMPR17) &
			    FE_B17_DATA)
				val |= bit;
		}
		*data++ = val;
	}

	/* Make sure the EEPROM is turned off. */
	bus_space_write_1(iot, ioh, FE_BMPR16, 0);
	bus_space_write_1(iot, ioh, FE_BMPR17, 0);

#if ATE_DEBUG >= 3
	/* Report what we got. */
	data -= FE_EEPROM_SIZE;
	log(LOG_INFO, "ate_read_eeprom: EEPROM at %04x:"
	    " %02x%02x%02x%02x %02x%02x%02x%02x -"
	    " %02x%02x%02x%02x %02x%02x%02x%02x -"
	    " %02x%02x%02x%02x %02x%02x%02x%02x -"
	    " %02x%02x%02x%02x %02x%02x%02x%02x\n",
	    (int) ioh,		/* XXX */
	    data[ 0], data[ 1], data[ 2], data[ 3],
	    data[ 4], data[ 5], data[ 6], data[ 7],
	    data[ 8], data[ 9], data[10], data[11],
	    data[12], data[13], data[14], data[15],
	    data[16], data[17], data[18], data[19],
	    data[20], data[21], data[22], data[23],
	    data[24], data[25], data[26], data[27],
	    data[28], data[29], data[30], data[31]);
#endif
}

/*
 * Hardware (vendor) specific probe routines.
 */

/*
 * Probe and initialization for Allied-Telesis AT1700/RE2000 series.
 */
static int
ate_find(iot, ioh, iobase, irq)
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	int *iobase, *irq;
{
	u_char eeprom[FE_EEPROM_SIZE];
	int n;

	static int const irqmap[4][4] = {
		{  3,  4,  5,  9 },
		{ 10, 11, 12, 15 },
		{  3, 11,  5, 15 },
		{ 10, 11, 14, 15 },
	};
	static struct fe_simple_probe_struct const probe_table[] = {
		{ FE_DLCR2,  0x70, 0x00 },
		{ FE_DLCR4,  0x08, 0x00 },
		{ FE_DLCR5,  0x80, 0x00 },
#if 0
		{ FE_BMPR16, 0x1B, 0x00 },
		{ FE_BMPR17, 0x7F, 0x00 },
#endif
		{ 0 }
	};

#if ATE_DEBUG >= 4
	log(LOG_INFO, "ate_find: probe (0x%x) for ATI\n", iobase);
#if 0
	fe_dump(LOG_INFO, sc);
#endif
#endif

	/*
	 * We should test if MB86965A is on the base address now.
	 * Unfortunately, it is very hard to probe it reliably, since
	 * we have no way to reset the chip under software control.
	 * On cold boot, we could check the "signature" bit patterns
	 * described in the Fujitsu document.  On warm boot, however,
	 * we can predict almost nothing about register values.
	 */
	if (!fe_simple_probe(iot, ioh, probe_table))
		return (0);

	/* Check if our I/O address matches config info on 86965. */
	n = (bus_space_read_1(iot, ioh, FE_BMPR19) & FE_B19_ADDR)
	    >> FE_B19_ADDR_SHIFT;
	*iobase = ate_iomap[n];

	/*
	 * We are now almost sure we have an AT1700 at the given
	 * address.  So, read EEPROM through 86965.  We have to write
	 * into LSI registers to read from EEPROM.  I want to avoid it
	 * at this stage, but I cannot test the presense of the chip
	 * any further without reading EEPROM.  FIXME.
	 */
	ate_read_eeprom(iot, ioh, eeprom);

	/* Make sure that config info in EEPROM and 86965 agree. */
	if (eeprom[FE_EEPROM_CONF] != bus_space_read_1(iot, ioh, FE_BMPR19)) {
#ifdef DIAGNOSTIC
		printf("ate_find: "
		    "incorrect configration in eeprom and chip\n");
#endif
		return (0);
	}

	/*
	 * Try to determine IRQ settings.
	 * Different models use different ranges of IRQs.
	 */
	n = (bus_space_read_1(iot, ioh, FE_BMPR19) & FE_B19_IRQ)
	    >> FE_B19_IRQ_SHIFT;
	switch (eeprom[FE_ATI_EEP_REVISION] & 0xf0) {
	case 0x30:
		*irq = irqmap[3][n];
		break;
	case 0x10:
	case 0x50:
		*irq = irqmap[2][n];
		break;
	case 0x40:
	case 0x60:
		if (eeprom[FE_ATI_EEP_MAGIC] & 0x04) {
			*irq = irqmap[1][n];
			break;
		}
	default:
		*irq = irqmap[0][n];
		break;
	}

	return (1);
}

/*
 * Determine type and ethernet address.
 */
static int
ate_detect(iot, ioh, enaddr)
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	u_int8_t enaddr[ETHER_ADDR_LEN];
{
	u_char eeprom[FE_EEPROM_SIZE];
	int type;

	/* Get our station address from EEPROM. */
	ate_read_eeprom(iot, ioh, eeprom);
	bcopy(eeprom + FE_ATI_EEP_ADDR, enaddr, ETHER_ADDR_LEN);

	/* Make sure we got a valid station address. */
	if ((enaddr[0] & 0x03) != 0x00 ||
	    (enaddr[0] == 0x00 && enaddr[1] == 0x00 && enaddr[2] == 0x00)) {
#ifdef ATE_DEBUG
		printf("fmv_detect: invalid ethernet address\n");
#endif
		return (0);
	}

	/*
	 * Determine the card type.
	 */
	switch (eeprom[FE_ATI_EEP_MODEL]) {
	case FE_ATI_MODEL_AT1700T:
		type = FE_TYPE_AT1700T;
		break;
	case FE_ATI_MODEL_AT1700BT:
		type = FE_TYPE_AT1700BT;
		break;
	case FE_ATI_MODEL_AT1700FT:
		type = FE_TYPE_AT1700FT;
		break;
	case FE_ATI_MODEL_AT1700AT:
		type = FE_TYPE_AT1700AT;
		break;
	default:
		type = FE_TYPE_RE2000;
		break;
	}

	return (type);
}

void
ate_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct ate_softc *isc = (struct ate_softc *)self;
	struct mb86960_softc *sc = &isc->sc_mb86960;
	struct isa_attach_args *ia = aux;
	bus_space_tag_t iot = ia->ia_iot;
	bus_space_handle_t ioh;
	u_int8_t myea[ETHER_ADDR_LEN];
	const char *typestr;
	int type;

	printf("\n");

	/* Map i/o space. */
	if (bus_space_map(iot, ia->ia_iobase, ATE_NPORTS, 0, &ioh)) {
		printf("%s: can't map i/o space\n", sc->sc_dev.dv_xname);
		return;
	}

	sc->sc_bst = iot;
	sc->sc_bsh = ioh;

	/* Determine the card type and get ehternet address. */
	type = ate_detect(iot, ioh, myea);
	switch (type) {
	case FE_TYPE_AT1700T:
		typestr = "AT-1700T";
		break;
	case FE_TYPE_AT1700BT:
		typestr = "AT-1700BT";
		break;
	case FE_TYPE_AT1700FT:
		typestr = "AT-1700FT";
		break;
	case FE_TYPE_AT1700AT:
		typestr = "AT-1700AT";
		break;
	case FE_TYPE_RE2000:
		typestr = "unknown (RE-2000?)";
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
	 * Do generic MB86960 attach.
	 */
	mb86960_attach(sc, MB86960_TYPE_86965, myea);

	mb86960_config(sc, NULL, 0, 0);

	/* Establish the interrupt handler. */
	isc->sc_ih = isa_intr_establish(ia->ia_ic, ia->ia_irq, IST_EDGE,
	    IPL_NET, mb86960_intr, sc);
	if (isc->sc_ih == NULL)
		printf("%s: couldn't establish interrupt handler\n",
		    sc->sc_dev.dv_xname);
}
