/*	$NetBSD: ahc_pci.c,v 1.20 2000/01/26 06:04:41 thorpej Exp $	*/

/*
 * Product specific probe and attach routines for:
 *      3940, 2940, aic7880, aic7870, aic7860 and aic7850 SCSI controllers
 *
 * Copyright (c) 1995, 1996 Justin T. Gibbs.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice immediately at the beginning of the file, without modification,
 *    this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * from	Id: aic7870.c,v 1.37 1996/06/08 06:55:55 gibbs Exp
 */

#if defined(__FreeBSD__)
#include <pci.h>
#endif
#if NPCI > 0 || defined(__NetBSD__)
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/queue.h>
#if defined(__NetBSD__)
#include <sys/device.h>
#include <machine/bus.h>
#include <machine/intr.h>
#endif /* defined(__NetBSD__) */

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#if defined (__NetBSD__)
#endif
#include <dev/scsipi/scsiconf.h>

#if defined(__FreeBSD__)

#include <pci/pcireg.h>
#include <pci/pcivar.h>

#include <machine/clock.h>

#include <i386/scsi/aic7xxx.h>
#include <i386/scsi/93cx6.h>

#include <dev/aic7xxx/aic7xxx_reg.h>

#define PCI_BASEADR0	PCI_MAP_REG_START

#elif defined(__NetBSD__)

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

#include <dev/ic/aic7xxxreg.h>
#include <dev/ic/aic7xxxvar.h>

/*
 * Under normal circumstances, these messages are unnecessary
 * and not terribly cosmetic.
 */
#ifdef DEBUG
#define bootverbose	1
#else
#define bootverbose	0
#endif

#define PCI_BASEADR_IO	0x10
#define PCI_BASEADR_MEM	0x14

#endif /* defined(__NetBSD__) */

#define PCI_DEVICE_ID_ADAPTEC_3940U	0x82789004ul
#define PCI_DEVICE_ID_ADAPTEC_2944U	0x84789004ul
#define PCI_DEVICE_ID_ADAPTEC_2940U	0x81789004ul
#define PCI_DEVICE_ID_ADAPTEC_2940UP	0x87789004ul
#define PCI_DEVICE_ID_ADAPTEC_2940AU	0x61789004ul
#define PCI_DEVICE_ID_ADAPTEC_3940	0x72789004ul
#define PCI_DEVICE_ID_ADAPTEC_2944	0x74789004ul
#define PCI_DEVICE_ID_ADAPTEC_2940	0x71789004ul
#define PCI_DEVICE_ID_ADAPTEC_AIC7880	0x80789004ul
#define PCI_DEVICE_ID_ADAPTEC_AIC7870	0x70789004ul
#define PCI_DEVICE_ID_ADAPTEC_AIC7860	0x60789004ul
#define PCI_DEVICE_ID_ADAPTEC_AIC7855	0x55789004ul
#define PCI_DEVICE_ID_ADAPTEC_AIC7850	0x50789004ul

#define	DEVCONFIG		0x40
#define		MPORTMODE	0x00000400ul	/* aic7870 only */
#define		RAMPSM		0x00000200ul	/* aic7870 only */
#define		VOLSENSE	0x00000100ul
#define		SCBRAMSEL	0x00000080ul
#define		MRDCEN		0x00000040ul
#define		EXTSCBTIME	0x00000020ul	/* aic7870 only */
#define		EXTSCBPEN	0x00000010ul	/* aic7870 only */
#define		BERREN		0x00000008ul
#define		DACEN		0x00000004ul
#define		STPWLEVEL	0x00000002ul
#define		DIFACTNEGEN	0x00000001ul	/* aic7870 only */

#define	CSIZE_LATTIME		0x0c
#define		CACHESIZE	0x0000003ful	/* only 5 bits */
#define		LATTIME		0x0000ff00ul

static u_char aic3940_count;

#if defined(__FreeBSD__)

static char* aic7870_probe __P((pcici_t tag, pcidi_t type));
static void aic7870_attach __P((pcici_t config_id, int unit));

static struct  pci_device ahc_pci_driver = {
	"ahc",
        aic7870_probe,
        aic7870_attach,
        &ahc_unit,
	NULL
};

DATA_SET (pcidevice_set, ahc_pci_driver);

