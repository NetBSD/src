/*	$NetBSD: if_ed_isa.c,v 1.1.2.1 1997/07/30 07:05:32 marc Exp $	*/

/*
 * Device driver for National Semiconductor DS8390/WD83C690 based ethernet
 * adapters.
 *
 * Copyright (c) 1994, 1995 Charles M. Hannum.  All rights reserved.
 *
 * Copyright (C) 1993, David Greenman.  This software may be used, modified,
 * copied, distributed, and sold, in both source and binary form provided that
 * the above copyright and these terms are retained.  Under no circumstances is
 * the author responsible for the proper functioning of this software, nor does
 * the author assume any responsibility for damages incurred with its use.
 *
 * Currently supports the Western Digital/SMC 8003 and 8013 series, the SMC
 * Elite Ultra (8216), the 3Com 3c503, the NE1000 and NE2000, and a variety of
 * similar clones.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/syslog.h>
#include <sys/device.h>
#include <sys/socket.h>

#include <net/if_types.h>
#include <net/if.h>
#include <net/if_ether.h>

#include <machine/cpu.h>
#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>

#include <dev/ic/dp8390reg.h>
/* XXX these will move to dev/ic */
#include <dev/isa/if_edreg.h>
#include <dev/isa/if_edvar.h>

int ed_probe_isa __P((struct device *, void *, void *));
void ed_attach_isa __P((struct device *, struct device *, void *));
int ed_find __P((struct ed_softc *, struct cfdata *,
    struct isa_attach_args *ia));
int ed_find_WD80x3 __P((struct ed_softc *, struct cfdata *,
    struct isa_attach_args *ia));
int ed_find_3Com __P((struct ed_softc *, struct cfdata *,
    struct isa_attach_args *ia));
int ed_find_Novell_isa __P((struct ed_softc *, struct cfdata *,
    struct isa_attach_args *ia));

struct cfattach ed_isa_ca = {
	sizeof(struct ed_softc), ed_probe_isa, ed_attach_isa
};

/*
 * Determine if the device is present.
 */
int
ed_probe_isa(parent, match, aux)
	struct device *parent;
	void *match, *aux;
{
	struct ed_softc *sc = match;

	return (ed_find(match, sc->sc_dev.dv_cfdata, aux));
}

/*
 * Fill in softc (if given), based on device type, cfdata and attach args.
 * Return 1 if successful, 0 otherwise.
 */
int
ed_find(sc, cf, ia)
	struct ed_softc *sc;
	struct cfdata *cf;
	struct isa_attach_args *ia;
{
	if (ed_find_WD80x3(sc, cf, ia))
		return (1);
	if (ed_find_3Com(sc, cf, ia))
		return (1);
	if (ed_find_Novell_isa(sc, cf, ia))
		return (1);
	return (0);
}

int ed_wd584_irq[] = { 9, 3, 5, 7, 10, 11, 15, 4 };
int ed_wd790_irq[] = { IRQUNK, 9, 3, 5, 7, 10, 11, 15 };

/*
 * Probe and vendor-specific initialization routine for SMC/WD80x3 boards.
 */
int
ed_find_WD80x3(sc, cf, ia)
	struct ed_softc *sc;
	struct cfdata *cf;
	struct isa_attach_args *ia;
{
	bus_space_tag_t iot;
	bus_space_tag_t memt;
	bus_space_handle_t ioh;
	bus_space_handle_t delaybah = ia->ia_delaybah;
	bus_space_handle_t memh;
	u_int memsize;
	u_char iptr, isa16bit, sum;
	int i, rv, memfail, mapped_mem = 0;
	int asicbase, nicbase;

	iot = ia->ia_iot;
	memt = ia->ia_memt;
	rv = 0;

	/* Set initial values for width/size. */
	memsize = 8192;
	isa16bit = 0;

	if (bus_space_map(iot, ia->ia_iobase, ED_WD_IO_PORTS, 0, &ioh))
		return (0);

	sc->asic_base = asicbase = 0;
	sc->nic_base = nicbase = asicbase + ED_WD_NIC_OFFSET;
	sc->is790 = 0;

#ifdef TOSH_ETHER
	bus_space_write_1(iot, ioh, asicbase + ED_WD_MSR, ED_WD_MSR_POW);
	delay(10000);
#endif

