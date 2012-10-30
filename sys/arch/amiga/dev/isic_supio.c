/*	$NetBSD: isic_supio.c,v 1.18.2.1 2012/10/30 17:18:49 yamt Exp $ */

/*
 *   Copyright (c) 1998,2001 Ignatios Souvatzis. All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 *
 *   1. Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *   3. Neither the name of the author nor the names of any co-contributors
 *      may be used to endorse or promote products derived from this software
 *      without specific prior written permission.
 *   4. Altered versions must be plainly marked as such, and must not be
 *      misrepresented as being the original software and/or documentation.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 *   ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *   ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 *   FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *   DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 *   OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 *   HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *   LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 *   OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 *   SUCH DAMAGE.
 *
 *---------------------------------------------------------------------------
 *
 *	isic_supio.c - Amiga supio pseudo bus frontend for i4b_isic driver
 *	supports:
 *		- VMC		ISDN Blaster	5001/1
 *		- ITH		ISDN MasterII	5000/1
 *		- BSC		ISDN Master	2092/64
 *		- Individual	ISDN Surfer	4626/5
 *		- Zeus		ISDN Link	2189/3
 *
 *	But we attach to the supio, so just see "isicxx"
 *
 *	-is	original implementation [Sun Feb 14 10:29:19 1999]
 *
 *---------------------------------------------------------------------------*/

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: isic_supio.c,v 1.18.2.1 2012/10/30 17:18:49 yamt Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/errno.h>
#include <sys/syslog.h>
#include <sys/device.h>
#include <sys/socket.h>
#include <net/if.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/bus.h>

#include <machine/cpu.h>
#include <machine/intr.h>

#if defined(__NetBSD__) && __NetBSD_Version__ >= 104230000
#include <sys/callout.h>
#endif

#include <amiga/dev/supio.h>

#include <netisdn/i4b_ioctl.h>
#include <netisdn/i4b_trace.h>
#include <netisdn/i4b_global.h>
#include <netisdn/i4b_debug.h>
#include <netisdn/i4b_l2.h>
#include <netisdn/i4b_l1l2.h>
#include <dev/ic/isic_l1.h>
#include <dev/ic/hscx.h>
#include <dev/ic/isac.h>

/* XXX I think the following line should be elsewhere ... -is */
extern const struct isdn_layer1_isdnif_driver isic_std_driver;

/*static*/ int isic_supio_match(device_t, cfdata_t, void *);
/*static*/ void isic_supio_attach(device_t, device_t, void *);

/*static*/ u_int8_t aster_read_reg(struct isic_softc *sc, int what,
		bus_size_t offs);
/*static*/ void aster_write_reg(struct isic_softc *sc, int what,
	bus_size_t offs, u_int8_t data);
/*static*/ void aster_read_fifo(struct isic_softc *sc, int what,
	void *buf, size_t size);
/*static*/ void aster_write_fifo(struct isic_softc *sc, int what,
	const void *data, size_t size);

static int supio_isicattach(struct isic_softc *sc, const char *);

struct isic_supio_softc {
	struct isic_softc	sc_isic;
	struct isr		sc_isr;
	struct bus_space_tag	sc_bst;
};

CFATTACH_DECL_NEW(isic_supio, sizeof(struct isic_supio_softc),
    isic_supio_match, isic_supio_attach, NULL, NULL);

/*
 * Probe card
 */
/*static*/ int
isic_supio_match(device_t parent, cfdata_t cf, void *aux)
{
	struct supio_attach_args *sap = aux;

	/* ARGSUSED */
	return (!strncmp("isic", sap->supio_name, 4));
}

int isic_supio_ipl = 2;
/*
 * Attach the card
 */
