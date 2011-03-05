/*	$NetBSD: nand.h,v 1.2.2.2 2011/03/05 15:10:22 bouyer Exp $	*/

/*-
 * Copyright (c) 2010 Department of Software Engineering,
 *		      University of Szeged, Hungary
 * Copyright (c) 2010 Adam Hoka <ahoka@NetBSD.org>
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

#ifndef _NAND_H_
#define _NAND_H_

#include <sys/param.h>
#include <sys/cdefs.h>

#include <sys/bufq.h>
#include <sys/buf.h>
#include <sys/time.h>

#include <dev/flash/flash.h>

/* flash interface implementation */
int nand_flash_isbad(device_t, uint64_t);
int nand_flash_markbad(device_t, uint64_t);
int nand_flash_write(device_t, off_t, size_t, size_t *, const u_char *);
int nand_flash_read(device_t, off_t, size_t, size_t *, uint8_t *);
int nand_flash_erase(device_t, struct flash_erase_instruction *);

/* nand specific functions */
int nand_erase_block(device_t, size_t);

int nand_io_submit(device_t, struct buf *);
void nand_sync_thread(void *);
int nand_sync_thread_start(device_t);
void nand_sync_thread_stop(device_t);

bool nand_isfactorybad(device_t, flash_addr_t);
bool nand_iswornoutbad(device_t, flash_addr_t);
bool nand_isbad(device_t, flash_addr_t);
void nand_markbad(device_t, size_t);

int nand_read_page(device_t, size_t, uint8_t *);
int nand_read_oob(device_t self, size_t page, void *oob);

/*
 * default functions for driver development
 */
void nand_default_select(device_t, bool);
int nand_default_ecc_compute(device_t, const uint8_t *, uint8_t *);
int nand_default_ecc_correct(device_t, uint8_t *, const uint8_t *,
    const uint8_t *);

static inline void nand_busy(device_t);
static inline void nand_select(device_t, bool);
static inline void nand_command(device_t, uint8_t);
static inline void nand_address(device_t, uint32_t);
static inline void nand_read_buf_byte(device_t, void *, size_t);
static inline void nand_read_buf_word(device_t, void *, size_t);
static inline void nand_read_byte(device_t, uint8_t *);
static inline void nand_write_buf_byte(device_t, const void *, size_t);
static inline void nand_write_buf_word(device_t, const void *, size_t);
//static inline bool nand_block_isbad(device_t, off_t);
//static inline void nand_block_markbad(device_t, off_t);
//static inline bool nand_isbusy(device_t);

//#define NAND_DEBUG 1
#ifdef NAND_DEBUG
#define DPRINTF(x)	if (nanddebug) printf x
#define DPRINTFN(n,x)	if (nanddebug>(n)) printf x
#else
#define DPRINTF(x)
#define DPRINTFN(n,x)
#endif

#define NAND_VERBOSE

/* same as in linux for compatibility */
enum {
	NAND_BAD_MARKER_OFFSET		= 0,
	NAND_BAD_MARKER_OFFSET_SMALL	= 5
};

/* feature flags use in nc_flags */
enum {
	NC_BUSWIDTH_16		= (1<<0),
	NC_SOURCE_SYNC		= (1<<2),
	NC_INTERLEAVED_PE	= (1<<1),
	NC_INTERLEAVED_R	= (1<<3),
	NC_EXTENDED_PARAM	= (1<<4)
};

/* various quirks used in nc_quirks */
enum {
	NC_QUIRK_NO_READ_START = (1<<0)
};

enum {
	NAND_ECC_READ,
	NAND_ECC_WRITE
};

enum {
	NAND_ECC_OK,
	NAND_ECC_CORRECTED,
	NAND_ECC_INVALID,
	NAND_ECC_TWOBIT
};

enum {
	NAND_ECC_TYPE_HW,
	NAND_ECC_TYPE_SW
};

struct nand_bbt {
	uint8_t *nbbt_bitmap;
	size_t nbbt_size;
};

struct nand_ecc {
	size_t necc_offset;		/* offset of ecc data in oob */
	size_t necc_size;		/* size of ecc data in oob */
	size_t necc_block_size;		/* block size used in ecc calc */
	size_t necc_code_size;		/* reduntant bytes per block */
	int necc_steps;			/* pagesize / code size */
	int necc_type;			/* type of the ecc engine */
};

/**
 * nand_chip: structure containing the required information
 *	      about the NAND chip.
 */