	/*
	 * Attempt to do a checksum over the station address PROM.  If it
	 * fails, it's probably not a SMC/WD board.  There is a problem with
	 * this, though: some clone WD boards don't pass the checksum test.
	 * Danpex boards for one.
	 */
	for (sum = 0, i = 0; i < 8; ++i)
		sum += bus_space_read_1(iot, ioh, asicbase + ED_WD_PROM + i);

	if (sum != ED_WD_ROM_CHECKSUM_TOTAL) {
		/*
		 * Checksum is invalid.  This often happens with cheap WD8003E
		 * clones.  In this case, the checksum byte (the eighth byte)
		 * seems to always be zero.
		 */
		if (bus_space_read_1(iot, ioh, asicbase + ED_WD_CARD_ID) !=
		    ED_TYPE_WD8003E ||
		    bus_space_read_1(iot, ioh, asicbase + ED_WD_PROM + 7) != 0)
			goto out;
	}

	/* Reset card to force it into a known state. */
#ifdef TOSH_ETHER
	bus_space_write_1(iot, ioh, asicbase + ED_WD_MSR,
	    ED_WD_MSR_RST | ED_WD_MSR_POW);
#else
	bus_space_write_1(iot, ioh, asicbase + ED_WD_MSR, ED_WD_MSR_RST);
#endif
	delay(100);
	bus_space_write_1(iot, ioh, asicbase + ED_WD_MSR,
	    bus_space_read_1(iot, ioh, asicbase + ED_WD_MSR) & ~ED_WD_MSR_RST);
	/* Wait in the case this card is reading it's EEROM. */
	delay(5000);

	sc->vendor = ED_VENDOR_WD_SMC;
	sc->type = bus_space_read_1(iot, ioh, asicbase + ED_WD_CARD_ID);

	switch (sc->type) {
	case ED_TYPE_WD8003S:
		sc->type_str = "WD8003S";
		break;
	case ED_TYPE_WD8003E:
		sc->type_str = "WD8003E";
		break;
	case ED_TYPE_WD8003EB:
		sc->type_str = "WD8003EB";
		break;
	case ED_TYPE_WD8003W:
		sc->type_str = "WD8003W";
		break;
	case ED_TYPE_WD8013EBT:
		sc->type_str = "WD8013EBT";
		memsize = 16384;
		isa16bit = 1;
		break;
	case ED_TYPE_WD8013W:
		sc->type_str = "WD8013W";
		memsize = 16384;
		isa16bit = 1;
		break;
	case ED_TYPE_WD8013EP:		/* also WD8003EP */
		if (bus_space_read_1(iot, ioh, asicbase + ED_WD_ICR)
		    & ED_WD_ICR_16BIT) {
			isa16bit = 1;
			memsize = 16384;
			sc->type_str = "WD8013EP";
		} else
			sc->type_str = "WD8003EP";
		break;
	case ED_TYPE_WD8013WC:
		sc->type_str = "WD8013WC";
		memsize = 16384;
		isa16bit = 1;
		break;
	case ED_TYPE_WD8013EBP:
		sc->type_str = "WD8013EBP";
		memsize = 16384;
		isa16bit = 1;
		break;
	case ED_TYPE_WD8013EPC:
		sc->type_str = "WD8013EPC";
		memsize = 16384;
		isa16bit = 1;
		break;
	case ED_TYPE_SMC8216C:
	case ED_TYPE_SMC8216T:
		sc->type_str = (sc->type == ED_TYPE_SMC8216C) ?
				"SMC8216/SMC8216C" : "SMC8216T";
		bus_space_write_1(iot, ioh, asicbase + ED_WD790_HWR,
			bus_space_read_1(iot, ioh, asicbase + ED_WD790_HWR)
			| ED_WD790_HWR_SWH);
		switch (bus_space_read_1(iot, ioh, asicbase + ED_WD790_RAR) &
		    ED_WD790_RAR_SZ64) {
		case ED_WD790_RAR_SZ64:
			memsize = 65536;
			break;
		case ED_WD790_RAR_SZ32:
			memsize = 32768;
			break;
		case ED_WD790_RAR_SZ16:
			memsize = 16384;
			break;
		case ED_WD790_RAR_SZ8:
			/* 8216 has 16K shared mem -- 8416 has 8K */
			sc->type_str = (sc->type == ED_TYPE_SMC8216C) ?
				"SMC8416C/SMC8416BT" : "SMC8416T";
			memsize = 8192;
			break;
		}
		bus_space_write_1(iot, ioh, asicbase + ED_WD790_HWR,
			bus_space_read_1(iot, ioh,
			asicbase + ED_WD790_HWR) & ~ED_WD790_HWR_SWH);

		isa16bit = 1;
		sc->is790 = 1;
		break;
#ifdef TOSH_ETHER
	case ED_TYPE_TOSHIBA1:
		sc->type_str = "Toshiba1";
		memsize = 32768;
		isa16bit = 1;
		break;
	case ED_TYPE_TOSHIBA4:
		sc->type_str = "Toshiba4";
		memsize = 32768;
		isa16bit = 1;
		break;
#endif
	default:
		sc->type_str = NULL;
		break;
	}
	/*
	 * Make some adjustments to initial values depending on what is found
	 * in the ICR.
	 */
	if (isa16bit && (sc->type != ED_TYPE_WD8013EBT) &&
#ifdef TOSH_ETHER
	    (sc->type != ED_TYPE_TOSHIBA1) && (sc->type != ED_TYPE_TOSHIBA4) &&
#endif
	    ((bus_space_read_1(iot, ioh,
	      asicbase + ED_WD_ICR) & ED_WD_ICR_16BIT) == 0)) {
		isa16bit = 0;
		memsize = 8192;
	}

#ifdef ED_DEBUG
	printf("type=%x type_str=%s isa16bit=%d memsize=%d id_msize=%d\n",
	    sc->type, sc->type_str ?: "unknown", isa16bit, memsize,
	    ia->ia_msize);
	for (i = 0; i < 8; i++)
		printf("%x -> %x\n", i, inb(asicbase + i));
#endif
	/* Allow the user to override the autoconfiguration. */
	if (ia->ia_msize)
		memsize = ia->ia_msize;
	/*
	 * (Note that if the user specifies both of the following flags that
	 * '8-bit' mode intentionally has precedence.)
	 */
	if (cf->cf_flags & ED_FLAGS_FORCE_16BIT_MODE)
		isa16bit = 1;
	if (cf->cf_flags & ED_FLAGS_FORCE_8BIT_MODE)
		isa16bit = 0;