/*static*/ void
isic_supio_attach(device_t parent, device_t self, void *aux)
{
	struct isic_supio_softc *ssc = device_private(self);
	struct isic_softc *sc = &ssc->sc_isic;
	struct supio_attach_args *sap = aux;

	bus_space_tag_t bst;
	bus_space_handle_t h;

	int o1, o2;

	sc->sc_dev = self;

	/* setup parameters */
	sc->sc_cardtyp = CARD_TYPEP_BLMASTER;
	sc->sc_num_mappings = 3;

	/* create io mappings */
	MALLOC_MAPS(sc);

	o1 = (sap->supio_name[4]-'0') << 7;
	o2 = (sap->supio_name[5]-'0') << 7;

	bst = sap->supio_iot;
	bus_space_map(bst, sap->supio_iobase, 0x1000, 0, &h);

	/* ISAC */
	sc->sc_maps[0].t = bst;
	sc->sc_maps[0].h = h;
	sc->sc_maps[0].offset = o1;
	sc->sc_maps[0].size = 0;	/* foreign mapping, leave it alone */

	/* HSCX A */
	sc->sc_maps[1].t = bst;
	sc->sc_maps[1].h = h;
	sc->sc_maps[1].offset = o2;
	sc->sc_maps[1].size = 0;	/* foreign mapping, leave it alone */

	/* HSCX B */
	sc->sc_maps[2].t = bst;
	sc->sc_maps[2].h = h;
	sc->sc_maps[2].offset = o2 + 0x40;
	sc->sc_maps[2].size = 0;	/* foreign mapping, leave it alone */

	sc->clearirq = NULL;
        sc->readreg = aster_read_reg;
        sc->writereg = aster_write_reg;
        sc->readfifo = aster_read_fifo;
        sc->writefifo = aster_write_fifo;

        /* setup card type */
        sc->sc_cardtyp = CARD_TYPEP_BLMASTER;
	sc->sc_bustyp = BUS_TYPE_IOM2;

	sc->sc_ipac = 0;
	sc->sc_bfifolen = HSCX_FIFO_LEN;

	/* enable RTS on HSCX A */
	aster_write_reg(sc, ISIC_WHAT_HSCXA, H_MODE, HSCX_MODE_RTS);

	/* MI initialization of card */

	printf("\n");
	supio_isicattach(sc, sap->supio_name+6);

 	ssc->sc_isr.isr_intr = isicintr;
	ssc->sc_isr.isr_arg = sc;
	ssc->sc_isr.isr_ipl = isic_supio_ipl;	/* XXX */
	add_isr(&ssc->sc_isr);
}

/*static*/ void
aster_read_fifo(struct isic_softc *sc, int what, void *buf, size_t size)
{
        bus_space_tag_t t = sc->sc_maps[what].t;
	bus_space_handle_t h = sc->sc_maps[what].h;
	bus_size_t o = sc->sc_maps[what].offset;

	bus_space_read_multi_1(t, h, o, buf, size);
}

/*static*/ void
aster_write_fifo(struct isic_softc *sc, int what, const void *buf, size_t size)
{
        bus_space_tag_t t = sc->sc_maps[what].t;
	bus_space_handle_t h = sc->sc_maps[what].h;
	bus_size_t o = sc->sc_maps[what].offset;

	bus_space_write_multi_1(t, h, o, (const u_int8_t *)buf, size);
}

/*static*/ u_int8_t
aster_read_reg(struct isic_softc *sc, int what, bus_size_t offs)
{
        bus_space_tag_t t = sc->sc_maps[what].t;
        bus_space_handle_t h = sc->sc_maps[what].h;
        bus_size_t o = sc->sc_maps[what].offset;

        return bus_space_read_1(t, h, o + offs);
}

/*static*/ void
aster_write_reg(struct isic_softc *sc, int what, bus_size_t offs, u_int8_t data)
{
        bus_space_tag_t t = sc->sc_maps[what].t;
	bus_space_handle_t h = sc->sc_maps[what].h;
	bus_size_t o = sc->sc_maps[what].offset;

	bus_space_write_1(t, h, o + offs, data);
}

/*---------------------------------------------------------------------------*
 *	card independend attach for pcmcia^Wsupio cards
 *	XXX this should be centralized!
 *---------------------------------------------------------------------------*/

/*
 * parameter and format for message producing e.g. "isic0: "
 * there is no FreeBSD/Amiga, so just:
 */