struct nand_chip {
	uint8_t	*nc_oob_cache;		/* buffer for oob cache */
	uint8_t *nc_page_cache;		/* buffer for page cache */
	uint8_t *nc_ecc_cache;
	size_t nc_size;			/* storage size in bytes */
	size_t nc_page_size;		/* page size in bytes */
	size_t nc_block_pages;		/* block size in pages */
	size_t nc_block_size;		/* block size in bytes */
	size_t nc_spare_size;		/* spare (oob) size in bytes */
	uint32_t nc_flags;		/* bitfield flags */
	uint32_t nc_quirks;		/* bitfield quirks */
	unsigned int nc_page_shift;	/* page shift for page alignment */
	unsigned int nc_page_mask;	/* page mask for page alignment */
	unsigned int nc_block_shift;	/* write shift */
	unsigned int nc_block_mask;	/* write mask */
	uint8_t nc_manf_id;		/* manufacturer id */
	uint8_t nc_dev_id;		/* device id  */
	uint8_t nc_addr_cycles_row;	/* row cycles for addressing */
	uint8_t nc_addr_cycles_column;	/* column cycles for addressing */
	uint8_t nc_badmarker_offs;	/* offset for marking bad blocks */
	
	struct nand_ecc *nc_ecc;
};

struct nand_write_cache {
	struct bintime nwc_creation;
	struct bintime nwc_last_write;
	struct bufq_state *nwc_bufq;
	uint8_t *nwc_data;
	daddr_t nwc_block;
	kmutex_t nwc_lock;
	bool nwc_write_pending;
};

/* driver softc for nand */
struct nand_softc {
	device_t sc_dev;
	device_t nand_dev;
	struct nand_interface *nand_if;
	void *nand_softc;
	struct nand_chip sc_chip;
	struct nand_bbt sc_bbt;
	size_t sc_part_offset;
	size_t sc_part_size;
	kmutex_t sc_device_lock; /* serialize access to chip */

	/* for the i/o thread */
	struct lwp *sc_sync_thread;
	struct nand_write_cache sc_cache;
	kmutex_t sc_io_lock;
	kmutex_t sc_waitq_lock;
	kcondvar_t sc_io_cv;
	bool sc_io_running;
};

/* structure holding the nand api */
struct nand_interface
{
	void (*select) (device_t, bool);
	void (*command) (device_t, uint8_t);
	void (*address) (device_t, uint8_t);
	void (*read_buf_byte) (device_t, void *, size_t);
	void (*read_buf_word) (device_t, void *, size_t);
	void (*read_byte) (device_t, uint8_t *);
	void (*read_word) (device_t, uint16_t *);
	void (*write_buf_byte) (device_t, const void *, size_t);
	void (*write_buf_word) (device_t, const void *, size_t);
	void (*write_byte) (device_t, uint8_t);
	void (*write_word) (device_t, uint16_t);
	void (*busy) (device_t);

	/* functions specific to ecc computation */
	int (*ecc_prepare)(device_t, int);
	int (*ecc_compute)(device_t, const uint8_t *, uint8_t *);
	int (*ecc_correct)(device_t, uint8_t *, const uint8_t *,
	    const uint8_t *);

	struct nand_ecc ecc;

	/* flash partition information */
	const struct flash_partition *part_info;
	int part_num;
};

/* attach args */
struct nand_attach_args {
	struct nand_interface *naa_nand_if;
};

device_t nand_attach_mi(struct nand_interface *nand_if, device_t dev);

static inline void
nand_busy(device_t device)
{
	struct nand_softc *sc = device_private(device);
	
	KASSERT(sc->nand_if->select != NULL);
	KASSERT(sc->nand_dev != NULL);
	
	sc->nand_if->select(sc->nand_dev, true);
	
	if (sc->nand_if->busy != NULL) {
		sc->nand_if->busy(sc->nand_dev);
	}

	sc->nand_if->select(sc->nand_dev, false);
}

static inline void
nand_select(device_t self, bool enable)
{
	struct nand_softc *sc = device_private(self);
	
	KASSERT(sc->nand_if->select != NULL);
	KASSERT(sc->nand_dev != NULL);
	
	sc->nand_if->select(sc->nand_dev, enable);
}

static inline void
nand_address(device_t self, uint32_t address)
{
	struct nand_softc *sc = device_private(self);
	
	KASSERT(sc->nand_if->address != NULL);
	KASSERT(sc->nand_dev != NULL);
	
	sc->nand_if->address(sc->nand_dev, address);
}

static inline void
nand_command(device_t self, uint8_t command)
{
	struct nand_softc *sc = device_private(self);
	
	KASSERT(sc->nand_if->command != NULL);
	KASSERT(sc->nand_dev != NULL);

	sc->nand_if->command(sc->nand_dev, command);
}

static inline void
nand_read_byte(device_t self, uint8_t *data)
{
	struct nand_softc *sc = device_private(self);
	
	KASSERT(sc->nand_if->read_byte != NULL);
	KASSERT(sc->nand_dev != NULL);
	
	sc->nand_if->read_byte(sc->nand_dev, data);
}