	/*
	 * If possible, get the assigned interrupt number from the card and
	 * use it.
	 */
	if (sc->is790) {
		u_char x;
		/* Assemble together the encoded interrupt number. */
		bus_space_write_1(iot, ioh, ED_WD790_HWR,
		    bus_space_read_1(iot, ioh, ED_WD790_HWR) | ED_WD790_HWR_SWH);
		x = bus_space_read_1(iot, ioh, ED_WD790_GCR);
		iptr = ((x & ED_WD790_GCR_IR2) >> 4) |
		    ((x & (ED_WD790_GCR_IR1|ED_WD790_GCR_IR0)) >> 2);
		bus_space_write_1(iot, ioh, ED_WD790_HWR,
		    bus_space_read_1(iot, ioh, ED_WD790_HWR) & ~ED_WD790_HWR_SWH);
		/*
		 * Translate it using translation table, and check for
		 * correctness.
		 */
		if (ia->ia_irq != IRQUNK) {
			if (ia->ia_irq != ed_wd790_irq[iptr]) {
				printf("%s: irq mismatch; kernel configured %d != board configured %d\n",
				    sc->sc_dev.dv_xname, ia->ia_irq,
				    ed_wd790_irq[iptr]);
				goto out;
			}
		} else
			ia->ia_irq = ed_wd790_irq[iptr];
		/* Enable the interrupt. */
		bus_space_write_1(iot, ioh, ED_WD790_ICR,
		    bus_space_read_1(iot, ioh, ED_WD790_ICR) | ED_WD790_ICR_EIL);
	} else if (sc->type & ED_WD_SOFTCONFIG) {
		/* Assemble together the encoded interrupt number. */
		iptr = (bus_space_read_1(iot, ioh, ED_WD_ICR) & ED_WD_ICR_IR2) |
		    ((bus_space_read_1(iot, ioh, ED_WD_IRR) &
		      (ED_WD_IRR_IR0 | ED_WD_IRR_IR1)) >> 5);
		/*
		 * Translate it using translation table, and check for
		 * correctness.
		 */
		if (ia->ia_irq != IRQUNK) {
			if (ia->ia_irq != ed_wd584_irq[iptr]) {
				printf("%s: irq mismatch; kernel configured %d != board configured %d\n",
				    sc->sc_dev.dv_xname, ia->ia_irq,
				    ed_wd584_irq[iptr]);
				goto out;
			}
		} else
			ia->ia_irq = ed_wd584_irq[iptr];
		/* Enable the interrupt. */
		bus_space_write_1(iot, ioh, ED_WD_IRR,
		    bus_space_read_1(iot, ioh, ED_WD_IRR) | ED_WD_IRR_IEN);
	} else {
		if (ia->ia_irq == IRQUNK) {
			printf("%s: %s does not have soft configuration\n",
			    sc->sc_dev.dv_xname, sc->type_str);
			goto out;
		}
	}

