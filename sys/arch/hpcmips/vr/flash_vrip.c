/* $NetBSD: flash_vrip.c,v 1.7.40.2 2014/08/20 00:03:03 tls Exp $ */

/*
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Naoto Shimazaki of YOKOGAWA Electric Corporation.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Flash Memory Driver
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: flash_vrip.c,v 1.7.40.2 2014/08/20 00:03:03 tls Exp $");

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/systm.h>

#include <machine/bus.h>

#include <hpcmips/vr/vripif.h>
#include <hpcmips/vr/cfireg.h>
#include <hpcmips/vr/flashreg.h>
#include <hpcmips/vr/flashvar.h>

#ifdef FLASH_DEBUG
int	flash_debug = 0;
#define DPRINTF(x)	if (flash_debug) printf x
#else
#define DPRINTF(x)
#endif

static int flash_probe(device_t, cfdata_t, void *);
static void flash_attach(device_t, device_t, void *);

const static struct flashops * find_command_set(u_int8_t cmdset0,
						u_int8_t cmdset1);
static int i28f128_probe(bus_space_tag_t, bus_space_handle_t);
static int mbm29160_probe(bus_space_tag_t, bus_space_handle_t);
static int is_block_same(struct flash_softc *, bus_size_t, const void *);
static int probe_cfi(bus_space_tag_t iot, bus_space_handle_t ioh);

static int intel_erase(struct flash_softc *, bus_size_t);
static int intel_write(struct flash_softc *, bus_size_t);
static int amd_erase(struct flash_softc *, bus_size_t);
static int amd_write(struct flash_softc *, bus_size_t);

extern struct cfdriver flash_cd;

CFATTACH_DECL_NEW(flash_vrip, sizeof(struct flash_softc),
	      flash_probe, flash_attach, NULL, NULL);

dev_type_open(flashopen);
dev_type_close(flashclose);
dev_type_read(flashread);
dev_type_write(flashwrite);

const struct cdevsw flash_cdevsw = {
	.d_open = flashopen,
	.d_close = flashclose,
	.d_read = flashread,
	.d_write = flashwrite,
	.d_ioctl = noioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = nommap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = 0
};

static const struct flash_command_set {
	u_int8_t	fc_set0;
	u_int8_t	fc_set1;
	struct flashops	fc_ops;
} flash_cmd[] = {
	{
		.fc_set0	= CFI_COMMSET_INTEL0,
		.fc_set1	= CFI_COMMSET_INTEL1,
		.fc_ops		= {
			.fo_name	= "Intel",
			.fo_erase	= intel_erase,
			.fo_write	= intel_write,
		}
	},
	{
		.fc_set0	= CFI_COMMSET_AMDFJITU0,
		.fc_set1	= CFI_COMMSET_AMDFJITU1,
		.fc_ops		= {
			.fo_name	= "AMD/Fujitsu",
			.fo_erase	= amd_erase,
			.fo_write	= amd_write,
		}
	},
	{
		.fc_set0	= 0,
		.fc_set1	= 0,
		.fc_ops		= {
			.fo_name	= NULL,
			.fo_erase	= NULL,
			.fo_write	= NULL,
		}
	}
};


const static struct flashops *
find_command_set(u_int8_t cmdset0, u_int8_t cmdset1)
{
	const struct flash_command_set	*fc;

	for (fc = flash_cmd; fc->fc_ops.fo_name; fc++) {
		if (cmdset0 == fc->fc_set0 && cmdset1 == fc->fc_set1)
			return &fc->fc_ops;
	}
	return NULL;
}

static int
probe_cfi(bus_space_tag_t iot, bus_space_handle_t ioh)
{
	const u_int8_t	*idstr = CFI_QUERY_ID_STR;
	int		i;
	u_int8_t	cmdset0;
	u_int8_t	cmdset1;

	/* start Common Flash Interface Query */
	bus_space_write_2(iot, ioh, CFI_QUERY_OFFSET, CFI_READ_CFI_QUERY);

	/* read CFI Query ID string */
	i = CFI_QUERY_ID_STR_REG << 1;
	do {
		if (bus_space_read_2(iot, ioh, i) != *idstr) {
			bus_space_write_2(iot, ioh, 0, FLASH_RESET);
			return 1;
		}
		i += 2;
		idstr++;
	} while (*idstr);

	cmdset0 = bus_space_read_2(iot, ioh, CFI_PRIM_COMM_REG0 << 1);
	cmdset1 = bus_space_read_2(iot, ioh, CFI_PRIM_COMM_REG1 << 1);

	/* switch flash to read mode */
	bus_space_write_2(iot, ioh, 0, FLASH_RESET);

	if (!find_command_set(cmdset0, cmdset1))
		return 1;

	return 0;
}

