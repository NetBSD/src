/*	$NetBSD: ahc_pci.c,v 1.23 2000/03/16 10:34:33 fvdl Exp $	*/

/*
 * Product specific probe and attach routines for:
 *      3940, 2940, aic7895, aic7890, aic7880,
 *	aic7870, aic7860 and aic7850 SCSI controllers
 *
 * Copyright (c) 1994, 1995, 1996, 1997, 1998, 1999, 2000 Justin T. Gibbs.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer,
 *    without modification.
 * 2. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * Alternatively, this software may be distributed under the terms of the
 * the GNU Public License ("GPL").
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
 * $FreeBSD: src/sys/dev/aic7xxx/ahc_pci.c,v 1.28 2000/02/09 21:00:22 gibbs Exp $
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/queue.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

/* XXXX some i386 on-board chips act weird when memory-mapped */
#ifndef __i386__
#define AHC_ALLOW_MEMIO
#endif

#define AHC_PCI_IOADDR	PCI_MAPREG_START	/* I/O Address */
#define AHC_PCI_MEMADDR	(PCI_MAPREG_START + 4)	/* Mem I/O Address */

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>

#include <dev/ic/aic7xxxvar.h>
#include <dev/ic/smc93cx6var.h>

#include <dev/microcode/aic7xxx/aic7xxx_reg.h>

#ifdef DEBUG
#define bootverbose 1
#else
#define bootverbose 1
#endif

struct ahc_pci_busdata {
	pci_chipset_tag_t pc;
	pcitag_t tag;
	u_int dev;
	u_int func;
};

static __inline u_int64_t
ahc_compose_id(u_int device, u_int vendor, u_int subdevice, u_int subvendor)
{
	u_int64_t id;

	id = subvendor
	   | (subdevice << 16)
	   | ((u_int64_t)vendor << 32)
	   | ((u_int64_t)device << 48);

	return (id);
}

#define ID_ALL_MASK		0xFFFFFFFFFFFFFFFFull
#define ID_DEV_VENDOR_MASK	0xFFFFFFFF00000000ull
#define ID_AIC7850		0x5078900400000000ull
#define ID_AHA_2910_15_20_30C	0x5078900478509004ull
#define ID_AIC7855		0x5578900400000000ull
#define ID_AIC7859		0x3860900400000000ull
#define ID_AHA_2930CU		0x3860900438699004ull
#define ID_AIC7860		0x6078900400000000ull
#define ID_AIC7860C		0x6078900478609004ull
#define ID_AHA_2940AU_0		0x6178900400000000ull
#define ID_AHA_2940AU_1		0x6178900478619004ull
#define ID_AHA_2940AU_CN	0x2178900478219004ull
#define ID_AHA_2930C_VAR	0x6038900438689004ull

#define ID_AIC7870		0x7078900400000000ull
#define ID_AHA_2940		0x7178900400000000ull
#define ID_AHA_3940		0x7278900400000000ull
#define ID_AHA_398X		0x7378900400000000ull
#define ID_AHA_2944		0x7478900400000000ull
#define ID_AHA_3944		0x7578900400000000ull

#define ID_AIC7880		0x8078900400000000ull
#define ID_AIC7880_B		0x8078900478809004ull
#define ID_AHA_2940U		0x8178900400000000ull
#define ID_AHA_3940U		0x8278900400000000ull
#define ID_AHA_2944U		0x8478900400000000ull
#define ID_AHA_3944U		0x8578900400000000ull
#define ID_AHA_398XU		0x8378900400000000ull
#define ID_AHA_4944U		0x8678900400000000ull
#define ID_AHA_2940UB		0x8178900478819004ull
#define ID_AHA_2930U		0x8878900478889004ull
#define ID_AHA_2940U_PRO	0x8778900478879004ull
#define ID_AHA_2940U_CN		0x0078900478009004ull

#define ID_AIC7895		0x7895900478959004ull
#define ID_AIC7895_RAID_PORT	0x7893900478939004ull
#define ID_AHA_2940U_DUAL	0x7895900478919004ull
#define ID_AHA_3940AU		0x7895900478929004ull
#define ID_AHA_3944AU		0x7895900478949004ull

#define ID_AIC7890		0x001F9005000F9005ull
#define ID_AHA_2930U2		0x0011900501819005ull
#define ID_AHA_2940U2B		0x00109005A1009005ull
#define ID_AHA_2940U2_OEM	0x0010900521809005ull
#define ID_AHA_2940U2		0x00109005A1809005ull
#define ID_AHA_2950U2B		0x00109005E1009005ull

#define ID_AIC7892		0x008F9005FFFF9005ull
#define ID_AHA_29160		0x00809005E2A09005ull
#define ID_AHA_29160_CPQ	0x00809005E2A00E11ull
#define ID_AHA_29160N		0x0080900562A09005ull
#define ID_AHA_29160B		0x00809005E2209005ull
#define ID_AHA_19160B		0x0081900562A19005ull

#define ID_AIC7896		0x005F9005FFFF9005ull
#define ID_AHA_3950U2B_0	0x00509005FFFF9005ull
#define ID_AHA_3950U2B_1	0x00509005F5009005ull
#define ID_AHA_3950U2D_0	0x00519005FFFF9005ull
#define ID_AHA_3950U2D_1	0x00519005B5009005ull

#define ID_AIC7899		0x00CF9005FFFF9005ull
#define ID_AHA_3960D		0x00C09005F6209005ull /* AKA AHA-39160 */
#define ID_AHA_3960D_CPQ	0x00C09005F6200E11ull

