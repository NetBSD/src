/* $NetBSD: pciide.c,v 1.5 2008/04/09 16:26:29 nisimura Exp $ */

#include <sys/param.h>
#include <sys/disklabel.h>

#include <dev/ic/wdcreg.h>
#include <dev/ata/atareg.h>

#include <lib/libsa/stand.h>
#include "globals.h"

/*
 * - no vtophys() translation, vaddr_t == paddr_t. 
 */
#define DEVTOV(pa) 		(uint32_t)(pa)

/*
 * cmdide
 * iteide
 * viaide
 * slide
 */

void *pciide_init(unsigned, unsigned);

#define PCIIDE_INTERFACE_PCI(chan)	(0x01 << (2 * (chan)))

/* native: PCI BAR0-3 */
static const struct {
	int cmd, ctl;
} pcibar[2] = {
	{ 0x10, 0x14 },
	{ 0x18, 0x1c },
};
/* legacy: ISA/PCI IO space */
static const struct {
	int cmd, ctl;
} compat[2] = {
	{ 0x1f0, 0x3f6 },
	{ 0x170, 0x376 },
};

void *
pciide_init(unsigned tag, unsigned data)
{
	int ch, i;
	unsigned val;
	struct atac_softc *l;
	struct atac_channel *cp;

	val = pcicfgread(tag, PCI_ID_REG);
	switch (val) {
	case PCI_DEVICE(0x1095, 0x0680): /* SiI 0680 IDE */
	case PCI_DEVICE(0x1283, 0x8211): /* ITE 8211 IDE */
	case PCI_DEVICE(0x1106, 0x1571): /* VIA 82C586 IDE */
	case PCI_DEVICE(0x10ad, 0x0105): /* Symphony Labs 82C105 IDE */
		break;
	default:
		return NULL;
	}
	l = alloc(sizeof(struct atac_softc));
	memset(l, 0, sizeof(struct atac_softc));
	val = pcicfgread(tag, PCI_CLASS_REG);
	for (ch = 0; ch < 2; ch += 1) {
		cp = &l->channel[ch];
		if (PCIIDE_INTERFACE_PCI(ch) & val) {
			cp->c_cmdbase =
			  (void *)DEVTOV(~03 & pcicfgread(tag, pcibar[ch].cmd));
			cp->c_ctlbase =
			  (void *)DEVTOV(~03 & pcicfgread(tag, pcibar[ch].ctl));
			cp->c_data = (u_int16_t *)(cp->c_cmdbase + wd_data);
			for (i = 0; i < 8; i++)
				cp->c_cmdreg[i] = cp->c_cmdbase + i;
			cp->c_cmdreg[wd_status] = cp->c_cmdreg[wd_command];
			cp->c_cmdreg[wd_features] = cp->c_cmdreg[wd_precomp];
		}
		else {
			uint32_t pciiobase = 0xfe000000; /* !!! */
			cp->c_cmdbase =
			    (void *)DEVTOV(pciiobase + compat[ch].cmd);
			cp->c_ctlbase =
			    (void *)DEVTOV(pciiobase + compat[ch].ctl);
			cp->c_data = (u_int16_t *)(cp->c_cmdbase + wd_data);
			for (i = 0; i < 8; i++)
				cp->c_cmdreg[i] = cp->c_cmdbase + i;
			cp->c_cmdreg[wd_status] = cp->c_cmdreg[wd_command];
			cp->c_cmdreg[wd_features] = cp->c_cmdreg[wd_precomp];
		}
		cp->compatchan = ch;
	}
	l->chvalid = 03 & data;
	l->tag = tag;
	return l;
}