static  char*
aic7870_probe (pcici_t tag, pcidi_t type)
{
	switch(type) {
		case PCI_DEVICE_ID_ADAPTEC_3940U:
			return ("Adaptec 3940 Ultra SCSI host adapter");
			break;
		case PCI_DEVICE_ID_ADAPTEC_3940:
			return ("Adaptec 3940 SCSI host adapter");
			break;
		case PCI_DEVICE_ID_ADAPTEC_2944U:
			return ("Adaptec 2944 Ultra SCSI host adapter");
			break;
		case PCI_DEVICE_ID_ADAPTEC_2940U:
			return ("Adaptec 2940 Ultra SCSI host adapter");
			break;
		case PCI_DEVICE_ID_ADAPTEC_2940UP:
			return ("Adaptec 2940 Ultra SCSI Pro host adapter");
			break;
		case PCI_DEVICE_ID_ADAPTEC_2944:
			return ("Adaptec 2944 SCSI host adapter");
			break;
		case PCI_DEVICE_ID_ADAPTEC_2940:
			return ("Adaptec 2940 SCSI host adapter");
			break;
		case PCI_DEVICE_ID_ADAPTEC_2940AU:
			return ("Adaptec 2940A Ultra SCSI host adapter");
			break;
		case PCI_DEVICE_ID_ADAPTEC_AIC7880:
			return ("Adaptec aic7880 Ultra SCSI host adapter");
			break;
		case PCI_DEVICE_ID_ADAPTEC_AIC7870:
			return ("Adaptec aic7870 SCSI host adapter");
			break;
		case PCI_DEVICE_ID_ADAPTEC_AIC7860:
			return ("Adaptec aic7860 SCSI host adapter");
			break;
		case PCI_DEVICE_ID_ADAPTEC_AIC7855:
			return ("Adaptec aic7855 SCSI host adapter");
			break;
		case PCI_DEVICE_ID_ADAPTEC_AIC7850:
			return ("Adaptec aic7850 SCSI host adapter");
			break;
		default:
			break;
	}
	return (0);

}

#elif defined(__NetBSD__)

int ahc_pci_probe __P((struct device *, struct cfdata *, void *));
void ahc_pci_attach __P((struct device *, struct device *, void *));

struct cfattach ahc_pci_ca = {
	sizeof(struct ahc_data), ahc_pci_probe, ahc_pci_attach
};

int
ahc_pci_probe(parent, match, aux)
        struct device *parent;
        struct cfdata *match;
        void *aux; 
{       
        struct pci_attach_args *pa = aux;

	switch (pa->pa_id) {
	case PCI_DEVICE_ID_ADAPTEC_3940U:
	case PCI_DEVICE_ID_ADAPTEC_2944U:
	case PCI_DEVICE_ID_ADAPTEC_2940U:
	case PCI_DEVICE_ID_ADAPTEC_2940UP:
	case PCI_DEVICE_ID_ADAPTEC_2940AU:
	case PCI_DEVICE_ID_ADAPTEC_3940:
	case PCI_DEVICE_ID_ADAPTEC_2944:
	case PCI_DEVICE_ID_ADAPTEC_2940:
	case PCI_DEVICE_ID_ADAPTEC_AIC7880:
	case PCI_DEVICE_ID_ADAPTEC_AIC7870:
	case PCI_DEVICE_ID_ADAPTEC_AIC7860:
	case PCI_DEVICE_ID_ADAPTEC_AIC7855:
	case PCI_DEVICE_ID_ADAPTEC_AIC7850:
		return 1;
	}
	return 0;
}
#endif /* defined(__NetBSD__) */

#if defined(__FreeBSD__)
static void
aic7870_attach(config_id, unit)
	pcici_t config_id;
	int	unit;
#elif defined(__NetBSD__)
void    
ahc_pci_attach(parent, self, aux)
        struct device *parent, *self;
        void *aux;