	/* XXX Figure out the shared memory address. */

	sc->isa16bit = isa16bit;
	sc->mem_shared = 1;
	ia->ia_msize = memsize;
	if (bus_space_map(memt, ia->ia_maddr, memsize, 0, &memh))
		goto out;
	mapped_mem = 1;
	sc->mem_start = 0;	/* offset */

	/* Allocate one xmit buffer if < 16k, two buffers otherwise. */
	if ((memsize < 16384) || (cf->cf_flags & ED_FLAGS_NO_MULTI_BUFFERING))
		sc->txb_cnt = 1;
	else
		sc->txb_cnt = 2;

	sc->tx_page_start = ED_WD_PAGE_OFFSET;
	sc->rec_page_start = sc->tx_page_start + sc->txb_cnt * ED_TXBUF_SIZE;
	sc->rec_page_stop = sc->tx_page_start + (memsize >> ED_PAGE_SHIFT);
	sc->mem_ring = sc->mem_start + (sc->rec_page_start << ED_PAGE_SHIFT);
	sc->mem_size = memsize;
	sc->mem_end = sc->mem_start + memsize;

	/* Get station address from on-board ROM. */
	for (i = 0; i < ETHER_ADDR_LEN; ++i)
		sc->sc_enaddr[i] =
		    bus_space_read_1(iot, ioh, asicbase + ED_WD_PROM + i);

	/*
	 * Set upper address bits and 8/16 bit access to shared memory.
	 */
	if (isa16bit) {
		if (sc->is790) {
			sc->wd_laar_proto =
			    bus_space_read_1(iot, ioh, asicbase + ED_WD_LAAR) &
			    ~ED_WD_LAAR_M16EN;
		} else {
			sc->wd_laar_proto =
			    ED_WD_LAAR_L16EN |
			    ((ia->ia_maddr >> 19) &
			    ED_WD_LAAR_ADDRHI);
		}
		bus_space_write_1(iot, ioh, asicbase + ED_WD_LAAR,
		    sc->wd_laar_proto | ED_WD_LAAR_M16EN);
	} else  {
		if ((sc->type & ED_WD_SOFTCONFIG) ||
#ifdef TOSH_ETHER
		    (sc->type == ED_TYPE_TOSHIBA1) ||
		    (sc->type == ED_TYPE_TOSHIBA4) ||
#endif
		    ((sc->type == ED_TYPE_WD8013EBT) && !sc->is790)) {
			sc->wd_laar_proto =
			    ((ia->ia_maddr >> 19) &
			    ED_WD_LAAR_ADDRHI);
			bus_space_write_1(iot, ioh, asicbase + ED_WD_LAAR,
			    sc->wd_laar_proto);
		}
	}

	/*
	 * Set address and enable interface shared memory.
	 */
	if (!sc->is790) {
#ifdef TOSH_ETHER
		bus_space_write_1(iot, ioh, asicbase + ED_WD_MSR + 1,
		    ((ia->ia_maddr >> 8) & 0xe0) | 4);
		bus_space_write_1(iot, ioh, asicbase + ED_WD_MSR + 2,
		    ((ia->ia_maddr >> 16) & 0x0f));
		sc->wd_msr_proto = ED_WD_MSR_POW;
#else
		sc->wd_msr_proto =
		    (ia->ia_maddr >> 13) & ED_WD_MSR_ADDR;
#endif
		sc->cr_proto = ED_CR_RD2;
	} else {
		bus_space_write_1(iot, ioh, asicbase + 0x04,
		    bus_space_read_1(iot, ioh, asicbase + 0x04) | 0x80);
		bus_space_write_1(iot, ioh, asicbase + 0x0b,
		    ((ia->ia_maddr >> 13) & 0x0f) |
		    ((ia->ia_maddr >> 11) & 0x40) |
		    (bus_space_read_1(iot, ioh, asicbase + 0x0b) & 0xb0));
		bus_space_write_1(iot, ioh, asicbase + 0x04,
		    bus_space_read_1(iot, ioh, asicbase + 0x04) & ~0x80);
		sc->wd_msr_proto = 0x00;
		sc->cr_proto = 0;
	}
	bus_space_write_1(iot, ioh, asicbase + ED_WD_MSR,
	    sc->wd_msr_proto | ED_WD_MSR_MENB);