#define	ISIC_FMT	"%s: "
#define	ISIC_PARM	device_xname(sc->sc_dev)
#define	TERMFMT	"\n"

int
supio_isicattach(struct isic_softc *sc, const char *cardname)
{
  	static const char *ISACversion[] = {
  		"2085 Version A1/A2 or 2086/2186 Version 1.1",
		"2085 Version B1",
		"2085 Version B2",
		"2085 Version V2.3 (B3)",
		"Unknown Version"
	};

	static const char *HSCXversion[] = {
		"82525 Version A1",
		"Unknown (0x01)",
		"82525 Version A2",
		"Unknown (0x03)",
		"82525 Version A3",
		"82525 or 21525 Version 2.1",
		"Unknown Version"
	};

	sc->sc_isac_version = 0;
	sc->sc_isac_version = ((ISAC_READ(I_RBCH)) >> 5) & 0x03;

	switch(sc->sc_isac_version)
	{
		case ISAC_VA:
		case ISAC_VB1:
                case ISAC_VB2:
		case ISAC_VB3:
			break;

		default:
			printf(ISIC_FMT "Error, ISAC version %d unknown!\n",
				ISIC_PARM, sc->sc_isac_version);
			return(0);
			break;
	}

	sc->sc_hscx_version = HSCX_READ(0, H_VSTR) & 0xf;

	switch(sc->sc_hscx_version)
	{
		case HSCX_VA1:
		case HSCX_VA2:
		case HSCX_VA3:
		case HSCX_V21:
			break;

		default:
			printf(ISIC_FMT "Error, HSCX version %d unknown!\n",
				ISIC_PARM, sc->sc_hscx_version);
			return(0);
			break;
	};

	/* ISAC setup */

	isic_isac_init(sc);

	/* HSCX setup */

	isic_bchannel_setup(sc, HSCX_CH_A, BPROT_NONE, 0);
	isic_bchannel_setup(sc, HSCX_CH_B, BPROT_NONE, 0);

	/* setup linktab */

	isic_init_linktab(sc);

	/* set trace level */

	sc->sc_trace = TRACE_OFF;

	sc->sc_state = ISAC_IDLE;

	sc->sc_ibuf = NULL;
	sc->sc_ib = NULL;
	sc->sc_ilen = 0;

	sc->sc_obuf = NULL;
	sc->sc_op = NULL;
	sc->sc_ol = 0;
	sc->sc_freeflag = 0;

	sc->sc_obuf2 = NULL;
	sc->sc_freeflag2 = 0;

#if defined(__FreeBSD__) && __FreeBSD__ >=3
	callout_handle_init(&sc->sc_T3_callout);
	callout_handle_init(&sc->sc_T4_callout);
#endif

	/* init higher protocol layers */

	/* MPH_Status_Ind(sc->sc_unit, STI_ATTACH, sc->sc_cardtyp); */
	isic_attach_bri(sc, cardname, &isic_std_driver);

	/* announce chip versions */

	if(sc->sc_isac_version >= ISAC_UNKN)
	{
		printf(ISIC_FMT "ISAC Version UNKNOWN (VN=0x%x)" TERMFMT,
				ISIC_PARM,
				sc->sc_isac_version);
		sc->sc_isac_version = ISAC_UNKN;
	}
	else
	{
		printf(ISIC_FMT "ISAC %s (IOM-%c)" TERMFMT,
				ISIC_PARM,
				ISACversion[sc->sc_isac_version],
				sc->sc_bustyp == BUS_TYPE_IOM1 ? '1' : '2');
	}

	if(sc->sc_hscx_version >= HSCX_UNKN)
	{
		printf(ISIC_FMT "HSCX Version UNKNOWN (VN=0x%x)" TERMFMT,
				ISIC_PARM,
				sc->sc_hscx_version);
		sc->sc_hscx_version = HSCX_UNKN;
	}
	else
	{
		printf(ISIC_FMT "HSCX %s" TERMFMT,
				ISIC_PARM,
				HSCXversion[sc->sc_hscx_version]);
	}

	return(1);
}

