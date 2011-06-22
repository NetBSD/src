/*	$NetBSD: nor.c,v 1.1 2011/06/22 21:59:15 ahoka Exp $	*/

/*-
 * Copyright (c) 2011 Department of Software Engineering,
 *		      University of Szeged, Hungary
 * Copyright (c) 2011 Adam Hoka <ahoka@NetBSD.org>
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by the Department of Software Engineering, University of Szeged, Hungary
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/* Common driver for NOR chips implementing the ONFI CFI specification */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: nor.c,v 1.1 2011/06/22 21:59:15 ahoka Exp $");

#include "locators.h"

#include <sys/param.h>
#include <sys/types.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/sysctl.h>
#include <sys/atomic.h>

#include <dev/flash/flash.h>
#include <dev/nor/nor.h>
#include <dev/nor/cfi.h>

//#include "opt_nor.h"

int nor_match(device_t, cfdata_t, void *);
void nor_attach(device_t, device_t, void *);
int nor_detach(device_t, int);
bool nor_shutdown(device_t, int);

int nor_print(void *, const char *);
static int nor_search(device_t, cfdata_t, const int *, void *);

CFATTACH_DECL_NEW(nor, sizeof(struct nor_softc),
    nor_match, nor_attach, nor_detach, NULL);

#ifdef NOR_DEBUG
int	nordebug = NOR_DEBUG;
#endif

int nor_cachesync_timeout = 1;
int nor_cachesync_nodenum;

#ifdef NOR_VERBOSE
const struct nor_manufacturer nor_mfrs[] = {
	{ NOR_MFR_AMD,		"AMD" },
	{ NOR_MFR_FUJITSU,	"Fujitsu" },
	{ NOR_MFR_RENESAS,	"Renesas" },
	{ NOR_MFR_STMICRO,	"ST Micro" },
	{ NOR_MFR_MICRON,	"Micron" },
	{ NOR_MFR_NATIONAL,	"National" },
	{ NOR_MFR_TOSHIBA,	"Toshiba" },
	{ NOR_MFR_HYNIX,	"Hynix" },
	{ NOR_MFR_SAMSUNG,	"Samsung" },
	{ NOR_MFR_UNKNOWN,	"Unknown" }
};

static const char *
nor_midtoname(int id)
{
	int i;

	for (i = 0; nor_mfrs[i].id != 0; i++) {
		if (nor_mfrs[i].id == id)
			return nor_mfrs[i].name;
	}

	KASSERT(nor_mfrs[i].id == 0);

	return nor_mfrs[i].name;
}
#endif

/* ARGSUSED */
int
nor_match(device_t parent, cfdata_t match, void *aux)
{
	/* pseudo device, always attaches */
	return 1;
}

void
nor_attach(device_t parent, device_t self, void *aux)
{
	struct nor_softc *sc = device_private(self);
	struct nor_attach_args *naa = aux;
	struct nor_chip *chip = &sc->sc_chip;

	sc->sc_dev = self;
	sc->controller_dev = parent;
	sc->nor_if = naa->naa_nor_if;

	aprint_naive("\n");

	/* allocate cache */
	chip->nc_oob_cache = kmem_alloc(chip->nc_spare_size, KM_SLEEP);
	chip->nc_page_cache = kmem_alloc(chip->nc_page_size, KM_SLEEP);

	mutex_init(&sc->sc_device_lock, MUTEX_DEFAULT, IPL_NONE);

	if (nor_sync_thread_start(self)) {
		goto error;
	}

	if (!pmf_device_register1(sc->sc_dev, NULL, NULL, nor_shutdown))
		aprint_error_dev(sc->sc_dev,
		    "couldn't establish power handler\n");

#ifdef NOR_BBT
	nor_bbt_init(self);
	nor_bbt_scan(self);
#endif

	/*
	 * Attach all our devices
	 */
	config_search_ia(nor_search, self, NULL, NULL);

	return;
error:
	kmem_free(chip->nc_oob_cache, chip->nc_spare_size);
	kmem_free(chip->nc_page_cache, chip->nc_page_size);
	mutex_destroy(&sc->sc_device_lock);
}