static int
flash_probe(device_t parent, cfdata_t match, void *aux)
{
	struct vrip_attach_args	*va = aux;
	bus_space_handle_t	ioh;

	if (bus_space_map(va->va_iot, va->va_addr, va->va_size, 0, &ioh))
		return 0;
	if (!probe_cfi(va->va_iot, ioh)) {
		DPRINTF("CFI ID str and command set recognized\n");
		goto detect;
	}
	if (!i28f128_probe(va->va_iot, ioh)) {
		DPRINTF("28F128 detected\n");
		goto detect;
	}
	if (!mbm29160_probe(va->va_iot, ioh)) {
		DPRINTF("29LV160 detected\n");
		goto detect;
	}
	return 0;

detect:
	bus_space_unmap(va->va_iot, ioh, va->va_size);
	return 1;
}

static void
flash_attach(device_t parent, device_t self, void *aux)
{
	struct flash_softc	*sc = device_private(self);
	struct vrip_attach_args	*va = aux;
	int			i;
	int			fence;
	bus_space_tag_t		iot = va->va_iot;
	bus_space_handle_t	ioh;
	size_t			block_size;

	if (bus_space_map(iot, va->va_addr, va->va_size, 0, &ioh)) {
		printf(": can't map i/o space\n");
                return;
  	}
	
	sc->sc_iot = iot;
	sc->sc_ioh = ioh;
	sc->sc_size = va->va_size;
	sc->sc_status = 0;

	/*
	 * Read entire CFI structure
	 */
	bus_space_write_2(iot, ioh, CFI_QUERY_OFFSET, CFI_READ_CFI_QUERY);
	for (i = 0; i < CFI_TOTAL_SIZE; i++) {
		sc->sc_cfi_raw[i] = bus_space_read_2(iot, ioh, i << 1);
	}
	bus_space_write_2(iot, ioh, 0, FLASH_RESET);

	sc->sc_ops = find_command_set(sc->sc_cfi_raw[CFI_PRIM_COMM_REG0],
				      sc->sc_cfi_raw[CFI_PRIM_COMM_REG1]);
	if (sc->sc_ops) {
		printf(": using %s command set", sc->sc_ops->fo_name);
	} else {
		printf("opps sc->sc_ops is NULL\n");
	}

	/*
	 * determine size of the largest block
	 */
	sc->sc_block_size = 0;
	i = CFI_EBLK1_INFO_REG;
	fence = sc->sc_cfi_raw[CFI_NUM_ERASE_BLK_REG] * CFI_EBLK_INFO_SIZE
		+ i;
	for (; i < fence; i += CFI_EBLK_INFO_SIZE) {
		if (sc->sc_cfi_raw[i + CFI_EBLK_INFO_NSECT0] == 0
		    && sc->sc_cfi_raw[i + CFI_EBLK_INFO_NSECT1] == 0)
			continue;
		block_size
			= (sc->sc_cfi_raw[i + CFI_EBLK_INFO_SECSIZE0] << 8)
			+ (sc->sc_cfi_raw[i + CFI_EBLK_INFO_SECSIZE1] << 16);
		if (sc->sc_block_size < block_size)
			sc->sc_block_size = block_size;
	}
	
	if ((sc->sc_buf = malloc(sc->sc_block_size, M_DEVBUF, M_NOWAIT))
	    == NULL) {
		printf(": can't alloc buffer space\n");
		return;
	}

	sc->sc_write_buffer_size
		= 1 << (sc->sc_cfi_raw[CFI_MAX_WBUF_SIZE_REG0]
			+ (sc->sc_cfi_raw[CFI_MAX_WBUF_SIZE_REG1] << 8));
	sc->sc_typ_word_prog_timo
		= 1 << sc->sc_cfi_raw[CFI_TYP_WORD_PROG_REG];
	sc->sc_max_word_prog_timo
		= 1 << sc->sc_cfi_raw[CFI_MAX_WORD_PROG_REG];
	sc->sc_typ_buffer_write_timo
		= 1 << sc->sc_cfi_raw[CFI_TYP_BUF_WRITE_REG];
	sc->sc_max_buffer_write_timo
		= 1 << sc->sc_cfi_raw[CFI_MAX_BUF_WRITE_REG];
	sc->sc_typ_block_erase_timo
		= 1 << sc->sc_cfi_raw[CFI_TYP_BLOCK_ERASE_REG];
	sc->sc_max_block_erase_timo
		= 1 << sc->sc_cfi_raw[CFI_MAX_BLOCK_ERASE_REG];

	printf("\n");

#ifdef FLASH_DEBUG
	printf("read_cfi: extract cfi\n");
	printf("max block size: %dbyte\n", sc->sc_block_size);
	printf("write buffer size: %dbyte\n", sc->sc_write_buffer_size);
	printf("typical word program timeout: %dusec\n",
	       sc->sc_typ_word_prog_timo);
	printf("maximam word program timeout: %dusec (%d time of typ)\n",
	       sc->sc_typ_word_prog_timo * sc->sc_max_word_prog_timo,
	       sc->sc_max_word_prog_timo);
	printf("typical buffer write timeout: %dusec\n",
	       sc->sc_typ_buffer_write_timo);
	printf("maximam buffer write timeout: %dusec (%d time of typ)\n",
	       sc->sc_typ_buffer_write_timo * sc->sc_max_buffer_write_timo,
	       sc->sc_max_buffer_write_timo);
	printf("typical block erase timeout: %dmsec\n",
	       sc->sc_typ_block_erase_timo);
	printf("maximam block erase timeout: %dmsec (%d time of typ)\n",
	       sc->sc_typ_block_erase_timo * sc->sc_max_block_erase_timo,
	       sc->sc_max_block_erase_timo);

	printf("read_cfi: dump cfi\n");
	for (i = 0; i < CFI_TOTAL_SIZE;) {
		int	j;
		for (j = 0; j < 16; j++) {
			printf("%02x ", sc->sc_cfi_raw[i++]);
		}
		printf("\n");
	}
#endif
}

