/* $NetBSD: pciide.c,v 1.2 2008/04/07 12:33:57 nisimura Exp $ */

#include <sys/param.h>
#include <sys/disklabel.h>

#include <dev/ic/wdcreg.h>
#include <dev/ata/atareg.h>

#include <lib/libsa/stand.h>
#include "globals.h"

/*
 * cmdide
 * iteide
 * viaide
 * slide
 */

void *pciide_init(unsigned, unsigned);

#define PCIIDE_INTERFACE_PCI(chan)	(0x01 << (2 * (chan)))
#define PCIIDE_REG_CMD_BASE(chan)	(0x10 + (8 * (chan)))
#define PCIIDE_REG_CTL_BASE(chan)	(0x14 + (8 * (chan)))
#define PCIIDE_COMPAT_CMD_BASE(ch)	(((ch)==0) ? 0x1f0 : 0x170)
#define PCIIDE_COMPAT_CTL_BASE(ch)	(((ch)==0) ? 0x3f6 : 0x376)

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
			cp->c_cmdbase = (void *)pcicfgread(tag,
			    PCIIDE_REG_CMD_BASE(ch));
			cp->c_ctlbase = (void *)pcicfgread(tag,
			    PCIIDE_REG_CTL_BASE(ch));
			cp->c_data = (u_int16_t *)(cp->c_cmdbase + wd_data);
			for (i = 0; i < 8; i++)
				cp->c_cmdreg[i] = cp->c_cmdbase + i;
			cp->c_cmdreg[wd_status] = cp->c_cmdreg[wd_command];
			cp->c_cmdreg[wd_features] = cp->c_cmdreg[wd_precomp];
		}
		else {
			tag = 0; /* !!! */
			cp->c_cmdbase = (void *)(pcicfgread(tag, 0x10) +
			    PCIIDE_COMPAT_CMD_BASE(ch));
			cp->c_ctlbase = (void *)(pcicfgread(tag, 0x10) +
			    PCIIDE_COMPAT_CTL_BASE(ch));
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