	(void) bus_space_read_1(iot, delaybah, 0);
	(void) bus_space_read_1(iot, delaybah, 0);

	/* Now zero memory and verify that it is clear. */
	if (isa16bit) {
		for (i = 0; i < memsize; i += 2)
			bus_space_write_2(memt, memh, sc->mem_start + i, 0);
	} else {
		for (i = 0; i < memsize; ++i)
			bus_space_write_1(memt, memh, sc->mem_start + i, 0);
	}

	memfail = 0;
	if (isa16bit) {
		for (i = 0; i < memsize; i += 2) {
			if (bus_space_read_2(memt, memh, sc->mem_start + i)) {
				memfail = 1;
				break;
			}
		}
	} else {
		for (i = 0; i < memsize; ++i) {
			if (bus_space_read_1(memt, memh, sc->mem_start + i)) {
				memfail = 1;
				break;
			}
		}
	}

	if (memfail) {
		printf("%s: failed to clear shared memory at %x - "
		    "check configuration\n",
		    sc->sc_dev.dv_xname,
		    (ia->ia_maddr + sc->mem_start + i));

		/* Disable 16 bit access to shared memory. */
		bus_space_write_1(iot, ioh, asicbase + ED_WD_MSR,
		    sc->wd_msr_proto);
		if (isa16bit)
			bus_space_write_1(iot, ioh, asicbase + ED_WD_LAAR,
			    sc->wd_laar_proto);
		(void) bus_space_read_1(iot, delaybah, 0);
		(void) bus_space_read_1(iot, delaybah, 0);
		goto out;
	}

	/*
	 * Disable 16bit access to shared memory - we leave it disabled
	 * so that 1) machines reboot properly when the board is set 16
	 * 16 bit mode and there are conflicting 8bit devices/ROMS in
	 * the same 128k address space as this boards shared memory,
	 * and 2) so that other 8 bit devices with shared memory can be
	 * used in this 128k region, too.
	 */
	bus_space_write_1(iot, ioh, asicbase + ED_WD_MSR, sc->wd_msr_proto);
	if (isa16bit)
		bus_space_write_1(iot, ioh, asicbase + ED_WD_LAAR,
		    sc->wd_laar_proto);
	(void) bus_space_read_1(iot, delaybah, 0);
	(void) bus_space_read_1(iot, delaybah, 0);

	ia->ia_iosize = ED_WD_IO_PORTS;
	rv = 1;

 out:
	/*
	 * XXX Sould always unmap, but we can't yet.
	 * XXX Need to squish "indirect" first.
	 */
	if (rv == 0) {
		bus_space_unmap(iot, ioh, ED_WD_IO_PORTS);
		if (mapped_mem)
			bus_space_unmap(memt, memh, memsize);
	} else {
		/* XXX this is all "indirect" brokenness */
		sc->sc_iot = iot;
		sc->sc_memt = memt;
		sc->sc_ioh = ioh;
		sc->sc_memh = memh;
	}
	return (rv);
}

int ed_3com_iobase[] = {0x2e0, 0x2a0, 0x280, 0x250, 0x350, 0x330, 0x310, 0x300};
int ed_3com_maddr[] = {MADDRUNK, MADDRUNK, MADDRUNK, MADDRUNK, 0xc8000, 0xcc000, 0xd8000, 0xdc000};
#if 0
int ed_3com_irq[] = {IRQUNK, IRQUNK, IRQUNK, IRQUNK, 9, 3, 4, 5};
#endif

/*
 * Probe and vendor-specific initialization routine for 3Com 3c503 boards.
 */
