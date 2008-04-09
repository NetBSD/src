/* $NetBSD: siisata.c,v 1.6 2008/04/09 16:26:29 nisimura Exp $ */

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

void *siisata_init(unsigned, unsigned);

static void map3112chan(unsigned, int, struct atac_channel *);
static void map3114chan(unsigned, int, struct atac_channel *);

void *
siisata_init(unsigned tag, unsigned data)
{
	unsigned val, chvalid;
	struct atac_softc *l;

	val = pcicfgread(tag, PCI_ID_REG);
	switch (val) {
	case PCI_DEVICE(0x1095, 0x3112): /* SiI 3112 SATALink */
	case PCI_DEVICE(0x1095, 0x3512): /*     3512 SATALink */
		chvalid = 0x3;
		break;
	case PCI_DEVICE(0x1095, 0x3114): /* SiI 3114 SATALink */
		chvalid = 0xf;
		break;
	default:
		return NULL;
	}

	l = alloc(sizeof(struct atac_softc));
	memset(l, 0, sizeof(struct atac_softc));

	if ((PCI_PRODUCT(val) & 0xf) == 0x2) {
		map3112chan(tag, 0, &l->channel[0]);
		map3112chan(tag, 1, &l->channel[1]);
	}
	else {
		map3114chan(tag, 0, &l->channel[0]);
		map3114chan(tag, 1, &l->channel[1]);
		map3114chan(tag, 2, &l->channel[2]);
		map3114chan(tag, 3, &l->channel[3]);
	}
	l->chvalid = chvalid & data;
	l->tag = tag;
	return l;
}

/* BAR0-3 */
static const struct {
	int cmd, ctl;
} regstd[2] = {
	{ 0x10, 0x14 },
	{ 0x18, 0x1c },
};

static void
map3112chan(unsigned tag, int ch, struct atac_channel *cp)
{
	int i;

	cp->c_cmdbase = (void *)DEVTOV(~07 & pcicfgread(tag, regstd[ch].cmd));
	cp->c_ctlbase = (void *)DEVTOV(~03 & pcicfgread(tag, regstd[ch].ctl));
	cp->c_data = (u_int16_t *)(cp->c_cmdbase + wd_data);
	for (i = 0; i < 8; i++)
		cp->c_cmdreg[i] = cp->c_cmdbase + i;
	cp->c_cmdreg[wd_status] = cp->c_cmdreg[wd_command];
	cp->c_cmdreg[wd_features] = cp->c_cmdreg[wd_precomp];
}

/* offset to BAR5 */
static const struct {
	int IDE_TF0, IDE_TF8;
} reg3114[4] = {
	{ 0x080, 0x091 },
	{ 0x0c0, 0x0d1 },
	{ 0x280, 0x291 },
	{ 0x2c0, 0x2d1 },
};

static void
map3114chan(unsigned tag, int ch, struct atac_channel *cp)
{
	int i;
	uint8_t *ba5;

	ba5 = (uint8_t *)DEVTOV(pcicfgread(tag, 0x24)); /* PCI_BAR5 */
	cp->c_cmdbase = ba5 + reg3114[ch].IDE_TF0;
	cp->c_ctlbase = ba5 + reg3114[ch].IDE_TF8;
	cp->c_data = (u_int16_t *)(cp->c_cmdbase + wd_data);
	for (i = 0; i < 8; i++)
		cp->c_cmdreg[i] = cp->c_cmdbase + i;
	cp->c_cmdreg[wd_status] = cp->c_cmdreg[wd_command];
	cp->c_cmdreg[wd_features] = cp->c_cmdreg[wd_precomp];
}