int
flashopen(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct flash_softc	*sc;

	sc = device_lookup_private(&flash_cd, minor(dev));
	if (sc == NULL)
		return ENXIO;
	if (sc->sc_status & FLASH_ST_BUSY)
		return EBUSY;
	sc->sc_status |= FLASH_ST_BUSY;
	return 0;
}

int
flashclose(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct flash_softc	*sc;

	sc = device_lookup_private(&flash_cd, minor(dev));
	sc->sc_status &= ~FLASH_ST_BUSY;
	return 0;
}

int
flashread(dev_t dev, struct uio *uio, int flag)
{
	struct flash_softc	*sc;
	bus_space_tag_t		iot;
	bus_space_handle_t	ioh;
	bus_size_t		off;
	int			total;
	int			count;
	int			error;

	sc = device_lookup_private(&flash_cd, minor(dev));
	iot = sc->sc_iot;
	ioh = sc->sc_ioh;

	off = uio->uio_offset;
	total = min(sc->sc_size - off, uio->uio_resid);

	while (total > 0) {
		count = min(sc->sc_block_size, uio->uio_resid);
		bus_space_read_region_1(iot, ioh, off, sc->sc_buf, count);
		if ((error = uiomove(sc->sc_buf, count, uio)) != 0)
			return error;
		off += count;
		total -= count;
	}
	return 0;
}


