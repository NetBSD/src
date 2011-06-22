/*	$NetBSD: nor.h,v 1.1 2011/06/22 21:59:15 ahoka Exp $	*/

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

#ifndef _NOR_H_
#define _NOR_H_

#include <sys/param.h>
#include <sys/cdefs.h>

#include <sys/bufq.h>
#include <sys/buf.h>

#include <dev/nor/cfi.h>
#include <dev/flash/flash.h>

#ifdef NOR_DEBUG
#define DPRINTF(x)	printf x
#else
#define DPRINTF(x)
#endif

/**
 * nor_chip: structure containing the required information
 *	      about the NOR chip.
 */
struct nor_chip {
	struct nor_ecc *nc_ecc; 	/* ecc information */
	uint8_t	*nc_oob_cache;		/* buffer for oob cache */
	uint8_t *nc_page_cache;		/* buffer for page cache */
	uint8_t *nc_ecc_cache;
	size_t nc_size;			/* storage size in bytes */
	size_t nc_page_size;		/* page size in bytes */
	size_t nc_block_pages;		/* block size in pages */
	size_t nc_block_size;		/* block size in bytes */
	size_t nc_spare_size;		/* spare (oob) size in bytes */
	uint32_t nc_lun_blocks;		/* LUN size in blocks */
	uint32_t nc_flags;		/* bitfield flags */
	uint32_t nc_quirks;		/* bitfield quirks */
	unsigned int nc_page_shift;	/* page shift for page alignment */
	unsigned int nc_page_mask;	/* page mask for page alignment */
	unsigned int nc_block_shift;	/* write shift */
	unsigned int nc_block_mask;	/* write mask */
	uint8_t nc_num_luns;		/* number of LUNs */
	uint8_t nc_manf_id;		/* manufacturer id */
	uint8_t nc_dev_id;		/* device id  */
	uint8_t nc_badmarker_offs;	/* offset for marking bad blocks */
};

/* driver softc for nor */
struct nor_softc {
	device_t sc_dev;
	device_t controller_dev;
	struct nor_interface *nor_if;
	void *nor_softc;
	struct nor_chip sc_chip;
	size_t sc_part_offset;
	size_t sc_part_size;
	kmutex_t sc_device_lock; /* serialize access to chip */
};

/* structure holding the nor api */
struct nor_interface
{
	/* basic nor controller commands */
	void (*select) (device_t, bool); /* optional */
	void (*read_1) (device_t, uint8_t *);
	void (*read_2) (device_t, uint16_t *);
	void (*read_4) (device_t, uint32_t *);
	void (*read_buf_1) (device_t, void *, size_t);
	void (*read_buf_2) (device_t, void *, size_t);
	void (*read_buf_4) (device_t, void *, size_t);
	void (*write_1) (device_t, uint8_t);
	void (*write_2) (device_t, uint16_t);
	void (*write_4) (device_t, uint32_t);
	void (*write_buf_1) (device_t, const void *, size_t);
	void (*write_buf_2) (device_t, const void *, size_t);
	void (*write_buf_4) (device_t, const void *, size_t);
	void (*busy) (device_t);

	int access_width;	/* x8/x16/x32: 1/2/4 */

	/* flash partition information */
	const struct flash_partition *part_info;
	int part_num;
};

/* attach args */
struct nor_attach_args {
	struct nor_interface *naa_nor_if;
};

static inline void
nor_busy(device_t device)
{
	struct nor_softc *sc = device_private(device);

	KASSERT(sc->nor_if->select != NULL);
	KASSERT(sc->controller_dev != NULL);

	sc->nor_if->select(sc->controller_dev, true);

	if (sc->nor_if->busy != NULL) {
		sc->nor_if->busy(sc->controller_dev);
	}

	sc->nor_if->select(sc->controller_dev, false);
}

static inline void
nor_select(device_t self, bool enable)
{
	struct nor_softc *sc = device_private(self);

	KASSERT(sc->nor_if->select != NULL);
	KASSERT(sc->controller_dev != NULL);

	sc->nor_if->select(sc->controller_dev, enable);
}

static inline void
nor_read_1(device_t self, uint8_t *data)
{
	struct nor_softc *sc = device_private(self);

	KASSERT(sc->nor_if->read_1 != NULL);
	KASSERT(sc->controller_dev != NULL);

	sc->nor_if->read_1(sc->controller_dev, data);
}

static inline void
nor_read_2(device_t self, uint16_t *data)
{
	struct nor_softc *sc = device_private(self);

	KASSERT(sc->nor_if->read_2 != NULL);
	KASSERT(sc->controller_dev != NULL);

	sc->nor_if->read_2(sc->controller_dev, data);
}

static inline void
nor_read_4(device_t self, uint16_t *data)
{
	struct nor_softc *sc = device_private(self);

	KASSERT(sc->nor_if->read_4 != NULL);
	KASSERT(sc->controller_dev != NULL);

	sc->nor_if->read_4(sc->controller_dev, data);
}