#define ID_AIC7810		0x1078900400000000ull
#define ID_AIC7815		0x1578900400000000ull

typedef int (ahc_device_setup_t)(struct pci_attach_args *, char *,
				 ahc_chip *, ahc_feature *, ahc_flag *);

static ahc_device_setup_t ahc_aic7850_setup;
static ahc_device_setup_t ahc_aic7855_setup;
static ahc_device_setup_t ahc_aic7859_setup;
static ahc_device_setup_t ahc_aic7860_setup;
static ahc_device_setup_t ahc_aic7870_setup;
static ahc_device_setup_t ahc_aha394X_setup;
static ahc_device_setup_t ahc_aha398X_setup;
static ahc_device_setup_t ahc_aic7880_setup;
static ahc_device_setup_t ahc_2940Pro_setup;
static ahc_device_setup_t ahc_aha394XU_setup;
static ahc_device_setup_t ahc_aha398XU_setup;
static ahc_device_setup_t ahc_aic7890_setup;
static ahc_device_setup_t ahc_aic7892_setup;
static ahc_device_setup_t ahc_aic7895_setup;
static ahc_device_setup_t ahc_aic7896_setup;
static ahc_device_setup_t ahc_aic7899_setup;
static ahc_device_setup_t ahc_raid_setup;
static ahc_device_setup_t ahc_aha394XX_setup;
static ahc_device_setup_t ahc_aha398XX_setup;

struct ahc_pci_identity {
	u_int64_t		 full_id;
	u_int64_t		 id_mask;
	char			*name;
	ahc_device_setup_t	*setup;
};