int
flashwrite(dev_t dev, struct uio *uio, int flag)
{
	struct flash_softc	*sc;
	bus_space_tag_t		iot;
	bus_space_handle_t	ioh;
	bus_size_t		off;
	int			stat;
	int			error;

	sc = device_lookup_private(&flash_cd, minor(dev));

	if (sc->sc_size < uio->uio_offset + uio->uio_resid)
		return ENOSPC;
	if (uio->uio_offset % sc->sc_block_size)
		return EINVAL;
	if (uio->uio_resid % sc->sc_block_size)
		return EINVAL;

	iot = sc->sc_iot;
	ioh = sc->sc_ioh;
	
	for (off = uio->uio_offset;
	     uio->uio_resid > 0;
	     off += sc->sc_block_size) {
		if ((error = uiomove(sc->sc_buf, sc->sc_block_size, uio)) != 0)
			return error;
		if (is_block_same(sc, off, sc->sc_buf))
			continue;
		if ((stat = flash_block_erase(sc, off)) != 0) {
			printf("block erase failed status = 0x%x\n", stat);
			return EIO;
		}
		if ((stat = flash_block_write(sc, off)) != 0) {
			printf("block write failed status = 0x%x\n", stat);
			return EIO;
		}
	}
	return 0;
}

/*
 * XXX
 * this function is too much specific for the device.
 */
static int
i28f128_probe(bus_space_tag_t iot, bus_space_handle_t ioh)
{
	static const u_int8_t	vendor_code[] = {
		0x89,	/* manufacturer code: 	intel */
		0x18,	/* device code:		28F128 */
	};

	static const u_int8_t	idstr[] = {
		'Q', 'R', 'Y',
		0x01, 0x00,
		0x31, 0x00,
		0xff
	};

	int	i;

	/* start Common Flash Interface Query */
	bus_space_write_2(iot, ioh, 0, CFI_READ_CFI_QUERY);
	/* read CFI Query ID string */
	for (i = 0; idstr[i] != 0xff; i++) {
		if (bus_space_read_2(iot, ioh, (0x10 + i) << 1) != idstr[i])
			return 1;
	}

	/* read manufacturer code and device code */
	if (bus_space_read_2(iot, ioh, 0x00) != vendor_code[0])
		return 1;
	if (bus_space_read_2(iot, ioh, 0x02) != vendor_code[1])
		return 1;
	
	bus_space_write_2(iot, ioh, 0, I28F128_RESET);
	return 0;
}

/*
 * XXX
 * this function is too much specific for the device.
 */
static int
mbm29160_probe(bus_space_tag_t iot, bus_space_handle_t ioh)
{
	static const u_int16_t	vendor_code[] = {
		0x0004,	/* manufacturer code: 	intel */
		0x2249,	/* device code:		29LV160BE */
	};

	static const u_int8_t	idstr[] = {
		'Q', 'R', 'Y',
		0x02, 0x00,
		0x40, 0x00,
		0xff
	};

	int	i;

	/* start Common Flash Interface Query */
	bus_space_write_2(iot, ioh, 0xaa, CFI_READ_CFI_QUERY);
	/* read CFI Query ID string */
	for (i = 0; idstr[i] != 0xff; i++) {
		if (bus_space_read_2(iot, ioh, (0x10 + i) << 1) != idstr[i])
			return 1;
	}

	bus_space_write_2(iot, ioh, 0, 0xff);

	/* read manufacturer code and device code */
	bus_space_write_2(iot, ioh, 0x555 << 1, 0xaa);
	bus_space_write_2(iot, ioh, 0x2aa << 1, 0x55);
	bus_space_write_2(iot, ioh, 0x555 << 1, 0x90);
	if (bus_space_read_2(iot, ioh, 0x00) != vendor_code[0])
		return 1;
	if (bus_space_read_2(iot, ioh, 0x02) != vendor_code[1])
		return 1;
	
	bus_space_write_2(iot, ioh, 0, 0xff);
	return 0;
}

static int
is_block_same(struct flash_softc *sc, bus_size_t offset, const void *bufp)
{
	bus_space_tag_t		iot = sc->sc_iot;
	bus_space_handle_t	ioh = sc->sc_ioh;
	const u_int8_t		*p = bufp;
	int			count = sc->sc_block_size;

	while (count-- > 0) {
		if (bus_space_read_1(iot, ioh, offset++) != *p++)
			return 0;
	}
	return 1;
}