int
ed_find_3Com(sc, cf, ia)
	struct ed_softc *sc;
	struct cfdata *cf;
	struct isa_attach_args *ia;
{
	bus_space_tag_t iot;
	bus_space_tag_t memt;
	bus_space_handle_t ioh;
	bus_space_handle_t memh;
	int i;
	u_int memsize, memfail;
	u_char isa16bit, x;
	int ptr, asicbase, nicbase;

	/*
	 * Hmmm...a 16bit 3Com board has 16k of memory, but only an 8k window
	 * to it.
	 */
	memsize = 8192;

	iot = ia->ia_iot;
	memt = ia->ia_memt;

	if (bus_space_map(iot, ia->ia_iobase, ED_3COM_IO_PORTS, 0, &ioh))
		return (0);

	sc->asic_base = asicbase = ED_3COM_ASIC_OFFSET;
	sc->nic_base = nicbase = ED_3COM_NIC_OFFSET;

	/*
	 * Verify that the kernel configured I/O address matches the board
	 * configured address.
	 *
	 * This is really only useful to see if something that looks like the
	 * board is there; after all, we are already talking it at that
	 * address.
	 */
	x = bus_space_read_1(iot, ioh, asicbase + ED_3COM_BCFR);
	if (x == 0 || (x & (x - 1)) != 0)
		goto err;
	ptr = ffs(x) - 1;
	if (ia->ia_iobase != IOBASEUNK) {
		if (ia->ia_iobase != ed_3com_iobase[ptr]) {
			printf("%s: %s mismatch; kernel configured %x != board configured %x\n",
			    "iobase", sc->sc_dev.dv_xname, ia->ia_iobase,
			    ed_3com_iobase[ptr]);
			goto err;
		}
	} else
		ia->ia_iobase = ed_3com_iobase[ptr];	/* XXX --thorpej */

	x = bus_space_read_1(iot, ioh, asicbase + ED_3COM_PCFR);
	if (x == 0 || (x & (x - 1)) != 0) {
		printf("%s: The 3c503 is not currently supported with memory "
		       "mapping disabled.\n%s: Reconfigure the card to "
		       "enable memory mapping.\n",
		       sc->sc_dev.dv_xname, sc->sc_dev.dv_xname);
		goto err;
	}
	ptr = ffs(x) - 1;
	if (ia->ia_maddr != MADDRUNK) {
		if (ia->ia_maddr != ed_3com_maddr[ptr]) {
			printf("%s: %s mismatch; kernel configured %x != board configured %x\n",
			    "maddr", sc->sc_dev.dv_xname, ia->ia_maddr,
			    ed_3com_maddr[ptr]);
			goto err;
		}
	} else
		ia->ia_maddr = ed_3com_maddr[ptr];

#if 0
	x = bus_space_read_1(iot, ioh, asicbase + ED_3COM_IDCFR) &
	    ED_3COM_IDCFR_IRQ;
	if (x == 0 || (x & (x - 1)) != 0)
		goto out;
	ptr = ffs(x) - 1;
	if (ia->ia_irq != IRQUNK) {
		if (ia->ia_irq != ed_3com_irq[ptr]) {
			printf("%s: irq mismatch; kernel configured %d != board configured %d\n",
			    sc->sc_dev.dv_xname, ia->ia_irq,
			    ed_3com_irq[ptr]);
			goto err;
		}
	} else
		ia->ia_irq = ed_3com_irq[ptr];
#endif

	/*
	 * Reset NIC and ASIC.  Enable on-board transceiver throughout reset
	 * sequence because it'll lock up if the cable isn't connected if we
	 * don't.
	 */
	bus_space_write_1(iot, ioh, asicbase + ED_3COM_CR,
	    ED_3COM_CR_RST | ED_3COM_CR_XSEL);

	/* Wait for a while, then un-reset it. */
	delay(50);

	/*
	 * The 3Com ASIC defaults to rather strange settings for the CR after a
	 * reset - it's important to set it again after the following outb
	 * (this is done when we map the PROM below).
	 */
	bus_space_write_1(iot, ioh, asicbase + ED_3COM_CR, ED_3COM_CR_XSEL);

	/* Wait a bit for the NIC to recover from the reset. */
	delay(5000);

	sc->vendor = ED_VENDOR_3COM;
	sc->type_str = "3c503";
	sc->mem_shared = 1;
	sc->cr_proto = ED_CR_RD2;

	/*
	 * Get station address from on-board ROM.
	 *
	 * First, map ethernet address PROM over the top of where the NIC
	 * registers normally appear.
	 */
	bus_space_write_1(iot, ioh, asicbase + ED_3COM_CR,
	    ED_3COM_CR_EALO | ED_3COM_CR_XSEL);