static int
nor_search(device_t parent, cfdata_t cf, const int *ldesc, void *aux)
{
	struct nor_softc *sc = device_private(parent);
	struct nor_chip *chip = &sc->sc_chip;
	struct flash_interface *flash_if;
	struct flash_attach_args faa;

	flash_if = kmem_alloc(sizeof(*flash_if), KM_SLEEP);

	flash_if->type = FLASH_TYPE_NOR;

	flash_if->read = nor_flash_read;
	flash_if->write = nor_flash_write;
	flash_if->erase = nor_flash_erase;
	flash_if->block_isbad = nor_flash_isbad;
	flash_if->block_markbad = nor_flash_markbad;

	flash_if->submit = nor_io_submit;

	flash_if->erasesize = chip->nc_block_size;
	flash_if->page_size = chip->nc_page_size;
	flash_if->writesize = chip->nc_page_size;

	flash_if->partition.part_offset = cf->cf_loc[FLASHBUSCF_OFFSET];

	if (cf->cf_loc[FLASHBUSCF_SIZE] == 0) {
		flash_if->size = chip->nc_size -
		    flash_if->partition.part_offset;
		flash_if->partition.part_size = flash_if->size;
	} else {
		flash_if->size = cf->cf_loc[FLASHBUSCF_SIZE];
		flash_if->partition.part_size = cf->cf_loc[FLASHBUSCF_SIZE];
	}

	if (cf->cf_loc[FLASHBUSCF_READONLY])
		flash_if->partition.part_flags = FLASH_PART_READONLY;
	else
		flash_if->partition.part_flags = 0;

	faa.flash_if = flash_if;

	if (config_match(parent, cf, &faa)) {
		if (config_attach(parent, cf, &faa, nor_print) != NULL) {
			return 0;
		} else {
			return 1;
		}
	} else {
		kmem_free(flash_if, sizeof(*flash_if));
	}

	return 1;
}

int
nor_detach(device_t self, int flags)
{
	struct nor_softc *sc = device_private(self);
	struct nor_chip *chip = &sc->sc_chip;
	int error = 0;

	error = config_detach_children(self, flags);
	if (error) {
		return error;
	}

	nor_sync_thread_stop(self);
#ifdef NOR_BBT
	nor_bbt_detach(self);
#endif
	/* free oob cache */
	kmem_free(chip->nc_oob_cache, chip->nc_spare_size);
	kmem_free(chip->nc_page_cache, chip->nc_page_size);

	mutex_destroy(&sc->sc_device_lock);

	pmf_device_deregister(sc->sc_dev);

	return error;
}

int
nor_print(void *aux, const char *pnp)
{
	if (pnp != NULL)
		aprint_normal("nor at %s\n", pnp);

	return UNCONF;
}

/* ask for a nor driver to attach to the controller */
device_t
nor_attach_mi(struct nor_interface *nor_if, device_t parent)
{
	struct nor_attach_args arg;

	KASSERT(nor_if != NULL);

	arg.naa_nor_if = nor_if;
	return config_found_ia(parent, "norbus", &arg, nor_print);
}

/* default everything to reasonable values, to ease future api changes */
void
nor_init_interface(struct nor_interface *interface)
{
	interface->select = &nor_default_select;
	interface->read_1 = NULL;
	interface->read_2 = NULL;
	interface->read_4 = NULL;
	interface->read_buf_1 = NULL;
	interface->read_buf_2 = NULL;
	interface->read_buf_4 = NULL;
	interface->write_1 = NULL;
	interface->write_2 = NULL;
	interface->write_4 = NULL;
	interface->write_buf_1 = NULL;
	interface->write_buf_2 = NULL;
	interface->write_buf_4 = NULL;
	interface->busy = NULL;
}

#if 0
/* handle quirks here */
static void
nor_quirks(device_t self, struct nor_chip *chip)
{
	/* this is an example only! */
	switch (chip->nc_manf_id) {
	case NOR_MFR_SAMSUNG:
		if (chip->nc_dev_id == 0x00) {
			/* do something only samsung chips need */
			/* or */
			/* chip->nc_quirks |= NC_QUIRK_NO_READ_START */
		}
	}

	return;
}
#endif

#if 0
/**
 * scan media to determine the chip's properties
 * this function resets the device
 */