static int
intel_erase(struct flash_softc *sc, bus_size_t offset)
{
	bus_space_tag_t		iot = sc->sc_iot;
	bus_space_handle_t	ioh = sc->sc_ioh;
	int			status;
	int			i;

	bus_space_write_2(iot, ioh, offset, I28F128_BLK_ERASE_1ST);
	bus_space_write_2(iot, ioh, offset, I28F128_BLK_ERASE_2ND);

	status = 0;
	for (i = sc->sc_max_block_erase_timo; i > 0; i--) {
		tsleep(sc, PRIBIO, "blockerase",
		       1 + (sc->sc_typ_block_erase_timo * hz) / 1000);
		if ((status = bus_space_read_2(iot, ioh, offset))
		    & I28F128_S_READY)
			break;
	}
	if (i == 0) 
		status |= FLASH_TIMEOUT; 

	bus_space_write_2(iot, ioh, offset, I28F128_CLEAR_STATUS);
	bus_space_write_2(iot, ioh, offset, I28F128_RESET);

	return status & (FLASH_TIMEOUT
			 | I28F128_S_ERASE_SUSPEND
			 | I28F128_S_COMSEQ_ERROR
			 | I28F128_S_ERASE_ERROR
			 | I28F128_S_BLOCK_LOCKED);
}

static int
intel_write(struct flash_softc *sc, bus_size_t offset)
{
	bus_space_tag_t		iot = sc->sc_iot;
	bus_space_handle_t	ioh = sc->sc_ioh;
	int			wbuf_size;
	int			timo;
	int			status;
	bus_size_t		fence;
	int			i;
	const u_int16_t		*p;

	/* wbuf_size = size in u_int16_t */
	wbuf_size = sc->sc_write_buffer_size >> 1;

	p = (u_int16_t *) sc->sc_buf;
	fence = offset + sc->sc_block_size;
	do {
		status = 0;
		for (timo = sc->sc_max_buffer_write_timo; timo > 0; timo--) {
			bus_space_write_2(iot, ioh, offset,
					  I28F128_WRITE_BUFFER);
			status = bus_space_read_2(iot, ioh, offset);
			if (status & I28F128_XS_BUF_AVAIL)
				break;
			DELAY(sc->sc_typ_buffer_write_timo);
		}
		if (timo == 0) {
			status |= FLASH_TIMEOUT;
			goto errout;
		}

		bus_space_write_2(iot, ioh, offset, wbuf_size - 1);

		for (i = wbuf_size; i > 0; i--, p++, offset += 2)
			bus_space_write_2(iot, ioh, offset, *p);

		bus_space_write_2(iot, ioh, offset, I28F128_WBUF_CONFIRM);

		do {
			bus_space_write_2(iot, ioh, offset,
					  I28F128_READ_STATUS);
			status = bus_space_read_2(iot, ioh, offset);
		} while (!(status & I28F128_S_READY));

	} while (offset < fence);

	bus_space_write_2(iot, ioh, offset, I28F128_CLEAR_STATUS);
	bus_space_write_2(iot, ioh, offset, I28F128_RESET);

	return 0;

errout:
	bus_space_write_2(iot, ioh, offset, I28F128_CLEAR_STATUS);
	bus_space_write_2(iot, ioh, offset, I28F128_RESET);

	status &= (FLASH_TIMEOUT
		   | I28F128_S_PROG_ERROR
		   | I28F128_S_COMSEQ_ERROR
		   | I28F128_S_LOW_VOLTAGE
		   | I28F128_S_PROG_SUSPEND
		   | I28F128_S_BLOCK_LOCKED);
	return status;
}

static int
amd_erase_sector(struct flash_softc *sc, bus_size_t offset)
{
	bus_space_tag_t		iot = sc->sc_iot;
	bus_space_handle_t	ioh = sc->sc_ioh;
	int			i;

	DPRINTF(("amd_erase_sector offset = %08lx\n", offset));

	bus_space_write_2(iot, ioh,
			  MBM29LV160_COMM_ADDR0, MBM29LV160_COMM_CMD0);
	bus_space_write_2(iot, ioh,
			  MBM29LV160_COMM_ADDR1, MBM29LV160_COMM_CMD1);
	bus_space_write_2(iot, ioh,
			  MBM29LV160_COMM_ADDR2, MBM29LV160_ESECT_CMD2);
	bus_space_write_2(iot, ioh,
			  MBM29LV160_COMM_ADDR3, MBM29LV160_ESECT_CMD3);
	bus_space_write_2(iot, ioh,
			  MBM29LV160_COMM_ADDR4, MBM29LV160_ESECT_CMD4);
	bus_space_write_2(iot, ioh, offset, MBM29LV160_ESECT_CMD5);

	for (i = sc->sc_max_block_erase_timo; i > 0; i--) {
		tsleep(sc, PRIBIO, "blockerase",
		       1 + (sc->sc_typ_block_erase_timo * hz) / 1000);
		if (bus_space_read_2(iot, ioh, offset) == 0xffff)
			return 0;
	}

	return FLASH_TIMEOUT;
}