	for (i = 0; i < ETHER_ADDR_LEN; ++i)
		sc->sc_enaddr[i] = NIC_GET(iot, ioh, nicbase, i);

	/*
	 * Unmap PROM - select NIC registers.  The proper setting of the
	 * tranceiver is set in edinit so that the attach code is given a
	 * chance to set the default based on a compile-time config option.
	 */
	bus_space_write_1(iot, ioh, asicbase + ED_3COM_CR, ED_3COM_CR_XSEL);

	/* Determine if this is an 8bit or 16bit board. */

	/* Select page 0 registers. */
	NIC_PUT(iot, ioh, nicbase, ED_P0_CR,
	    ED_CR_RD2 | ED_CR_PAGE_0 | ED_CR_STP);

	/*
	 * Attempt to clear WTS bit.  If it doesn't clear, then this is a
	 * 16-bit board.
	 */
	NIC_PUT(iot, ioh, nicbase, ED_P0_DCR, 0);

	/* Select page 2 registers. */
	NIC_PUT(iot, ioh, nicbase,
	    ED_P0_CR, ED_CR_RD2 | ED_CR_PAGE_2 | ED_CR_STP);

	/* The 3c503 forces the WTS bit to a one if this is a 16bit board. */
	if (NIC_GET(iot, ioh, nicbase, ED_P2_DCR) & ED_DCR_WTS)
		isa16bit = 1;
	else
		isa16bit = 0;

	/* Select page 0 registers. */
	NIC_PUT(iot, ioh, nicbase, ED_P2_CR,
	    ED_CR_RD2 | ED_CR_PAGE_0 | ED_CR_STP);

	if (bus_space_map(memt, ia->ia_maddr, memsize, 0, &memh))
		goto err;
	sc->mem_start = 0;		/* offset */
	sc->mem_size = memsize;
	sc->mem_end = sc->mem_start + memsize;

	/*
	 * We have an entire 8k window to put the transmit buffers on the
	 * 16-bit boards.  But since the 16bit 3c503's shared memory is only
	 * fast enough to overlap the loading of one full-size packet, trying
	 * to load more than 2 buffers can actually leave the transmitter idle
	 * during the load.  So 2 seems the best value.  (Although a mix of
	 * variable-sized packets might change this assumption.  Nonetheless,
	 * we optimize for linear transfers of same-size packets.)
	 */
	if (isa16bit) {
 		if (cf->cf_flags & ED_FLAGS_NO_MULTI_BUFFERING)
			sc->txb_cnt = 1;
		else
			sc->txb_cnt = 2;

		sc->tx_page_start = ED_3COM_TX_PAGE_OFFSET_16BIT;
		sc->rec_page_start = ED_3COM_RX_PAGE_OFFSET_16BIT;
		sc->rec_page_stop =
		    (memsize >> ED_PAGE_SHIFT) + ED_3COM_RX_PAGE_OFFSET_16BIT;
		sc->mem_ring = sc->mem_start;
	} else {
		sc->txb_cnt = 1;
		sc->tx_page_start = ED_3COM_TX_PAGE_OFFSET_8BIT;
		sc->rec_page_start =
		    ED_TXBUF_SIZE + ED_3COM_TX_PAGE_OFFSET_8BIT;
		sc->rec_page_stop =
		    (memsize >> ED_PAGE_SHIFT) + ED_3COM_TX_PAGE_OFFSET_8BIT;
		sc->mem_ring =
		    sc->mem_start + (ED_TXBUF_SIZE << ED_PAGE_SHIFT);
	}

	sc->isa16bit = isa16bit;

	/*
	 * Initialize GA page start/stop registers.  Probably only needed if
	 * doing DMA, but what the Hell.
	 */
	bus_space_write_1(iot, ioh, asicbase + ED_3COM_PSTR, sc->rec_page_start);
	bus_space_write_1(iot, ioh, asicbase + ED_3COM_PSPR, sc->rec_page_stop);