#endif
{
#if defined(__FreeBSD__)
	u_long io_port;
	int unit = ahc->sc_dev.dv_unit;
#elif defined(__NetBSD__)
	struct pci_attach_args *pa = aux;
	struct ahc_data *ahc = (void *)self;
	bus_space_tag_t st, iot, memt;
	bus_space_handle_t sh, ioh, memh;
	int ioh_valid, memh_valid;
	pci_intr_handle_t ih;
	const char *intrstr;
#endif
	u_long id;
	unsigned opri = 0;
	ahc_type ahc_t = AHC_NONE;
	ahc_flag ahc_f = AHC_FNONE;
#if defined(__FreeBSD__)
	struct ahc_data *ahc;
#endif
	u_char ultra_enb = 0;
	u_char our_id = 0;

#if defined(__FreeBSD__)
        if(!(io_port = pci_conf_read(config_id, PCI_BASEADR0)))
		return;
	/*
	 * The first bit of PCI_BASEADR0 is always
	 * set hence we mask it off.
	 */
	io_port &= 0xfffffffe;
#elif defined(__NetBSD__)
	ioh_valid = (pci_mapreg_map(pa, PCI_BASEADR_IO,
	    PCI_MAPREG_TYPE_IO, 0,
	    &iot, &ioh, NULL, NULL) == 0);
	memh_valid = (pci_mapreg_map(pa, PCI_BASEADR_MEM,
	    PCI_MAPREG_TYPE_MEM | PCI_MAPREG_MEM_TYPE_32BIT, 0,
	    &memt, &memh, NULL, NULL) == 0);

	if (memh_valid) {
		st = memt;
		sh = memh;
	} else if (ioh_valid) {
		st = iot;
		sh = ioh;
	} else {
		printf(": unable to map registers\n");
		return;
	}
	printf("\n");
#endif

#if defined(__FreeBSD__)
	switch ((id = pci_conf_read(config_id, PCI_ID_REG))) {
#elif defined(__NetBSD__)
	switch (id = pa->pa_id) {
#endif
		case PCI_DEVICE_ID_ADAPTEC_3940U:
		case PCI_DEVICE_ID_ADAPTEC_3940:
			if (id == PCI_DEVICE_ID_ADAPTEC_3940U)
				ahc_t = AHC_394U;
			else
				ahc_t = AHC_394;
			aic3940_count++;
			if(!(aic3940_count & 0x01))
				/* Even count implies second channel */
				ahc_f |= AHC_CHNLB;
			break;
		case PCI_DEVICE_ID_ADAPTEC_2944U:
		case PCI_DEVICE_ID_ADAPTEC_2940U:
		case PCI_DEVICE_ID_ADAPTEC_2940UP:
			ahc_t = AHC_294U;
			break;
		case PCI_DEVICE_ID_ADAPTEC_2944:
		case PCI_DEVICE_ID_ADAPTEC_2940:
			ahc_t = AHC_294;
			break;
		case PCI_DEVICE_ID_ADAPTEC_2940AU:
			ahc_t = AHC_294AU;
			break;
		case PCI_DEVICE_ID_ADAPTEC_AIC7880:
			ahc_t = AHC_AIC7880;
			break;
		case PCI_DEVICE_ID_ADAPTEC_AIC7870:
			ahc_t = AHC_AIC7870;
			break;
		case PCI_DEVICE_ID_ADAPTEC_AIC7860:
			ahc_t = AHC_AIC7860;
			break;
		case PCI_DEVICE_ID_ADAPTEC_AIC7855:
		case PCI_DEVICE_ID_ADAPTEC_AIC7850:
			ahc_t = AHC_AIC7850;
			break;
		default:
			break;
	}

	/* On all PCI adapters, we allow SCB paging */
	ahc_f |= AHC_PAGESCBS;

	/* Remeber how the card was setup in case there is no SEEPROM */
#if defined(__FreeBSD__)
	our_id = inb(SCSIID + io_port) & OID;
	if(ahc_t & AHC_ULTRA)
		ultra_enb = inb(SXFRCTL0 + io_port) & ULTRAEN;
#else
	our_id = bus_space_read_1(st, sh, SCSIID) & OID;
	if(ahc_t & AHC_ULTRA)
		ultra_enb = bus_space_read_1(st, sh, SXFRCTL0) & ULTRAEN;
#endif

#if defined(__FreeBSD__)
	ahc_reset(io_port);
#elif defined(__NetBSD__)
	ahc_reset(ahc->sc_dev.dv_xname, st, sh);
#endif

	if(ahc_t & AHC_AIC7870){
#if defined(__FreeBSD__)
		u_long devconfig = pci_conf_read(config_id, DEVCONFIG);
#elif defined(__NetBSD__)
		u_long devconfig =
			pci_conf_read(pa->pa_pc, pa->pa_tag, DEVCONFIG);
#endif

		if(devconfig & (RAMPSM)) {
			/*
			 * External SRAM present.  Have the probe walk
			 * the SCBs to see how much SRAM we have and set
			 * the number of SCBs accordingly.  We have to
			 * turn off SCBRAMSEL to access the external
			 * SCB SRAM.
			 *
			 * It seems that early versions of the aic7870
			 * didn't use these bits, hence the hack for the
			 * 3940 above.  I would guess that recent 3940s
			 * using later aic7870 or aic7880 chips do
			 * actually set RAMPSM.
			 *
			 * The documentation isn't clear, but it sounds
			 * like the value written to devconfig must not
			 * have RAMPSM set.  The second sixteen bits of
			 * the register are R/O anyway, so it shouldn't
			 * affect RAMPSM either way.
			 */
			devconfig &= ~(RAMPSM|SCBRAMSEL);
#if defined(__FreeBSD__)
			pci_conf_write(config_id, DEVCONFIG, devconfig);
#elif defined(__NetBSD__)
			pci_conf_write(pa->pa_pc, pa->pa_tag,
				       DEVCONFIG, devconfig);
#endif
		}
	}

#if defined(__FreeBSD__)
	if(!(ahc = ahc_alloc(unit, io_port, ahc_t, ahc_f)))
		return;  /* XXX PCI code should take return status */

	if(!(pci_map_int(config_id, ahc_intr, (void *)ahc, &bio_imask))) {
		ahc_free(ahc);
		return;
	}
#elif defined(__NetBSD__)
	ahc_construct(ahc, st, sh, pa->pa_dmat, ahc_t, ahc_f);

	if (pci_intr_map(pa->pa_pc, pa->pa_intrtag, pa->pa_intrpin,
			 pa->pa_intrline, &ih)) {
		printf("%s: couldn't map interrupt\n", ahc->sc_dev.dv_xname);
		ahc_free(ahc);
		return;
	}
	intrstr = pci_intr_string(pa->pa_pc, ih);
	ahc->sc_ih = pci_intr_establish(pa->pa_pc, ih, IPL_BIO, ahc_intr, ahc);
	if (ahc->sc_ih == NULL) {
		printf("%s: couldn't establish interrupt",
		       ahc->sc_dev.dv_xname);
		if (intrstr != NULL)
			printf(" at %s", intrstr);
		printf("\n");
		ahc_free(ahc);
		return;
	}
	if (intrstr != NULL)
		printf("%s: interrupting at %s\n", ahc->sc_dev.dv_xname,
		       intrstr);
#endif
	/*
	 * Protect ourself from spurrious interrupts during
	 * intialization.
	 */
	opri = splbio();

	/*
	 * Do aic7870/aic7880/aic7850 specific initialization
	 */
	{
		u_char	sblkctl;
		char	*id_string;

		switch(ahc->type) {
		   case AHC_394U:
		   case AHC_294U:
		   case AHC_AIC7880:
		   {
			id_string = "aic7880 ";
			ahc_load_seeprom(ahc);
			break;
		   }
		   case AHC_394:
		   case AHC_294:
		   case AHC_AIC7870:
		   {
			id_string = "aic7870 ";
			ahc_load_seeprom(ahc);
			break;
		   }
		   case AHC_294AU:
		   case AHC_AIC7860:
		   {
			id_string = "aic7860 ";
			ahc_load_seeprom(ahc);
			break;
		   }
		   case AHC_AIC7850:
		   {
			id_string = "aic7850 ";
			/*
			 * Use defaults, if the chip wasn't initialized by
			 * a BIOS.
			 */
			ahc->flags |= AHC_USEDEFAULTS;
			break;
		   }
		   default:
		   {
			printf("ahc: Unknown controller type.  Ignoring.\n");
			ahc_free(ahc);
			splx(opri);
			return;
		   }
		}

		/*
		 * Take the LED out of diagnostic mode
		 */
		sblkctl = AHC_INB(ahc, SBLKCTL);
		AHC_OUTB(ahc, SBLKCTL, (sblkctl & ~(DIAGLEDEN|DIAGLEDON)));

		/*
		 * I don't know where this is set in the SEEPROM or by the
		 * BIOS, so we default to 100%.
		 */
		AHC_OUTB(ahc, DSPCISTATUS, DFTHRSH_100);

		if(ahc->flags & AHC_USEDEFAULTS) {
			/*
			 * PCI Adapter default setup
			 * Should only be used if the adapter does not have
			 * an SEEPROM.
			 */
			/* See if someone else set us up already */
			u_long i;
		        for(i = TARG_SCRATCH; i < 0x60; i++) {
                        	if(AHC_INB(ahc, i) != 0x00)
					break;
			}
			if(i == TARG_SCRATCH) {
				/*
				 * Try looking for all ones.  You can get
				 * either.
				 */
		        	for (i = TARG_SCRATCH; i < 0x60; i++) {
                        		if(AHC_INB(ahc, i) != 0xff)
						break;
				}
			}
			if((i != 0x60) && (our_id != 0)) {
				printf("%s: Using left over BIOS settings\n",
					ahc_name(ahc));
				ahc->flags &= ~AHC_USEDEFAULTS;
			}
			else
				our_id = 0x07;
			AHC_OUTB(ahc, SCSICONF,
				 (our_id & 0x07)|ENSPCHK|RESET_SCSI);
			/* In case we are a wide card */
			AHC_OUTB(ahc, SCSICONF + 1, our_id);

			if(!ultra_enb || (ahc->flags & AHC_USEDEFAULTS)) {
				/*
				 * If there wasn't a BIOS or the board
				 * wasn't in this mode to begin with, 
				 * turn off ultra.
				 */
				ahc->type &= ~AHC_ULTRA;
			}
		}

		printf("%s: %s", ahc_name(ahc), id_string);
	}

	if(ahc_init(ahc)){
		ahc_free(ahc);
		splx(opri);
		return; /* XXX PCI code should take return status */
	}
	splx(opri);

	ahc_attach(ahc);
}
#endif /* NPCI > 0 */