struct ahc_pci_identity ahc_pci_ident_table [] =
{
	/* aic7850 based controllers */
	{
		ID_AHA_2910_15_20_30C,
		ID_ALL_MASK,
		"Adaptec 2910/15/20/30C SCSI adapter",
		ahc_aic7850_setup
	},
	/* aic7859 based controllers */
	{
		ID_AHA_2930CU,
		ID_ALL_MASK,
		"Adaptec 2930CU SCSI adapter",
		ahc_aic7859_setup
	},
	/* aic7860 based controllers */
	{
		ID_AHA_2940AU_0 & ID_DEV_VENDOR_MASK,
		ID_DEV_VENDOR_MASK,
		"Adaptec 2940A Ultra SCSI adapter",
		ahc_aic7860_setup
	},
	{
		ID_AHA_2940AU_CN & ID_DEV_VENDOR_MASK,
		ID_DEV_VENDOR_MASK,
		"Adaptec 2940A/CN Ultra SCSI adapter",
		ahc_aic7860_setup
	},
	{
		ID_AHA_2930C_VAR & ID_DEV_VENDOR_MASK,
		ID_DEV_VENDOR_MASK,
		"Adaptec 2930C SCSI adapter (VAR)",
		ahc_aic7860_setup
	},
	/* aic7870 based controllers */
	{
		ID_AHA_2940,
		ID_ALL_MASK,
		"Adaptec 2940 SCSI adapter",
		ahc_aic7870_setup
	},
	{
		ID_AHA_3940,
		ID_ALL_MASK,
		"Adaptec 3940 SCSI adapter",
		ahc_aha394X_setup
	},
	{
		ID_AHA_398X,
		ID_ALL_MASK,
		"Adaptec 398X SCSI RAID adapter",
		ahc_aha398X_setup
	},
	{
		ID_AHA_2944,
		ID_ALL_MASK,
		"Adaptec 2944 SCSI adapter",
		ahc_aic7870_setup
	},
	{
		ID_AHA_3944,
		ID_ALL_MASK,
		"Adaptec 3944 SCSI adapter",
		ahc_aha394X_setup
	},
	/* aic7880 based controllers */
	{
		ID_AHA_2940U & ID_DEV_VENDOR_MASK,
		ID_DEV_VENDOR_MASK,
		"Adaptec 2940 Ultra SCSI adapter",
		ahc_aic7880_setup
	},
	{
		ID_AHA_3940U & ID_DEV_VENDOR_MASK,
		ID_DEV_VENDOR_MASK,
		"Adaptec 3940 Ultra SCSI adapter",
		ahc_aha394XU_setup
	},
	{
		ID_AHA_2944U & ID_DEV_VENDOR_MASK,
		ID_DEV_VENDOR_MASK,
		"Adaptec 2944 Ultra SCSI adapter",
		ahc_aic7880_setup
	},
	{
		ID_AHA_3944U & ID_DEV_VENDOR_MASK,
		ID_DEV_VENDOR_MASK,
		"Adaptec 3944 Ultra SCSI adapter",
		ahc_aha394XU_setup
	},
	{
		ID_AHA_398XU & ID_DEV_VENDOR_MASK,
		ID_DEV_VENDOR_MASK,
		"Adaptec 398X Ultra SCSI RAID adapter",
		ahc_aha398XU_setup
	},
	{
		/*
		 * XXX Don't know the slot numbers
		 * so we can't identify channels
		 */
		ID_AHA_4944U & ID_DEV_VENDOR_MASK,
		ID_DEV_VENDOR_MASK,
		"Adaptec 4944 Ultra SCSI adapter",
		ahc_aic7880_setup
	},
	{
		ID_AHA_2930U & ID_DEV_VENDOR_MASK,
		ID_DEV_VENDOR_MASK,
		"Adaptec 2930 Ultra SCSI adapter",
		ahc_aic7880_setup
	},
	{
		ID_AHA_2940U_PRO & ID_DEV_VENDOR_MASK,
		ID_DEV_VENDOR_MASK,
		"Adaptec 2940 Pro Ultra SCSI adapter",
		ahc_2940Pro_setup
	},
	{
		ID_AHA_2940U_CN & ID_DEV_VENDOR_MASK,
		ID_DEV_VENDOR_MASK,
		"Adaptec 2940/CN Ultra SCSI adapter",
		ahc_aic7880_setup
	},
	/* aic7890 based controllers */
	{
		ID_AHA_2930U2,
		ID_ALL_MASK,
		"Adaptec 2930 Ultra2 SCSI adapter",
		ahc_aic7890_setup
	},
	{
		ID_AHA_2940U2B,
		ID_ALL_MASK,
		"Adaptec 2940B Ultra2 SCSI adapter",
		ahc_aic7890_setup
	},
	{
		ID_AHA_2940U2_OEM,
		ID_ALL_MASK,
		"Adaptec 2940 Ultra2 SCSI adapter (OEM)",
		ahc_aic7890_setup
	},
	{
		ID_AHA_2940U2,
		ID_ALL_MASK,
		"Adaptec 2940 Ultra2 SCSI adapter",
		ahc_aic7890_setup
	},
	{
		ID_AHA_2950U2B,
		ID_ALL_MASK,
		"Adaptec 2950 Ultra2 SCSI adapter",
		ahc_aic7890_setup
	},
	/* aic7892 based controllers */
	{
		ID_AHA_29160,
		ID_ALL_MASK,
		"Adaptec 29160 Ultra160 SCSI adapter",
		ahc_aic7892_setup
	},
	{
		ID_AHA_29160_CPQ,
		ID_ALL_MASK,
		"Adaptec (Compaq OEM) 29160 Ultra160 SCSI adapter",
		ahc_aic7892_setup
	},
	{
		ID_AHA_29160N,
		ID_ALL_MASK,
		"Adaptec 29160N Ultra160 SCSI adapter",
		ahc_aic7892_setup
	},
	{
		ID_AHA_29160B,
		ID_ALL_MASK,
		"Adaptec 29160B Ultra160 SCSI adapter",
		ahc_aic7892_setup
	},
	{
		ID_AHA_19160B,
		ID_ALL_MASK,
		"Adaptec 19160B Ultra160 SCSI adapter",
		ahc_aic7892_setup
	},
	/* aic7895 based controllers */	
	{
		ID_AHA_2940U_DUAL,
		ID_ALL_MASK,
		"Adaptec 2940/DUAL Ultra SCSI adapter",
		ahc_aic7895_setup
	},
	{
		ID_AHA_3940AU,
		ID_ALL_MASK,
		"Adaptec 3940A Ultra SCSI adapter",
		ahc_aic7895_setup
	},
	{
		ID_AHA_3944AU,
		ID_ALL_MASK,
		"Adaptec 3944A Ultra SCSI adapter",
		ahc_aic7895_setup
	},
	/* aic7896/97 based controllers */	
	{
		ID_AHA_3950U2B_0,
		ID_ALL_MASK,
		"Adaptec 3950B Ultra2 SCSI adapter",
		ahc_aic7896_setup
	},
	{
		ID_AHA_3950U2B_1,
		ID_ALL_MASK,
		"Adaptec 3950B Ultra2 SCSI adapter",
		ahc_aic7896_setup
	},
	{
		ID_AHA_3950U2D_0,
		ID_ALL_MASK,
		"Adaptec 3950D Ultra2 SCSI adapter",
		ahc_aic7896_setup
	},
	{
		ID_AHA_3950U2D_1,
		ID_ALL_MASK,
		"Adaptec 3950D Ultra2 SCSI adapter",
		ahc_aic7896_setup
	},
	/* aic7899 based controllers */	
	{
		ID_AHA_3960D,
		ID_ALL_MASK,
		"Adaptec 3960D Ultra160 SCSI adapter",
		ahc_aic7899_setup
	},
	{
		ID_AHA_3960D_CPQ,
		ID_ALL_MASK,
		"Adaptec (Compaq OEM) 3960D Ultra160 SCSI adapter",
		ahc_aic7899_setup
	},
	/* Generic chip probes for devices we don't know 'exactly' */
	{
		ID_AIC7850 & ID_DEV_VENDOR_MASK,
		ID_DEV_VENDOR_MASK,
		"Adaptec aic7850 SCSI adapter",
		ahc_aic7850_setup
	},
	{
		ID_AIC7855 & ID_DEV_VENDOR_MASK,
		ID_DEV_VENDOR_MASK,
		"Adaptec aic7855 SCSI adapter",
		ahc_aic7855_setup
	},
	{
		ID_AIC7859 & ID_DEV_VENDOR_MASK,
		ID_DEV_VENDOR_MASK,
		"Adaptec aic7859 SCSI adapter",
		ahc_aic7859_setup
	},
	{
		ID_AIC7860 & ID_DEV_VENDOR_MASK,
		ID_DEV_VENDOR_MASK,
		"Adaptec aic7860 SCSI adapter",
		ahc_aic7860_setup
	},
	{
		ID_AIC7870 & ID_DEV_VENDOR_MASK,
		ID_DEV_VENDOR_MASK,
		"Adaptec aic7870 SCSI adapter",
		ahc_aic7870_setup
	},
	{
		ID_AIC7880 & ID_DEV_VENDOR_MASK,
		ID_DEV_VENDOR_MASK,
		"Adaptec aic7880 Ultra SCSI adapter",
		ahc_aic7880_setup
	},
	{
		ID_AIC7890 & ID_DEV_VENDOR_MASK,
		ID_DEV_VENDOR_MASK,
		"Adaptec aic7890/91 Ultra2 SCSI adapter",
		ahc_aic7890_setup
	},
	{
		ID_AIC7892 & ID_DEV_VENDOR_MASK,
		ID_DEV_VENDOR_MASK,
		"Adaptec aic7892 Ultra160 SCSI adapter",
		ahc_aic7892_setup
	},
	{
		ID_AIC7895 & ID_DEV_VENDOR_MASK,
		ID_DEV_VENDOR_MASK,
		"Adaptec aic7895 Ultra SCSI adapter",
		ahc_aic7895_setup
	},
	{
		ID_AIC7895_RAID_PORT & ID_DEV_VENDOR_MASK,
		ID_DEV_VENDOR_MASK,
		"Adaptec aic7895 Ultra SCSI adapter (RAID PORT)",
		ahc_aic7895_setup
	},
	{
		ID_AIC7896 & ID_DEV_VENDOR_MASK,
		ID_DEV_VENDOR_MASK,
		"Adaptec aic7896/97 Ultra2 SCSI adapter",
		ahc_aic7896_setup
	},
	{
		ID_AIC7899 & ID_DEV_VENDOR_MASK,
		ID_DEV_VENDOR_MASK,
		"Adaptec aic7899 Ultra160 SCSI adapter",
		ahc_aic7899_setup
	},
	{
		ID_AIC7810 & ID_DEV_VENDOR_MASK,
		ID_DEV_VENDOR_MASK,
		"Adaptec aic7810 RAID memory controller",
		ahc_raid_setup
	},
	{
		ID_AIC7815 & ID_DEV_VENDOR_MASK,
		ID_DEV_VENDOR_MASK,
		"Adaptec aic7815 RAID memory controller",
		ahc_raid_setup
	}
};