static int
nor_scan_media(device_t self, struct nor_chip *chip)
{
	struct nor_softc *sc = device_private(self);
	uint8_t onfi_signature[4];


	/* TODO: enter query mode, check for signature */

	/* TODO: get parameters in query mode */

#ifdef NOR_VERBOSE
	aprint_normal_dev(self,
	    "manufacturer id: 0x%.2x (%s), device id: 0x%.2x\n",
	    chip->nc_manf_id,
	    nor_midtoname(chip->nc_manf_id),
	    chip->nc_dev_id);
#endif

	aprint_normal_dev(self,
	    "page size: %zu bytes, spare size: %zu bytes, "
	    "block size: %zu bytes\n",
	    chip->nc_page_size, chip->nc_spare_size, chip->nc_block_size);

	aprint_normal_dev(self,
	    "LUN size: %" PRIu32 " blocks, LUNs: %" PRIu8
	    ", total storage size: %zu MB\n",
	    chip->nc_lun_blocks, chip->nc_num_luns,
	    chip->nc_size / 1024 / 1024);

#ifdef NOR_VERBOSE
	aprint_normal_dev(self, "column cycles: %" PRIu8 ", row cycles: %"
	    PRIu8 "\n",
	    chip->nc_addr_cycles_column, chip->nc_addr_cycles_row);
#endif

	/* XXX does this apply to nor? */
	/*
	 * calculate badblock marker offset in oob
	 * we try to be compatible with linux here
	 */
	if (chip->nc_page_size > 512)
		chip->nc_badmarker_offs = 0;
	else
		chip->nc_badmarker_offs = 5;

	/* Calculate page shift and mask */
	chip->nc_page_shift = ffs(chip->nc_page_size) - 1;
	chip->nc_page_mask = ~(chip->nc_page_size - 1);
	/* same for block */
	chip->nc_block_shift = ffs(chip->nc_block_size) - 1;
	chip->nc_block_mask = ~(chip->nc_block_size - 1);

	/* look for quirks here if needed in future */
	/* nor_quirks(self, chip); */

	return 0;
}
#endif

/* ARGSUSED */
bool
nor_shutdown(device_t self, int howto)
{
	return true;
}

/* implementation of the block device API */

int
nor_flash_write(device_t self, flash_off_t offset, size_t len, size_t *retlen,
    const uint8_t *buf)
{
	/* TODO: implement this based on nand.c */
	return 0;
}

int
nor_flash_read(device_t self, flash_off_t offset, size_t len, size_t *retlen,
    uint8_t *buf)
{
	/* TODO: implement this based on nand.c */
	return 0;
}

int
nor_flash_isbad(device_t self, flash_off_t ofs, bool *isbad)
{
	struct nor_softc *sc = device_private(self);
	struct nor_chip *chip = &sc->sc_chip;
//	bool result;

	if (ofs > chip->nc_size) {
		DPRINTF(("nor_flash_isbad: offset 0x%jx is larger than"
			" device size (0x%jx)\n", (uintmax_t)ofs,
			(uintmax_t)chip->nc_size));
		return EINVAL;
	}

	if (ofs % chip->nc_block_size != 0) {
		DPRINTF(("offset (0x%jx) is not the multiple of block size "
			"(%ju)",
			(uintmax_t)ofs, (uintmax_t)chip->nc_block_size));
		return EINVAL;
	}

	mutex_enter(&sc->sc_device_lock);
//	result = nor_isbad(self, ofs);
	mutex_exit(&sc->sc_device_lock);

//	*isbad = result;
	*isbad = false;

	return 0;
}

int
nor_flash_markbad(device_t self, flash_off_t ofs)
{
	struct nor_softc *sc = device_private(self);
	struct nor_chip *chip = &sc->sc_chip;

	if (ofs > chip->nc_size) {
		DPRINTF(("nor_flash_markbad: offset 0x%jx is larger than"
			" device size (0x%jx)\n", ofs,
			(uintmax_t)chip->nc_size));
		return EINVAL;
	}

	if (ofs % chip->nc_block_size != 0) {
		panic("offset (%ju) is not the multiple of block size (%ju)",
		    (uintmax_t)ofs, (uintmax_t)chip->nc_block_size);
	}

	/* TODO: implement this */

	return 0;
}