static inline void
nor_write_1(device_t self, uint8_t data)
{
	struct nor_softc *sc = device_private(self);

	KASSERT(sc->nor_if->write_1 != NULL);
	KASSERT(sc->controller_dev != NULL);

	sc->nor_if->write_1(sc->controller_dev, data);
}

static inline void
nor_write_2(device_t self, uint16_t data)
{
	struct nor_softc *sc = device_private(self);

	KASSERT(sc->nor_if->write_2 != NULL);
	KASSERT(sc->controller_dev != NULL);

	sc->nor_if->write_2(sc->controller_dev, data);
}

static inline void
nor_write_4(device_t self, uint16_t data)
{
	struct nor_softc *sc = device_private(self);

	KASSERT(sc->nor_if->write_4 != NULL);
	KASSERT(sc->controller_dev != NULL);

	sc->nor_if->write_4(sc->controller_dev, data);
}

static inline void
nor_read_buf_1(device_t self, void *buf, size_t size)
{
	struct nor_softc *sc = device_private(self);

	KASSERT(sc->nor_if->read_buf_1 != NULL);
	KASSERT(sc->controller_dev != NULL);

	sc->nor_if->read_buf_1(sc->controller_dev, buf, size);
}

static inline void
nor_read_buf_2(device_t self, void *buf, size_t size)
{
	struct nor_softc *sc = device_private(self);

	KASSERT(sc->nor_if->read_buf_2 != NULL);
	KASSERT(sc->controller_dev != NULL);

	sc->nor_if->read_buf_2(sc->controller_dev, buf, size);
}

static inline void
nor_read_buf_4(device_t self, void *buf, size_t size)
{
	struct nor_softc *sc = device_private(self);

	KASSERT(sc->nor_if->read_buf_4 != NULL);
	KASSERT(sc->controller_dev != NULL);

	sc->nor_if->read_buf_4(sc->controller_dev, buf, size);
}

static inline void
nor_write_buf_1(device_t self, const void *buf, size_t size)
{
	struct nor_softc *sc = device_private(self);

	KASSERT(sc->nor_if->write_buf_1 != NULL);
	KASSERT(sc->controller_dev != NULL);

	sc->nor_if->write_buf_1(sc->controller_dev, buf, size);
}

static inline void
nor_write_buf_2(device_t self, const void *buf, size_t size)
{
	struct nor_softc *sc = device_private(self);

	KASSERT(sc->nor_if->write_buf_2 != NULL);
	KASSERT(sc->controller_dev != NULL);

	sc->nor_if->write_buf_2(sc->controller_dev, buf, size);
}

static inline void
nor_write_buf_4(device_t self, const void *buf, size_t size)
{
	struct nor_softc *sc = device_private(self);

	KASSERT(sc->nor_if->write_buf_4 != NULL);
	KASSERT(sc->controller_dev != NULL);

	sc->nor_if->write_buf_4(sc->controller_dev, buf, size);
}

/* Manufacturer IDs defined by JEDEC */
enum {
	NOR_MFR_UNKNOWN	= 0x00,
	NOR_MFR_AMD	= 0x01,
	NOR_MFR_FUJITSU	= 0x04,
	NOR_MFR_RENESAS	= 0x07,
	NOR_MFR_STMICRO	= 0x20,
	NOR_MFR_MICRON	= 0x2c,
	NOR_MFR_NATIONAL= 0x8f,
	NOR_MFR_TOSHIBA	= 0x98,
	NOR_MFR_HYNIX	= 0xad,
	NOR_MFR_SAMSUNG	= 0xec
};

struct nor_manufacturer {
	int id;
	const char *name;
};

extern const struct nor_manufacturer nor_mfrs[];


/* flash interface implementation */
int nor_flash_isbad(device_t, flash_off_t, bool *);
int nor_flash_markbad(device_t, flash_off_t);
int nor_flash_write(device_t, flash_off_t, size_t, size_t *, const u_char *);
int nor_flash_read(device_t, flash_off_t, size_t, size_t *, uint8_t *);
int nor_flash_erase(device_t, struct flash_erase_instruction *);

/* nor specific functions */
int nor_erase_block(device_t, size_t);

int nor_io_submit(device_t, struct buf *);
void nor_sync_thread(void *);
int nor_sync_thread_start(device_t);
void nor_sync_thread_stop(device_t);

device_t nor_attach_mi(struct nor_interface *, device_t);
void nor_init_interface(struct nor_interface *);

/*
 * default functions for driver development
 */
void nor_default_select(device_t, bool);

#if 0
static inline void nor_busy(device_t);
static inline void nor_select(device_t, bool);
static inline void nor_command(device_t, uint8_t);
static inline void nor_address(device_t, uint32_t);
static inline void nor_read_buf_1(device_t, void *, size_t);
static inline void nor_read_buf_2(device_t, void *, size_t);
static inline void nor_read_1(device_t, uint8_t *);
static inline void nor_read_2(device_t, uint8_t *);
static inline void nor_write_buf_1(device_t, const void *, size_t);
static inline void nor_write_buf_2(device_t, const void *, size_t);
#endif

#endif	/* _NOR_H_ */