static const int ahc_num_pci_devs =
	sizeof(ahc_pci_ident_table) / sizeof(*ahc_pci_ident_table);
		
#define AHC_394X_SLOT_CHANNEL_A	4
#define AHC_394X_SLOT_CHANNEL_B	5

#define AHC_398X_SLOT_CHANNEL_A	4
#define AHC_398X_SLOT_CHANNEL_B	8
#define AHC_398X_SLOT_CHANNEL_C	12

#define	DEVCONFIG		0x40
#define		SCBSIZE32	0x00010000	/* aic789X only */
#define		MPORTMODE	0x00000400	/* aic7870 only */
#define		RAMPSM		0x00000200	/* aic7870 only */
#define		VOLSENSE	0x00000100
#define		SCBRAMSEL	0x00000080
#define		MRDCEN		0x00000040
#define		EXTSCBTIME	0x00000020	/* aic7870 only */
#define		EXTSCBPEN	0x00000010	/* aic7870 only */
#define		BERREN		0x00000008
#define		DACEN		0x00000004
#define		STPWLEVEL	0x00000002
#define		DIFACTNEGEN	0x00000001	/* aic7870 only */

#define	CSIZE_LATTIME		0x0c
#define		CACHESIZE	0x0000003f	/* only 5 bits */
#define		LATTIME		0x0000ff00

static struct ahc_pci_identity *ahc_find_pci_device(pcireg_t, pcireg_t);
static int ahc_ext_scbram_present(struct ahc_softc *ahc);
static void ahc_ext_scbram_config(struct ahc_softc *ahc, int enable,
				  int pcheck, int fast);
static void ahc_probe_ext_scbram(struct ahc_softc *ahc);

int ahc_pci_probe __P((struct device *, struct cfdata *, void *));
void ahc_pci_attach __P((struct device *, struct device *, void *));

/* Exported for use in the ahc_intr routine */
int ahc_pci_intr(struct ahc_softc *ahc);

struct cfattach ahc_pci_ca = {
        sizeof(struct ahc_softc), ahc_pci_probe, ahc_pci_attach
};

static struct ahc_pci_identity *
ahc_find_pci_device(id, subid)
	pcireg_t id, subid;
{
	u_int64_t  full_id;
	struct	   ahc_pci_identity *entry;
	u_int	   i;

	full_id = ahc_compose_id(PCI_PRODUCT(id), PCI_VENDOR(id),
				 PCI_PRODUCT(subid), PCI_VENDOR(subid));

	for (i = 0; i < ahc_num_pci_devs; i++) {
		entry = &ahc_pci_ident_table[i];
		if (entry->full_id == (full_id & entry->id_mask))
			return (entry);
	}
	return (NULL);
}

int
ahc_pci_probe(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct pci_attach_args *pa = aux;
	struct	   ahc_pci_identity *entry;
	pcireg_t   subid;

	subid = pci_conf_read(pa->pa_pc, pa->pa_tag, PCI_SUBSYS_ID_REG);
	entry = ahc_find_pci_device(pa->pa_id, subid);
	return entry != NULL ? 1 : 0;
}

void
ahc_pci_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct pci_attach_args *pa = aux;
	struct		   ahc_pci_identity *entry;
	struct		   ahc_softc *ahc = (void *)self;
	pcireg_t	   command;
	ahc_chip	   ahc_t = AHC_NONE;
	ahc_feature	   ahc_fe = AHC_FENONE;
	ahc_flag	   ahc_f = AHC_FNONE;
	u_int		   our_id = 0;
	u_int		   sxfrctl1;
	u_int		   scsiseq;
	int		   error;
	char		   channel;
	pcireg_t	   subid;
	int		   ioh_valid, memh_valid;
	bus_space_tag_t st, iot;
	bus_space_handle_t sh, ioh;
#ifdef AHC_ALLOW_MEMIO
	bus_space_tag_t memt;
	bus_space_handle_t memh;