static int
amd_erase(struct flash_softc *sc, bus_size_t offset)
{
	static const struct mbm29lv_subsect {
		u_int16_t	devcode;
		u_int32_t	subsect_mask;
		u_int32_t	subsect_addr;
	} subsect[] = {
		{
			MBM29LV160TE_DEVCODE,
			MBM29LV160_SUBSECT_MASK,
			MBM29LV160TE_SUBSECT_ADDR
		},
		{
			MBM29LV160BE_DEVCODE,
			MBM29LV160_SUBSECT_MASK,
			MBM29LV160BE_SUBSECT_ADDR
		},
		{ 0, 0, 0 }
	};

	bus_space_tag_t			iot = sc->sc_iot;
	bus_space_handle_t		ioh = sc->sc_ioh;
	u_int16_t			devcode;
	const struct mbm29lv_subsect	*ss;
	bus_size_t			fence;
	int				step;
	int				status;

	bus_space_write_2(iot, ioh,
			  MBM29LV160_COMM_ADDR0, MBM29LV160_COMM_CMD0);
	bus_space_write_2(iot, ioh,
			  MBM29LV160_COMM_ADDR1, MBM29LV160_COMM_CMD1);
	bus_space_write_2(iot, ioh,
			  MBM29LV160_COMM_ADDR2, MBM29LV160_SIGN_CMD2);
	devcode = bus_space_read_2(iot, ioh, MBM29LV160_DEVCODE_REG);

	for (ss = subsect; ss->devcode; ss++) {
		if (ss->devcode == devcode)
			break;
	}
	if (ss->devcode == 0) {
		printf("flash: amd_erase(): unknown device code %04x\n",
		       devcode);
		return -1;
	}

	DPRINTF(("flash: amd_erase(): devcode = %04x subsect = %08x\n",
		 devcode, ss->subsect_addr));

	fence = offset + sc->sc_block_size;
	step = (offset & ss->subsect_mask) == ss->subsect_addr
		? MBM29LV160_SUBSECT_SIZE : MBM29LV160_SECT_SIZE;
	do {
		if ((status = amd_erase_sector(sc, offset)) != 0)
			return status;
		offset += step;
	} while (offset < fence);

	return 0;
}

static int
amd_write(struct flash_softc *sc, bus_size_t offset)
{
	bus_space_tag_t		iot = sc->sc_iot;
	bus_space_handle_t	ioh = sc->sc_ioh;
	int			timo;
	bus_size_t		fence;
	const u_int16_t		*p;

	p = (u_int16_t *) sc->sc_buf;
	fence = offset + sc->sc_block_size;
	do {
		bus_space_write_2(iot, ioh,
				  MBM29LV160_COMM_ADDR0,
				  MBM29LV160_COMM_CMD0);
		bus_space_write_2(iot, ioh,
				  MBM29LV160_COMM_ADDR1,
				  MBM29LV160_COMM_CMD1);
		bus_space_write_2(iot, ioh,
				  MBM29LV160_COMM_ADDR2,
				  MBM29LV160_PROG_CMD2);
		bus_space_write_2(iot, ioh, offset, *p);

		for (timo = sc->sc_max_word_prog_timo; timo > 0; timo--) {
			if (bus_space_read_2(iot, ioh, offset) == *p)
				break;
			DELAY(sc->sc_typ_word_prog_timo);
		}
		if (timo == 0)
			return FLASH_TIMEOUT;

		p++;
		offset += 2;
	} while (offset < fence);

	return 0;
}