	/* Set IRQ.  3c503 only allows a choice of irq 3-5 or 9. */
	switch (ia->ia_irq) {
	case 9:
		bus_space_write_1(iot, ioh, asicbase + ED_3COM_IDCFR,
		    ED_3COM_IDCFR_IRQ2);
		break;
	case 3:
		bus_space_write_1(iot, ioh, asicbase + ED_3COM_IDCFR,
		    ED_3COM_IDCFR_IRQ3);
		break;
	case 4:
		bus_space_write_1(iot, ioh, asicbase + ED_3COM_IDCFR,
		    ED_3COM_IDCFR_IRQ4);
		break;
	case 5:
		bus_space_write_1(iot, ioh, asicbase + ED_3COM_IDCFR,
		    ED_3COM_IDCFR_IRQ5);
		break;
	default:
		printf("%s: invalid irq configuration (%d) must be 3-5 or 9 for 3c503\n",
		    sc->sc_dev.dv_xname, ia->ia_irq);
		goto out;
	}

	/*
	 * Initialize GA configuration register.  Set bank and enable shared
	 * mem.
	 */
	bus_space_write_1(iot, ioh, asicbase + ED_3COM_GACFR,
	    ED_3COM_GACFR_RSEL | ED_3COM_GACFR_MBS0);

	/*
	 * Initialize "Vector Pointer" registers. These gawd-awful things are
	 * compared to 20 bits of the address on ISA, and if they match, the
	 * shared memory is disabled. We set them to 0xffff0...allegedly the
	 * reset vector.
	 */
	bus_space_write_1(iot, ioh, asicbase + ED_3COM_VPTR2, 0xff);
	bus_space_write_1(iot, ioh, asicbase + ED_3COM_VPTR1, 0xff);
	bus_space_write_1(iot, ioh, asicbase + ED_3COM_VPTR0, 0x00);

	/* Now zero memory and verify that it is clear. */
	if (isa16bit) {
		for (i = 0; i < memsize; i += 2)
			bus_space_write_2(memt, memh, sc->mem_start + i, 0);
	} else {
		for (i = 0; i < memsize; ++i)
			bus_space_write_1(memt, memh, sc->mem_start + i, 0);
	}

	memfail = 0;
	if (isa16bit) {
		for (i = 0; i < memsize; i += 2) {
			if (bus_space_read_2(memt, memh, sc->mem_start + i)) {
				memfail = 1;
				break;
			}
		}
	} else {
		for (i = 0; i < memsize; ++i) {
			if (bus_space_read_1(memt, memh, sc->mem_start + i)) {
				memfail = 1;
				break;
			}
		}
	}

	if (memfail) {
		printf("%s: failed to clear shared memory at %x - "
		    "check configuration\n",
		    sc->sc_dev.dv_xname,
		    (ia->ia_maddr + sc->mem_start + i));
		goto out;
	}

	ia->ia_msize = memsize;
	ia->ia_iosize = ED_3COM_IO_PORTS;

	/*
	 * XXX Sould always unmap, but we can't yet.
	 * XXX Need to squish "indirect" first.
	 */
	sc->sc_iot = iot;
	sc->sc_memt = memt;
	sc->sc_ioh = ioh;
	sc->sc_memh = memh;
	return 1;

 out:
	bus_space_unmap(memt, memh, memsize);
 err:
	bus_space_unmap(iot, ioh, ED_3COM_IO_PORTS);
	return 0;
}

int
ed_find_Novell_isa(sc, cf, ia)
	struct ed_softc *sc;
	struct cfdata *cf;
	struct isa_attach_args *ia;
{
    int ret; 
    bus_space_handle_t ioh;

    if (bus_space_map(ia->ia_iot, ia->ia_iobase, ED_NOVELL_IO_PORTS, 0, &ioh))
	return (0);

    if ((ret = ed_find_Novell(sc, cf, ia->ia_iot, ioh))) {
	if (ia->ia_irq == IRQUNK) {
	    printf("%s: %s does not have soft configuration\n",
		   sc->sc_dev.dv_xname, sc->type_str);
	    return(0);
	}

	ia->ia_msize = 0;
	ia->ia_iosize = ED_NOVELL_IO_PORTS;
    }

    return(ret);
}

/*
 * Install interface into kernel networking data structures.
 */
void
ed_attach_isa(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct ed_softc *sc = (void *)self;
	struct isa_attach_args *ia = aux;

	sc->sc_delaybah = ia->ia_delaybah;

	edattach(self);

	sc->sc_ih = isa_intr_establish(ia->ia_ic, ia->ia_irq, IST_EDGE,
	    IPL_NET, edintr, sc);
}