#endif
	pci_intr_handle_t ih;
	const char *intrstr;
	struct ahc_pci_busdata *bd;

	command = pci_conf_read(pa->pa_pc, pa->pa_tag, PCI_COMMAND_STATUS_REG);
	subid = pci_conf_read(pa->pa_pc, pa->pa_tag, PCI_SUBSYS_ID_REG);
	entry = ahc_find_pci_device(pa->pa_id, subid);
	if (entry == NULL)
		return;
	error = entry->setup(pa, &channel, &ahc_t, &ahc_fe, &ahc_f);
	if (error != 0)
		return;

	ioh_valid = memh_valid = 0;

#ifdef AHC_ALLOW_MEMIO
	memh_valid = (pci_mapreg_map(pa, AHC_PCI_MEMADDR,
	    PCI_MAPREG_TYPE_MEM | PCI_MAPREG_MEM_TYPE_32BIT, 0,
	    &memt, &memh, NULL, NULL) == 0);
#endif
	ioh_valid = (pci_mapreg_map(pa, AHC_PCI_IOADDR,
	    PCI_MAPREG_TYPE_IO, 0, &iot, &ioh, NULL, NULL) == 0);

	if (ioh_valid) {
		st = iot;
		sh = ioh;
#ifdef AHC_ALLOW_MEMIO
	} else if (memh_valid) {
		st = memt;
		sh = memh;
#endif
	} else {
		printf(": unable to map registers\n");
		return;
	}

	printf("\n");
		

	/* Ensure busmastering is enabled */
	command |= PCI_COMMAND_MASTER_ENABLE;;
	pci_conf_write(pa->pa_pc, pa->pa_tag, PCI_COMMAND_STATUS_REG, command);

	/* On all PCI adapters, we allow SCB paging */
	ahc_f |= AHC_PAGESCBS;
	if (ahc_alloc(ahc, sh, st, pa->pa_dmat,
	    ahc_t|AHC_PCI, ahc_fe, ahc_f) < 0)
		return;

	bd = malloc(sizeof (struct ahc_pci_busdata), M_DEVBUF, M_NOWAIT);
	if (bd == NULL) {
		printf(": unable to allocate bus-specific data\n");
		return;
	}

	bd->pc = pa->pa_pc;
	bd->tag = pa->pa_tag;
	bd->func = pa->pa_function;
	bd->dev = pa->pa_device;

	ahc->bus_data = bd;
	ahc->bus_intr = ahc_pci_intr;
	ahc->channel = channel;

	/* Remeber how the card was setup in case there is no SEEPROM */
	ahc_outb(ahc, HCNTRL, ahc->pause);
	if ((ahc->features & AHC_ULTRA2) != 0)
		our_id = ahc_inb(ahc, SCSIID_ULTRA2) & OID;
	else
		our_id = ahc_inb(ahc, SCSIID) & OID;
	sxfrctl1 = ahc_inb(ahc, SXFRCTL1) & STPWEN;
	scsiseq = ahc_inb(ahc, SCSISEQ);

	if (ahc_reset(ahc) != 0) {
		/* Failed */
		ahc_free(ahc);
		return;
	}

	if ((ahc->features & AHC_DT) != 0) {
		u_int optionmode;
		u_int sfunct;

		/* Perform ALT-Mode Setup */
		sfunct = ahc_inb(ahc, SFUNCT) & ~ALT_MODE;
		ahc_outb(ahc, SFUNCT, sfunct | ALT_MODE);
		optionmode = ahc_inb(ahc, OPTIONMODE);
		printf("OptionMode = %x\n", optionmode);
		ahc_outb(ahc, OPTIONMODE, OPTIONMODE_DEFAULTS);
		/* Send CRC info in target mode every 4K */
		ahc_outb(ahc, TARGCRCCNT, 0);
		ahc_outb(ahc, TARGCRCCNT + 1, 0x10);
		ahc_outb(ahc, SFUNCT, sfunct);

		/* Normal mode setup */
		ahc_outb(ahc, CRCCONTROL1, CRCVALCHKEN|CRCENDCHKEN|CRCREQCHKEN
					  |TARGCRCENDEN|TARGCRCCNTEN);
	}

	if (pci_intr_map(pa->pa_pc, pa->pa_intrtag, pa->pa_intrpin,
			 pa->pa_intrline, &ih)) {
		printf("%s: couldn't map interrupt\n", ahc->sc_dev.dv_xname);
		ahc_free(ahc);
		return;
	}
	intrstr = pci_intr_string(pa->pa_pc, ih);
	ahc->ih = pci_intr_establish(pa->pa_pc, ih, IPL_BIO, ahc_intr, ahc);
	if (ahc->ih == NULL) {
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

	/*
	 * Do aic7880/aic7870/aic7860/aic7850 specific initialization
	 */
	{
		u_int8_t sblkctl;
		u_int dscommand0;

		dscommand0 = ahc_inb(ahc, DSCOMMAND0);
		dscommand0 |= MPARCKEN;
		if ((ahc->features & AHC_ULTRA2) != 0) {

			/*
			 * DPARCKEN doesn't work correctly on
			 * some MBs so don't use it.
			 */
			dscommand0 &= ~(USCBSIZE32|DPARCKEN);
			dscommand0 |= CACHETHEN;
		}

		ahc_outb(ahc, DSCOMMAND0, dscommand0);

		/* See if we have an SEEPROM and perform auto-term */
		check_extport(ahc, &sxfrctl1);

		/*
		 * Take the LED out of diagnostic mode
		 */
		sblkctl = ahc_inb(ahc, SBLKCTL);
		ahc_outb(ahc, SBLKCTL, (sblkctl & ~(DIAGLEDEN|DIAGLEDON)));

		/*
		 * I don't know where this is set in the SEEPROM or by the
		 * BIOS, so we default to 100% on Ultra or slower controllers
		 * and 75% on ULTRA2 controllers.
		 */
		if ((ahc->features & AHC_ULTRA2) != 0) {
			ahc_outb(ahc, DFF_THRSH, RD_DFTHRSH_75|WR_DFTHRSH_75);
		} else {
			ahc_outb(ahc, DSPCISTATUS, DFTHRSH_100);
		}

		if (ahc->flags & AHC_USEDEFAULTS) {
			/*
			 * PCI Adapter default setup
			 * Should only be used if the adapter does not have
			 * an SEEPROM.
			 */
			/* See if someone else set us up already */
			if (scsiseq != 0) {
				printf("%s: Using left over BIOS settings\n",
					ahc_name(ahc));
				ahc->flags &= ~AHC_USEDEFAULTS;
			} else {
				/*
				 * Assume only one connector and always turn
				 * on termination.
				 */
 				our_id = 0x07;
				sxfrctl1 = STPWEN;
			}
			ahc_outb(ahc, SCSICONF, our_id|ENSPCHK|RESET_SCSI);

			ahc->our_id = our_id;
		}
	}

	/*
	 * Take a look to see if we have external SRAM.
	 * We currently do not attempt to use SRAM that is
	 * shared among multiple controllers.
	 */
	ahc_probe_ext_scbram(ahc);


	printf("%s: %s ", ahc_name(ahc),
	       ahc_chip_names[ahc->chip & AHC_CHIPID_MASK]);

	/*
	 * Record our termination setting for the
	 * generic initialization routine.
	 */
	if ((sxfrctl1 & STPWEN) != 0)
		ahc->flags |= AHC_TERM_ENB_A;

	if (ahc_init(ahc)) {
		ahc_free(ahc);
		return;
	}

	ahc_attach(ahc);
}