static inline void
nand_write_byte(device_t self, uint8_t data)
{
	struct nand_softc *sc = device_private(self);
	
	KASSERT(sc->nand_if->write_byte != NULL);
	KASSERT(sc->nand_dev != NULL);
	
	sc->nand_if->write_byte(sc->nand_dev, data);
}

static inline void
nand_read_word(device_t self, uint16_t *data)
{
	struct nand_softc *sc = device_private(self);
	
	KASSERT(sc->nand_if->read_word != NULL);
	KASSERT(sc->nand_dev != NULL);
	
	sc->nand_if->read_word(sc->nand_dev, data);
}

static inline void
nand_write_word(device_t self, uint16_t data)
{
	struct nand_softc *sc = device_private(self);
	
	KASSERT(sc->nand_if->write_word != NULL);
	KASSERT(sc->nand_dev != NULL);
	
	sc->nand_if->write_word(sc->nand_dev, data);
}

static inline void
nand_read_buf_byte(device_t self, void *buf, size_t size)
{
	struct nand_softc *sc = device_private(self);

	KASSERT(sc->nand_if->read_buf_byte != NULL);
	KASSERT(sc->nand_dev != NULL);
	
	sc->nand_if->read_buf_byte(sc->nand_dev, buf, size);	
}

static inline void
nand_read_buf_word(device_t self, void *buf, size_t size)
{
	struct nand_softc *sc = device_private(self);

	KASSERT(sc->nand_if->read_buf_word != NULL);
	KASSERT(sc->nand_dev != NULL);
	
	sc->nand_if->read_buf_word(sc->nand_dev, buf, size);	
}

static inline void
nand_write_buf_byte(device_t self, const void *buf, size_t size)
{
	struct nand_softc *sc = device_private(self);
	
	KASSERT(sc->nand_if->write_buf_byte != NULL);
	KASSERT(sc->nand_dev != NULL);
	
	sc->nand_if->write_buf_byte(sc->nand_dev, buf, size);	
}

static inline void
nand_write_buf_word(device_t self, const void *buf, size_t size)
{
	struct nand_softc *sc = device_private(self);

	KASSERT(sc->nand_if->write_buf_word != NULL);
	KASSERT(sc->nand_dev != NULL);
	
	sc->nand_if->write_buf_word(sc->nand_dev, buf, size);	
}

static inline int
nand_ecc_correct(device_t self, uint8_t *data, const uint8_t *oldcode,
    const uint8_t *newcode)
{
	struct nand_softc *sc = device_private(self);

	KASSERT(sc->nand_if->ecc_correct != NULL);
	KASSERT(sc->nand_dev != NULL);

	return sc->nand_if->ecc_correct(sc->nand_dev, data, oldcode, newcode);
}

static inline void
nand_ecc_compute(device_t self, const uint8_t *data, uint8_t *code)
{
	struct nand_softc *sc = device_private(self);

	KASSERT(sc->nand_if->ecc_compute != NULL);
	KASSERT(sc->nand_dev != NULL);

	sc->nand_if->ecc_compute(sc->nand_dev, data, code);	
}

static inline void
nand_ecc_prepare(device_t self, int mode)
{
	struct nand_softc *sc = device_private(self);

	KASSERT(sc->nand_dev != NULL);
	
	if (sc->nand_if->ecc_prepare != NULL)
		sc->nand_if->ecc_prepare(sc->nand_dev, mode);
}

#if 0
static inline bool
nand_block_isbad(device_t self, off_t block)
{
	struct nand_softc *sc = device_private(self);
	
	KASSERT(sc->nand_if->block_isbad != NULL);
	KASSERT(sc->nand_dev != NULL);
	
	return sc->nand_if->block_isbad(sc->nand_dev, block);
}
#endif

/* Manufacturer IDs defined by JEDEC */
enum {
	NAND_MFR_UNKNOWN	= 0x00,
	NAND_MFR_AMD		= 0x01,
	NAND_MFR_FUJITSU	= 0x04,
	NAND_MFR_RENESAS	= 0x07,
	NAND_MFR_STMICRO	= 0x20,
	NAND_MFR_MICRON		= 0x2c,
	NAND_MFR_NATIONAL	= 0x8f,
	NAND_MFR_TOSHIBA	= 0x98,
	NAND_MFR_HYNIX		= 0xad,
	NAND_MFR_SAMSUNG	= 0xec
};

struct nand_manufacturer {
	int id;
	const char *name;
};

extern const struct nand_manufacturer nand_mfrs[];

static inline void
nand_dump_data(const char *name, void *data, size_t len)
{
	uint8_t *dump = data;
	int i;

	printf("dumping %s\n--------------\n", name);
	for (i = 0; i < len; i++) {
		printf("0x%.2hhx ", *dump);
		dump++;
	}
	printf("\n--------------\n");
}

#endif	/* _NAND_H_ */