int
nor_flash_erase(device_t self,
    struct flash_erase_instruction *ei)
{
	struct nor_softc *sc = device_private(self);
	struct nor_chip *chip = &sc->sc_chip;
//	flash_off_t addr;
//	int error = 0;

	if (ei->ei_addr < 0 || ei->ei_len < chip->nc_block_size)
		return EINVAL;

	if (ei->ei_addr + ei->ei_len > chip->nc_size) {
		DPRINTF(("nor_flash_erase: erase address is over the end"
			" of the device\n"));
		return EINVAL;
	}

	if (ei->ei_addr % chip->nc_block_size != 0) {
		aprint_error_dev(self,
		    "nor_flash_erase: ei_addr (%ju) is not"
		    "the multiple of block size (%ju)",
		    (uintmax_t)ei->ei_addr,
		    (uintmax_t)chip->nc_block_size);
		return EINVAL;
	}

	if (ei->ei_len % chip->nc_block_size != 0) {
		aprint_error_dev(self,
		    "nor_flash_erase: ei_len (%ju) is not"
		    "the multiple of block size (%ju)",
		    (uintmax_t)ei->ei_addr,
		    (uintmax_t)chip->nc_block_size);
		return EINVAL;
	}

	/* TODO: do erase here */

	ei->ei_state = FLASH_ERASE_DONE;
	if (ei->ei_callback != NULL) {
		ei->ei_callback(ei);
	}

	return 0;
#if 0
out:
	mutex_exit(&sc->sc_device_lock);

	return error;
#endif
}

static int
sysctl_nor_verify(SYSCTLFN_ARGS)
{
	int error, t;
	struct sysctlnode node;

	node = *rnode;
	t = *(int *)rnode->sysctl_data;
	node.sysctl_data = &t;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	if (node.sysctl_num == nor_cachesync_nodenum) {
		if (t <= 0 || t > 60)
			return EINVAL;
	} else {
		return EINVAL;
	}

	*(int *)rnode->sysctl_data = t;

	return 0;
}

SYSCTL_SETUP(sysctl_nor, "sysctl nor subtree setup")
{
	int rc, nor_root_num;
	const struct sysctlnode *node;

	if ((rc = sysctl_createv(clog, 0, NULL, NULL,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE, "hw", NULL,
	    NULL, 0, NULL, 0, CTL_HW, CTL_EOL)) != 0) {
		goto error;
	}

	if ((rc = sysctl_createv(clog, 0, NULL, &node,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE, "nor",
	    SYSCTL_DESCR("NOR driver controls"),
	    NULL, 0, NULL, 0, CTL_HW, CTL_CREATE, CTL_EOL)) != 0) {
		goto error;
	}

	nor_root_num = node->sysctl_num;

	if ((rc = sysctl_createv(clog, 0, NULL, &node,
	    CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
	    CTLTYPE_INT, "cache_sync_timeout",
	    SYSCTL_DESCR("NOR write cache sync timeout in seconds"),
	    sysctl_nor_verify, 0, &nor_cachesync_timeout,
	    0, CTL_HW, nor_root_num, CTL_CREATE,
	    CTL_EOL)) != 0) {
		goto error;
	}

	nor_cachesync_nodenum = node->sysctl_num;

	return;

error:
	aprint_error("%s: sysctl_createv failed (rc = %d)\n", __func__, rc);
}

MODULE(MODULE_CLASS_DRIVER, nor, "flash");

#ifdef _MODULE
#include "ioconf.c"
#endif

static int
nor_modcmd(modcmd_t cmd, void *opaque)
{
	switch (cmd) {
	case MODULE_CMD_INIT:
#ifdef _MODULE
		return config_init_component(cfdriver_ioconf_nor,
		    cfattach_ioconf_nor, cfdata_ioconf_nor);
#else
		return 0;
#endif
	case MODULE_CMD_FINI:
#ifdef _MODULE
		return config_fini_component(cfdriver_ioconf_nor,
		    cfattach_ioconf_nor, cfdata_ioconf_nor);
#else
		return 0;
#endif
	default:
		return ENOTTY;
	}
}