/*
 * Test for the presense of external sram in an
 * "unshared" configuration.
 */
static int
ahc_ext_scbram_present(struct ahc_softc *ahc)
{
	int ramps;
	int single_user;
	pcireg_t devconfig;
	struct ahc_pci_busdata *bd = ahc->bus_data;

	devconfig = pci_conf_read(bd->pc, bd->tag, DEVCONFIG);
	single_user = (devconfig & MPORTMODE) != 0;

	if ((ahc->features & AHC_ULTRA2) != 0)
		ramps = (ahc_inb(ahc, DSCOMMAND0) & RAMPS) != 0;
	else if ((ahc->chip & AHC_CHIPID_MASK) >= AHC_AIC7870)
		ramps = (devconfig & RAMPSM) != 0;
	else
		ramps = 0;

	if (ramps && single_user)
		return (1);
	return (0);
}

/*
 * Enable external scbram.
 */
static void
ahc_ext_scbram_config(struct ahc_softc *ahc, int enable, int pcheck, int fast)
{
	pcireg_t devconfig;
	struct ahc_pci_busdata *bd = ahc->bus_data;

	if (ahc->features & AHC_MULTI_FUNC) {
		/*
		 * Set the SCB Base addr (highest address bit)
		 * depending on which channel we are.
		 */
		ahc_outb(ahc, SCBBADDR, (u_int8_t)bd->func);
	}

	devconfig = pci_conf_read(bd->pc, bd->tag, DEVCONFIG);
	if ((ahc->features & AHC_ULTRA2) != 0) {
		u_int dscommand0;

		dscommand0 = ahc_inb(ahc, DSCOMMAND0);
		if (enable)
			dscommand0 &= ~INTSCBRAMSEL;
		else
			dscommand0 |= INTSCBRAMSEL;
		ahc_outb(ahc, DSCOMMAND0, dscommand0);
	} else {
		if (fast)
			devconfig &= ~EXTSCBTIME;
		else
			devconfig |= EXTSCBTIME;
		if (enable)
			devconfig &= ~SCBRAMSEL;
		else
			devconfig |= SCBRAMSEL;
	}
	if (pcheck)
		devconfig |= EXTSCBPEN;
	else
		devconfig &= ~EXTSCBPEN;

	pci_conf_write(bd->pc, bd->tag, DEVCONFIG, devconfig);
}

/*
 * Take a look to see if we have external SRAM.
 * We currently do not attempt to use SRAM that is
 * shared among multiple controllers.
 */
static void
ahc_probe_ext_scbram(struct ahc_softc *ahc)
{
	int num_scbs;
	int test_num_scbs;
	int enable;
	int pcheck;
	int fast;

	if (ahc_ext_scbram_present(ahc) == 0)
		return;

	/*
	 * Probe for the best parameters to use.
	 */
	enable = FALSE;
	pcheck = FALSE;
	fast = FALSE;
	ahc_ext_scbram_config(ahc, /*enable*/TRUE, pcheck, fast);
	num_scbs = ahc_probe_scbs(ahc);
	if (num_scbs == 0) {
		/* The SRAM wasn't really present. */
		goto done;
	}
	enable = TRUE;

	/*
	 * Clear any outstanding parity error
	 * and ensure that parity error reporting
	 * is enabled.
	 */
	ahc_outb(ahc, SEQCTL, 0);
	ahc_outb(ahc, CLRINT, CLRPARERR);
	ahc_outb(ahc, CLRINT, CLRBRKADRINT);

	/* Now see if we can do parity */
	ahc_ext_scbram_config(ahc, enable, /*pcheck*/TRUE, fast);
	num_scbs = ahc_probe_scbs(ahc);
	if ((ahc_inb(ahc, INTSTAT) & BRKADRINT) == 0
	 || (ahc_inb(ahc, ERROR) & MPARERR) == 0)
		pcheck = TRUE;

	/* Clear any resulting parity error */
	ahc_outb(ahc, CLRINT, CLRPARERR);
	ahc_outb(ahc, CLRINT, CLRBRKADRINT);

	/* Now see if we can do fast timing */
	ahc_ext_scbram_config(ahc, enable, pcheck, /*fast*/TRUE);
	test_num_scbs = ahc_probe_scbs(ahc);
	if (test_num_scbs == num_scbs
	 && ((ahc_inb(ahc, INTSTAT) & BRKADRINT) == 0
	  || (ahc_inb(ahc, ERROR) & MPARERR) == 0))
		fast = TRUE;

done:
	/*
	 * Disable parity error reporting until we
	 * can load instruction ram.
	 */
	ahc_outb(ahc, SEQCTL, PERRORDIS|FAILDIS);
	/* Clear any latched parity error */
	ahc_outb(ahc, CLRINT, CLRPARERR);
	ahc_outb(ahc, CLRINT, CLRBRKADRINT);
	if (bootverbose && enable) {
		printf("%s: External SRAM, %s access%s\n",
		       ahc_name(ahc), fast ? "fast" : "slow", 
		       pcheck ? ", parity checking enabled" : "");
		       
	}
	ahc_ext_scbram_config(ahc, enable, pcheck, fast);
}

#define	DPE	PCI_STATUS_PARITY_DETECT
#define SSE	PCI_STATUS_SPECIAL_ERROR
#define	RMA	PCI_STATUS_MASTER_ABORT
#define	RTA	PCI_STATUS_MASTER_TARGET_ABORT
#define STA	PCI_STATUS_TARGET_TARGET_ABORT
#define DPR	PCI_STATUS_PARITY_ERROR

int
ahc_pci_intr(struct ahc_softc *ahc)
{
	pcireg_t status1;
	struct ahc_pci_busdata *bd = ahc->bus_data;

	if ((ahc_inb(ahc, ERROR) & PCIERRSTAT) == 0)
		return 0;

	status1 = pci_conf_read(bd->pc, bd->tag, PCI_COMMAND_STATUS_REG);

	if (status1 & DPE) {
		printf("%s: Data Parity Error Detected during address "
		       "or write data phase\n", ahc_name(ahc));
	}
	if (status1 & SSE) {
		printf("%s: Signal System Error Detected\n", ahc_name(ahc));
	}
	if (status1 & RMA) {
		printf("%s: Received a Master Abort\n", ahc_name(ahc));
	}
	if (status1 & RTA) {
		printf("%s: Received a Target Abort\n", ahc_name(ahc));
	}
	if (status1 & STA) {
		printf("%s: Signaled a Target Abort\n", ahc_name(ahc));
	}
	if (status1 & DPR) {
		printf("%s: Data Parity Error has been reported via PERR#\n",
		       ahc_name(ahc));
	}
	if ((status1 & (DPE|SSE|RMA|RTA|STA|DPR)) == 0) {
		printf("%s: Latched PCIERR interrupt with "
		       "no status bits set\n", ahc_name(ahc)); 
	}
	pci_conf_write(bd->pc, bd->tag, PCI_COMMAND_STATUS_REG, status1);

	if (status1 & (DPR|RMA|RTA)) {
		ahc_outb(ahc, CLRINT, CLRPARERR);
	}

	return 1;
}

static int
ahc_aic7850_setup(struct pci_attach_args *pa, char *channel,
		  ahc_chip *chip, ahc_feature *features, ahc_flag *flags)
{
	*channel = 'A';
	*chip = AHC_AIC7850;
	*features = AHC_AIC7850_FE;
	return (0);
}

static int
ahc_aic7855_setup(struct pci_attach_args *pa, char *channel,
		  ahc_chip *chip, ahc_feature *features, ahc_flag *flags)
{
	*channel = 'A';
	*chip = AHC_AIC7855;
	*features = AHC_AIC7855_FE;
	return (0);
}

static int
ahc_aic7859_setup(struct pci_attach_args *pa, char *channel,
		  ahc_chip *chip, ahc_feature *features, ahc_flag *flags)
{
	*channel = 'A';
	*chip = AHC_AIC7859;
	*features = AHC_AIC7859_FE;
	return (0);
}

static int
ahc_aic7860_setup(struct pci_attach_args *pa, char *channel,
		  ahc_chip *chip, ahc_feature *features, ahc_flag *flags)
{
	*channel = 'A';
	*chip = AHC_AIC7860;
	*features = AHC_AIC7860_FE;
	return (0);
}

static int
ahc_aic7870_setup(struct pci_attach_args *pa, char *channel,
		  ahc_chip *chip, ahc_feature *features, ahc_flag *flags)
{
	*channel = 'A';
	*chip = AHC_AIC7870;
	*features = AHC_AIC7870_FE;
	return (0);
}

static int
ahc_aha394X_setup(struct pci_attach_args *pa, char *channel,
		  ahc_chip *chip, ahc_feature *features, ahc_flag *flags)
{
	int error;

	error = ahc_aic7870_setup(pa, channel, chip, features, flags);
	if (error == 0)
		error = ahc_aha394XX_setup(pa, channel, chip, features, flags);
	return (error);
}

static int
ahc_aha398X_setup(struct pci_attach_args *pa, char *channel,
		  ahc_chip *chip, ahc_feature *features, ahc_flag *flags)
{
	int error;

	error = ahc_aic7870_setup(pa, channel, chip, features, flags);
	if (error == 0)
		error = ahc_aha398XX_setup(pa, channel, chip, features, flags);
	return (error);
}

static int
ahc_aic7880_setup(struct pci_attach_args *pa, char *channel,
		  ahc_chip *chip, ahc_feature *features, ahc_flag *flags)
{
	*channel = 'A';
	*chip = AHC_AIC7880;
	*features = AHC_AIC7880_FE;
	return (0);
}

static int
ahc_2940Pro_setup(struct pci_attach_args *pa, char *channel,
		  ahc_chip *chip, ahc_feature *features, ahc_flag *flags)
{
	int error;

	*flags |= AHC_INT50_SPEEDFLEX;
	error = ahc_aic7880_setup(pa, channel, chip, features, flags);
	return (0);
}

static int
ahc_aha394XU_setup(struct pci_attach_args *pa, char *channel,
		   ahc_chip *chip, ahc_feature *features, ahc_flag *flags)
{
	int error;

	error = ahc_aic7880_setup(pa, channel, chip, features, flags);
	if (error == 0)
		error = ahc_aha394XX_setup(pa, channel, chip, features, flags);
	return (error);
}

static int
ahc_aha398XU_setup(struct pci_attach_args *pa, char *channel,
		   ahc_chip *chip, ahc_feature *features, ahc_flag *flags)
{
	int error;

	error = ahc_aic7880_setup(pa, channel, chip, features, flags);
	if (error == 0)
		error = ahc_aha398XX_setup(pa, channel, chip, features, flags);
	return (error);
}

static int
ahc_aic7890_setup(struct pci_attach_args *pa, char *channel,
		  ahc_chip *chip, ahc_feature *features, ahc_flag *flags)
{
	*channel = 'A';
	*chip = AHC_AIC7890;
	*features = AHC_AIC7890_FE;
	*flags |= AHC_NEWEEPROM_FMT;
	return (0);
}

static int
ahc_aic7892_setup(struct pci_attach_args *pa, char *channel,
		  ahc_chip *chip, ahc_feature *features, ahc_flag *flags)
{
	*channel = 'A';
	*chip = AHC_AIC7892;
	*features = AHC_AIC7892_FE;
	*flags |= AHC_NEWEEPROM_FMT;
	return (0);
}

static int
ahc_aic7895_setup(struct pci_attach_args *pa, char *channel,
		  ahc_chip *chip, ahc_feature *features, ahc_flag *flags)
{
	pcireg_t devconfig;

	*channel = pa->pa_function == 1 ? 'B' : 'A';
	*chip = AHC_AIC7895;
	/* The 'C' revision of the aic7895 has a few additional features */
	if (PCI_REVISION(pa->pa_class) >= 4)
		*features = AHC_AIC7895C_FE;
	else
		*features = AHC_AIC7895_FE;
	*flags |= AHC_NEWEEPROM_FMT;
	devconfig = pci_conf_read(pa->pa_pc, pa->pa_tag, DEVCONFIG);
	devconfig &= ~SCBSIZE32;
	pci_conf_write(pa->pa_pc, pa->pa_tag, DEVCONFIG, devconfig);
	return (0);
}

static int
ahc_aic7896_setup(struct pci_attach_args *pa, char *channel,
		  ahc_chip *chip, ahc_feature *features, ahc_flag *flags)
{
	*channel = pa->pa_function == 1 ? 'B' : 'A';
	*chip = AHC_AIC7896;
	*features = AHC_AIC7896_FE;
	*flags |= AHC_NEWEEPROM_FMT;
	return (0);
}

static int
ahc_aic7899_setup(struct pci_attach_args *pa, char *channel,
		  ahc_chip *chip, ahc_feature *features, ahc_flag *flags)
{
	*channel = pa->pa_function == 1 ? 'B' : 'A';
	*chip = AHC_AIC7899;
	*features = AHC_AIC7899_FE;
	*flags |= AHC_NEWEEPROM_FMT;
	return (0);
}

static int
ahc_raid_setup(struct pci_attach_args *pa, char *channel,
	       ahc_chip *chip, ahc_feature *features, ahc_flag *flags)
{
	printf("RAID functionality unsupported\n");
	return (ENXIO);
}

static int
ahc_aha394XX_setup(struct pci_attach_args *pa, char *channel,
		   ahc_chip *chip, ahc_feature *features, ahc_flag *flags)
{
	switch (pa->pa_device) {
	case AHC_394X_SLOT_CHANNEL_A:
		*channel = 'A';
		break;
	case AHC_394X_SLOT_CHANNEL_B:
		*channel = 'B';
		break;
	default:
		printf("adapter at unexpected slot %d\n"
		       "unable to map to a channel\n",
		       pa->pa_device);
		*channel = 'A';
	}
	return (0);
}

static int
ahc_aha398XX_setup(struct pci_attach_args *pa, char *channel,
		   ahc_chip *chip, ahc_feature *features, ahc_flag *flags)
{
	switch (pa->pa_device) {
	case AHC_398X_SLOT_CHANNEL_A:
		*channel = 'A';
		break;
	case AHC_398X_SLOT_CHANNEL_B:
		*channel = 'B';
		break;
	case AHC_398X_SLOT_CHANNEL_C:
		*channel = 'C';
		break;
	default:
		printf("adapter at unexpected slot %d\n"
		       "unable to map to a channel\n",
		       pa->pa_device);
		*channel = 'A';
	}
	*flags |= AHC_LARGE_SEEPROM;
	return (0);
}
