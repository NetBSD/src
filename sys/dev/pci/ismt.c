/*-
 * Copyright (c) 2016 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Masanobu SAITOH.
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

/*-
 * Copyright (C) 2014 Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of Intel Corporation nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#if 0
__FBSDID("$FreeBSD: head/sys/dev/ismt/ismt.c 266474 2014-05-20 19:55:06Z jimharris $");
#endif
__KERNEL_RCSID(0, "$NetBSD: ismt.c,v 1.3.2.3 2016/10/05 20:55:43 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/kernel.h>
#include <sys/module.h>
#include <sys/mutex.h>
#include <sys/proc.h>

#include <sys/bus.h>

#include <dev/pci/pcidevs.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

#include <dev/i2c/i2cvar.h>

#define ISMT_DESC_ENTRIES	32

/* Hardware Descriptor Constants - Control Field */
#define ISMT_DESC_CWRL	0x01	/* Command/Write Length */
#define ISMT_DESC_BLK	0X04	/* Perform Block Transaction */
#define ISMT_DESC_FAIR	0x08	/* Set fairness flag upon successful arbit. */
#define ISMT_DESC_PEC	0x10	/* Packet Error Code */
#define ISMT_DESC_I2C	0x20	/* I2C Enable */
#define ISMT_DESC_INT	0x40	/* Interrupt */
#define ISMT_DESC_SOE	0x80	/* Stop On Error */

/* Hardware Descriptor Constants - Status Field */
#define ISMT_DESC_SCS	0x01	/* Success */
#define ISMT_DESC_DLTO	0x04	/* Data Low Time Out */
#define ISMT_DESC_NAK	0x08	/* NAK Received */
#define ISMT_DESC_CRC	0x10	/* CRC Error */
#define ISMT_DESC_CLTO	0x20	/* Clock Low Time Out */
#define ISMT_DESC_COL	0x40	/* Collisions */
#define ISMT_DESC_LPR	0x80	/* Large Packet Received */

/* Macros */
#define ISMT_DESC_ADDR_RW(addr, is_read) (((addr) << 1) | (is_read))

/* iSMT General Register address offsets (SMBBAR + <addr>) */
#define ISMT_GR_GCTRL		0x000	/* General Control */
#define ISMT_GR_SMTICL		0x008	/* SMT Interrupt Cause Location */
#define ISMT_GR_ERRINTMSK	0x010	/* Error Interrupt Mask */
#define ISMT_GR_ERRAERMSK	0x014	/* Error AER Mask */
#define ISMT_GR_ERRSTS		0x018	/* Error Status */
#define ISMT_GR_ERRINFO		0x01c	/* Error Information */

/* iSMT Master Registers */
#define ISMT_MSTR_MDBA		0x100	/* Master Descriptor Base Address */
#define ISMT_MSTR_MCTRL		0x108	/* Master Control */
#define ISMT_MSTR_MSTS		0x10c	/* Master Status */
#define ISMT_MSTR_MDS		0x110	/* Master Descriptor Size */
#define ISMT_MSTR_RPOLICY	0x114	/* Retry Policy */

/* iSMT Miscellaneous Registers */
#define ISMT_SPGT	0x300	/* SMBus PHY Global Timing */

/* General Control Register (GCTRL) bit definitions */
#define ISMT_GCTRL_TRST	0x04	/* Target Reset */
#define ISMT_GCTRL_KILL	0x08	/* Kill */
#define ISMT_GCTRL_SRST	0x40	/* Soft Reset */

/* Master Control Register (MCTRL) bit definitions */
#define ISMT_MCTRL_SS	0x01		/* Start/Stop */
#define ISMT_MCTRL_MEIE	0x10		/* Master Error Interrupt Enable */
#define ISMT_MCTRL_FMHP	0x00ff0000	/* Firmware Master Head Ptr (FMHP) */

/* Master Status Register (MSTS) bit definitions */
#define ISMT_MSTS_HMTP	0xff0000	/* HW Master Tail Pointer (HMTP) */
#define ISMT_MSTS_MIS	0x20		/* Master Interrupt Status (MIS) */
#define ISMT_MSTS_MEIS	0x10		/* Master Error Int Status (MEIS) */
#define ISMT_MSTS_IP	0x01		/* In Progress */

/* Master Descriptor Size (MDS) bit definitions */
#define ISMT_MDS_MASK	0xff	/* Master Descriptor Size mask (MDS) */

/* SMBus PHY Global Timing Register (SPGT) bit definitions */
#define ISMT_SPGT_SPD_MASK	0xc0000000	/* SMBus Speed mask */
#define ISMT_SPGT_SPD_80K	0x00		/* 80 kHz */
#define ISMT_SPGT_SPD_100K	(0x1 << 30)	/* 100 kHz */
#define ISMT_SPGT_SPD_400K	(0x2 << 30)	/* 400 kHz */
#define ISMT_SPGT_SPD_1M	(0x3 << 30)	/* 1 MHz */

/* MSI Control Register (MSICTL) bit definitions */
#define ISMT_MSICTL_MSIE	0x01	/* MSI Enable */

#define ISMT_MAX_BLOCK_SIZE	32 /* per SMBus spec */

#define ISMT_INTR_TIMEOUT	(hz / 50) /* 0.02s */
#define ISMT_POLL_DELAY		100	/* 100usec */
#define ISMT_POLL_COUNT		200	/* 100usec * 200 = 0.02s */

//#define ISMT_DEBUG	aprint_debug_dev
#ifndef ISMT_DEBUG
#define ISMT_DEBUG(...)
#endif

#define ISMT_LOW(a)	((a) & 0xFFFFFFFFULL)
#define ISMT_HIGH(a)	(((uint64_t)(a) >> 32) & 0xFFFFFFFFFULL)

/* iSMT Hardware Descriptor */
struct ismt_desc {
	uint8_t tgtaddr_rw;	/* target address & r/w bit */
	uint8_t wr_len_cmd;	/* write length in bytes or a command */
	uint8_t rd_len;		/* read length */
	uint8_t control;	/* control bits */
	uint8_t status;		/* status bits */
	uint8_t retry;		/* collision retry and retry count */
	uint8_t rxbytes;	/* received bytes */
	uint8_t txbytes;	/* transmitted bytes */
	uint32_t dptr_low;	/* lower 32 bit of the data pointer */
	uint32_t dptr_high;	/* upper 32 bit of the data pointer */
} __packed;

#define DESC_SIZE	(ISMT_DESC_ENTRIES * sizeof(struct ismt_desc))

#define DMA_BUFFER_SIZE	64

struct ismt_softc {
	device_t		pcidev;
	device_t		smbdev;

	struct i2c_controller	sc_i2c_tag;
	kmutex_t 		sc_i2c_mutex;

	pci_chipset_tag_t	sc_pc;
	pcitag_t		sc_pcitag;
	pci_intr_handle_t	*sc_pihp;
	void			*sc_ih;

	bus_space_tag_t		mmio_tag;
	bus_space_handle_t	mmio_handle;
	bus_size_t		mmio_size;

	uint8_t			head;

	struct ismt_desc	*desc;
	bus_dma_tag_t		desc_dma_tag;
	bus_dmamap_t		desc_dma_map;
	bus_dma_segment_t	desc_dma_seg;
	int			desc_rseg;

	uint8_t			*dma_buffer;
	bus_dma_tag_t		dma_buffer_dma_tag;
	bus_dmamap_t		dma_buffer_dma_map;
	bus_dma_segment_t	dma_buffer_dma_seg;
	int			dma_buffer_rseg;

	uint8_t			using_msi;
};

static int	ismt_intr(void *);
static int	ismt_i2c_acquire_bus(void *, int);
static void	ismt_i2c_release_bus(void *, int);
static int	ismt_i2c_exec(void *, i2c_op_t, i2c_addr_t, const void *,
    size_t, void *, size_t, int);
static struct ismt_desc *ismt_alloc_desc(struct ismt_softc *);
static int	ismt_submit(struct ismt_softc *, struct ismt_desc *,
    i2c_addr_t, uint8_t, int);
static int	ismt_quick(struct ismt_softc *, i2c_addr_t, i2c_op_t, int);
static int	ismt_sendb(struct ismt_softc *, i2c_addr_t, i2c_op_t, char,
    int);
static int	ismt_recvb(struct ismt_softc *, i2c_addr_t, i2c_op_t, int);
static int	ismt_writeb(struct ismt_softc *, i2c_addr_t, i2c_op_t, uint8_t,
    char, int);
static int	ismt_writew(struct ismt_softc *, i2c_addr_t, i2c_op_t, uint8_t,
    uint16_t, int);
static int	ismt_readb(struct ismt_softc *, i2c_addr_t, i2c_op_t, char,
    int);
static int	ismt_readw(struct ismt_softc *, i2c_addr_t, i2c_op_t, char,
    int);

static int	ismt_match(device_t, cfdata_t, void *);
static void	ismt_attach(device_t, device_t, void *);
static int	ismt_detach(device_t, int);
static int	ismt_rescan(device_t, const char *, const int *);
static void	ismt_config_interrupts(device_t);
static void	ismt_chdet(device_t, device_t);

CFATTACH_DECL3_NEW(ismt, sizeof(struct ismt_softc),
    ismt_match, ismt_attach, ismt_detach, NULL, ismt_rescan, ismt_chdet,
    DVF_DETACH_SHUTDOWN);

static int
ismt_intr(void *arg)
{
	struct ismt_softc *sc = arg;
	uint32_t val;

	val = bus_space_read_4(sc->mmio_tag, sc->mmio_handle, ISMT_MSTR_MSTS);
	if ((sc->using_msi == 0)
	    && (val & (ISMT_MSTS_MIS | ISMT_MSTS_MEIS)) == 0)
		return 0; /* Not for me */

	ISMT_DEBUG(sc->pcidev, "%s MSTS = 0x%08x\n", __func__, val);

	val |= (ISMT_MSTS_MIS | ISMT_MSTS_MEIS);
	bus_space_write_4(sc->mmio_tag, sc->mmio_handle, ISMT_MSTR_MSTS, val);

	if (sc->using_msi)
		wakeup(sc);

	return 1;
}

static int
ismt_i2c_acquire_bus(void *cookie, int flags)
{
	struct ismt_softc *sc = cookie;

	mutex_enter(&sc->sc_i2c_mutex);
	return 0;
}

static void
ismt_i2c_release_bus(void *cookie, int flags)
{
	struct ismt_softc *sc = cookie;

	mutex_exit(&sc->sc_i2c_mutex);
}

static int
ismt_i2c_exec(void *cookie, i2c_op_t op, i2c_addr_t addr,
    const void *cmd, size_t cmdlen, void *buf, size_t buflen, int flags)
{
	struct ismt_softc *sc = cookie;
	uint8_t *p = buf;
	int rv;

	ISMT_DEBUG(sc->pcidev, "exec: op %d, addr 0x%02x, cmdlen %zu, "
 	    " buflen %zu, flags 0x%02x\n", op, addr, cmdlen, buflen, flags);

	if ((cmdlen == 0) && (buflen == 0))
		return ismt_quick(sc, addr, op, flags);

	if (I2C_OP_READ_P(op) && (cmdlen == 0) && (buflen == 1)) {
		rv = ismt_recvb(sc, addr, op, flags);
		if (rv == -1)
			return -1;
		*p = (uint8_t)rv;
		return 0;
	}

	if ((I2C_OP_READ_P(op)) && (cmdlen == 1) && (buflen == 1)) {
		rv = ismt_readb(sc, addr, op, *(const uint8_t*)cmd, flags);
		if (rv == -1)
			return -1;
		*p = (uint8_t)rv;
		return 0;
	}

	if ((I2C_OP_READ_P(op)) && (cmdlen == 1) && (buflen == 2)) {
		rv = ismt_readw(sc, addr, op, *(const uint8_t*)cmd, flags);
		if (rv == -1)
			return -1;
		*(uint16_t *)p = (uint16_t)rv;
		return 0;
	}

	if ((I2C_OP_WRITE_P(op)) && (cmdlen == 0) && (buflen == 1))
		return ismt_sendb(sc, addr, op, *(uint8_t*)buf, flags);

	if ((I2C_OP_WRITE_P(op)) && (cmdlen == 1) && (buflen == 1))
		return ismt_writeb(sc, addr, op, *(const uint8_t*)cmd,
		    *(uint8_t*)buf, flags);

	if ((I2C_OP_WRITE_P(op)) && (cmdlen == 1) && (buflen == 2))
		return ismt_writew(sc, addr, op,
		    *(const uint8_t*)cmd, *((uint16_t *)buf), flags);

	return -1;
}

static struct ismt_desc *
ismt_alloc_desc(struct ismt_softc *sc)
{
	struct ismt_desc *desc;

	KASSERT(mutex_owned(&sc->sc_i2c_mutex));

	desc = &sc->desc[sc->head++];
	if (sc->head == ISMT_DESC_ENTRIES)
		sc->head = 0;

	memset(desc, 0, sizeof(*desc));

	return (desc);
}

static int
ismt_submit(struct ismt_softc *sc, struct ismt_desc *desc, i2c_addr_t slave,
    uint8_t is_read, int flags)
{
	uint32_t err, fmhp, val;
	int timeout, i;

	if (sc->using_msi == 0)
		flags |= I2C_F_POLL;
	desc->control |= ISMT_DESC_FAIR;
	if ((flags & I2C_F_POLL) == 0)
		desc->control |= ISMT_DESC_INT;

	desc->tgtaddr_rw = ISMT_DESC_ADDR_RW(slave, is_read);
	desc->dptr_low = ISMT_LOW(sc->dma_buffer_dma_map->dm_segs[0].ds_addr);
	desc->dptr_high = ISMT_HIGH(sc->dma_buffer_dma_map->dm_segs[0].ds_addr);

	bus_dmamap_sync(sc->desc_dma_tag, sc->desc_dma_map,
	    desc - &sc->desc[0], sizeof(struct ismt_desc),
	    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);

	fmhp = sc->head << 16;
	val = bus_space_read_4(sc->mmio_tag, sc->mmio_handle, ISMT_MSTR_MCTRL);
	val &= ~ISMT_MCTRL_FMHP;
	val |= fmhp;
	bus_space_write_4(sc->mmio_tag, sc->mmio_handle, ISMT_MSTR_MCTRL, val);

	/* set the start bit */
	val = bus_space_read_4(sc->mmio_tag, sc->mmio_handle, ISMT_MSTR_MCTRL);
	val |= ISMT_MCTRL_SS;
	bus_space_write_4(sc->mmio_tag, sc->mmio_handle, ISMT_MSTR_MCTRL, val);

	i = 0;
	if ((flags & I2C_F_POLL) == 0) {
		timeout = ISMT_INTR_TIMEOUT;
		if (timeout == 0)
			timeout = 1;
		err = tsleep(sc, PWAIT, "ismt_wait", timeout);
		if (err != 0) {
			ISMT_DEBUG(sc->pcidev, "%s timeout\n", __func__);
			return -1;
		}
	} else {
		/* Polling */
		for (i = 0; i < ISMT_POLL_COUNT; i++) {
			val = bus_space_read_4(sc->mmio_tag, sc->mmio_handle,
			    ISMT_MSTR_MSTS);
			if ((val & (ISMT_MSTS_MIS | ISMT_MSTS_MEIS)) != 0) {
				ismt_intr(sc);
				err = 0;
				break;
			}
			delay(ISMT_POLL_DELAY);
		}
		if (i >= ISMT_POLL_COUNT) {
			ISMT_DEBUG(sc->pcidev, "%s polling timeout. "
			    "MSTS = %08x\n", __func__, val);
			return -1;
		}
	}

	bus_dmamap_sync(sc->desc_dma_tag, sc->desc_dma_map,
	    desc - &sc->desc[0], sizeof(struct ismt_desc),
		BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);
	ISMT_DEBUG(sc->pcidev, "%s status=0x%02x\n", __func__, desc->status);

	if (desc->status & ISMT_DESC_SCS)
		return 0;

	if (desc->status & ISMT_DESC_NAK)
		return -1;

	if (desc->status & ISMT_DESC_CRC)
		return -1;

	if (desc->status & ISMT_DESC_COL)
		return -1;

	if (desc->status & ISMT_DESC_LPR)
		return -1;

	if (desc->status & (ISMT_DESC_DLTO | ISMT_DESC_CLTO))
		return -1;

	return -1;
}

static int
ismt_quick(struct ismt_softc *sc, i2c_addr_t slave, i2c_op_t op, int flags)
{
	struct ismt_desc	*desc;
	int			is_read;

	ISMT_DEBUG(sc->pcidev, "%s\n", __func__);

	desc = ismt_alloc_desc(sc);
	is_read = I2C_OP_READ_P(op);
	return (ismt_submit(sc, desc, slave, is_read, flags));
}

static int
ismt_sendb(struct ismt_softc *sc, i2c_addr_t slave, i2c_op_t op, char byte,
    int flags)
{
	struct ismt_desc	*desc;

	ISMT_DEBUG(sc->pcidev, "%s\n", __func__);

	desc = ismt_alloc_desc(sc);
	desc->control = ISMT_DESC_CWRL;
	desc->wr_len_cmd = byte;

	return (ismt_submit(sc, desc, slave, 0, flags));
}

static int
ismt_recvb(struct ismt_softc *sc, i2c_addr_t slave, i2c_op_t op, int flags)
{
	struct ismt_desc	*desc;
	int			err;

	ISMT_DEBUG(sc->pcidev, "%s\n", __func__);

	desc = ismt_alloc_desc(sc);
	desc->rd_len = 1;

	err = ismt_submit(sc, desc, slave, 1, flags);

	if (err != 0)
		return (err);

	return sc->dma_buffer[0];
}

static int
ismt_writeb(struct ismt_softc *sc, i2c_addr_t slave, i2c_op_t op, uint8_t cmd,
    char byte, int flags)
{
	struct ismt_desc	*desc;

	ISMT_DEBUG(sc->pcidev, "%s\n", __func__);

	desc = ismt_alloc_desc(sc);
	desc->wr_len_cmd = 2;
	sc->dma_buffer[0] = cmd;
	sc->dma_buffer[1] = byte;

	return (ismt_submit(sc, desc, slave, 0, flags));
}

static int
ismt_writew(struct ismt_softc *sc, i2c_addr_t slave, i2c_op_t op, uint8_t cmd,
    uint16_t word, int flags)
{
	struct ismt_desc	*desc;

	ISMT_DEBUG(sc->pcidev, "%s\n", __func__);

	desc = ismt_alloc_desc(sc);
	desc->wr_len_cmd = 3;
	sc->dma_buffer[0] = cmd;
	sc->dma_buffer[1] = word & 0xFF;
	sc->dma_buffer[2] = word >> 8;

	return (ismt_submit(sc, desc, slave, 0, flags));
}

static int
ismt_readb(struct ismt_softc *sc, i2c_addr_t slave, i2c_op_t op, char cmd,
    int flags)
{
	struct ismt_desc	*desc;
	int			err;

	ISMT_DEBUG(sc->pcidev, "%s\n", __func__);

	desc = ismt_alloc_desc(sc);
	desc->control = ISMT_DESC_CWRL;
	desc->wr_len_cmd = cmd;
	desc->rd_len = 1;

	err = ismt_submit(sc, desc, slave, 1, flags);

	if (err != 0)
		return (err);

	return sc->dma_buffer[0];
}

static int
ismt_readw(struct ismt_softc *sc, i2c_addr_t slave, i2c_op_t op, char cmd,
    int flags)
{
	struct ismt_desc	*desc;
	uint16_t		word;
	int			err;

	ISMT_DEBUG(sc->pcidev, "%s\n", __func__);

	desc = ismt_alloc_desc(sc);
	desc->control = ISMT_DESC_CWRL;
	desc->wr_len_cmd = cmd;
	desc->rd_len = 2;

	err = ismt_submit(sc, desc, slave, 1, flags);

	if (err != 0)
		return (err);

	word = sc->dma_buffer[0] | (sc->dma_buffer[1] << 8);

	return word;
}

#if 0
static int
ismt_pcall(struct ismt_softc *sc, i2c_addr_t slave, i2c_op_t op, char cmd,
    uint16_t sdata, uint16_t *rdata, int flags)
{
	struct ismt_desc	*desc;
	int			err;

	ISMT_DEBUG(sc->pcidev, "%s\n", __func__);

	desc = ismt_alloc_desc(sc);
	desc->wr_len_cmd = 3;
	desc->rd_len = 2;
	sc->dma_buffer[0] = cmd;
	sc->dma_buffer[1] = sdata & 0xff;
	sc->dma_buffer[2] = sdata >> 8;

	err = ismt_submit(sc, desc, slave, 0, flags);

	if (err != 0)
		return (err);

	*rdata = sc->dma_buffer[0] | (sc->dma_buffer[1] << 8);

	return (err);
}

static int
ismt_bwrite(struct ismt_softc *sc, i2c_addr_t slave, i2c_op_t op, char cmd,
    u_char count, char *buf, int flags)
{
	struct ismt_desc	*desc;

	ISMT_DEBUG(sc->pcidev, "%s\n", __func__);

	if (count == 0 || count > ISMT_MAX_BLOCK_SIZE)
		return -1;

	desc = ismt_alloc_desc(sc);
	desc->control = ISMT_DESC_I2C;
	desc->wr_len_cmd = count + 1;
	sc->dma_buffer[0] = cmd;
	memcpy(&sc->dma_buffer[1], buf, count);

	return (ismt_submit(sc, desc, slave, 0, flags));
}

static int
ismt_bread(struct ismt_softc *sc, i2c_addr_t slave, i2c_op_t op, char cmd,
    u_char *count, char *buf, int flags)
{
	struct ismt_desc	*desc;
	int			err;

	ISMT_DEBUG(sc->pcidev, "%s\n", __func__);

	if (*count == 0 || *count > ISMT_MAX_BLOCK_SIZE)
		return -1;

	desc = ismt_alloc_desc(sc);
	desc->control = ISMT_DESC_I2C | ISMT_DESC_CWRL;
	desc->wr_len_cmd = cmd;
	desc->rd_len = *count;

	err = ismt_submit(sc, desc, slave, 0, flags);

	if (err != 0)
		return (err);

	memcpy(buf, sc->dma_buffer, desc->rxbytes);
	*count = desc->rxbytes;

	return (err);
}
#endif

static int
ismt_detach(device_t self, int flags)
{
	struct ismt_softc	*sc;
	int rv = 0;

	ISMT_DEBUG(self, "%s\n", __func__);
	sc = device_private(self);
	if (sc->smbdev != NULL) {
		rv = config_detach(sc->smbdev, flags);
		if (rv != 0)
			return rv;
	}
	if (sc->sc_ih != NULL) {
		pci_intr_disestablish(sc->sc_pc, sc->sc_ih);
		sc->sc_ih = NULL;
	}
	if (sc->sc_pihp != NULL) {
		pci_intr_release(sc->sc_pc, sc->sc_pihp, 1);
		sc->sc_pihp = NULL;
	}

	bus_dmamap_unload(sc->desc_dma_tag, sc->desc_dma_map);
	bus_dmamap_unload(sc->dma_buffer_dma_tag, sc->dma_buffer_dma_map);

	bus_dmamem_free(sc->desc_dma_tag, &sc->desc_dma_seg, sc->desc_rseg);
	bus_dmamem_free(sc->dma_buffer_dma_tag, &sc->dma_buffer_dma_seg,
	    sc->dma_buffer_rseg);

	if (sc->mmio_size)
		bus_space_unmap(sc->mmio_tag, sc->mmio_handle, sc->mmio_size);

	mutex_destroy(&sc->sc_i2c_mutex);
	return rv;
}

static void
ismt_attach(device_t parent, device_t self, void *aux)
{
	struct ismt_softc *sc = device_private(self);
	struct pci_attach_args *pa = aux;
	const char *intrstr = NULL;
	char intrbuf[PCI_INTRSTR_LEN];
	pcireg_t reg;
	int val;

	sc->pcidev = self;
	sc->sc_pc = pa->pa_pc;
	sc->sc_pcitag = pa->pa_tag;

	/* Enable busmastering */
	reg = pci_conf_read(sc->sc_pc, sc->sc_pcitag, PCI_COMMAND_STATUS_REG);
	reg |= PCI_COMMAND_MASTER_ENABLE;
	pci_conf_write(sc->sc_pc, sc->sc_pcitag, PCI_COMMAND_STATUS_REG, reg);

	pci_aprint_devinfo(pa, NULL);

	/* Map mem space */
	if (pci_mapreg_map(pa, PCI_BAR0, PCI_MAPREG_TYPE_MEM, 0,
	    &sc->mmio_tag, &sc->mmio_handle, NULL, &sc->mmio_size)) {
		aprint_error_dev(self, "can't map mem space\n");
		goto fail;
	}

	if (pci_dma64_available(pa)) {
		sc->desc_dma_tag = pa->pa_dmat64;
		sc->dma_buffer_dma_tag = pa->pa_dmat64;
	} else {
		sc->desc_dma_tag = pa->pa_dmat;
		sc->dma_buffer_dma_tag = pa->pa_dmat;
	}
	bus_dmamem_alloc(sc->desc_dma_tag, DESC_SIZE, PAGE_SIZE, 0,
	    &sc->desc_dma_seg, ISMT_DESC_ENTRIES, &sc->desc_rseg,
	    BUS_DMA_WAITOK);
	bus_dmamem_alloc(sc->dma_buffer_dma_tag, DMA_BUFFER_SIZE, PAGE_SIZE, 0,
	    &sc->dma_buffer_dma_seg, 1, &sc->dma_buffer_rseg, BUS_DMA_WAITOK);

	bus_dmamem_map(sc->desc_dma_tag, &sc->desc_dma_seg,
	    sc->desc_rseg, DESC_SIZE, (void **)&sc->desc, BUS_DMA_COHERENT);
	bus_dmamem_map(sc->dma_buffer_dma_tag, &sc->dma_buffer_dma_seg,
	    sc->dma_buffer_rseg, DMA_BUFFER_SIZE, (void **)&sc->dma_buffer,
	    BUS_DMA_COHERENT);

	bus_dmamap_create(sc->desc_dma_tag, DESC_SIZE, 1,
	    DESC_SIZE, 0, 0, &sc->desc_dma_map);
	bus_dmamap_create(sc->dma_buffer_dma_tag, DMA_BUFFER_SIZE, 1,
	    DMA_BUFFER_SIZE, 0, 0, &sc->dma_buffer_dma_map);

	bus_dmamap_load(sc->desc_dma_tag,
	    sc->desc_dma_map, sc->desc, DESC_SIZE, NULL, 0);
	bus_dmamap_load(sc->dma_buffer_dma_tag,
	    sc->dma_buffer_dma_map, sc->dma_buffer, DMA_BUFFER_SIZE,
	    NULL, 0);

	bus_space_write_4(sc->mmio_tag, sc->mmio_handle, ISMT_MSTR_MDBA,
	    ISMT_LOW(sc->desc_dma_map->dm_segs[0].ds_addr));
	bus_space_write_4(sc->mmio_tag, sc->mmio_handle, ISMT_MSTR_MDBA + 4,
	    ISMT_HIGH(sc->desc_dma_map->dm_segs[0].ds_addr));

	/* initialize the Master Control Register (MCTRL) */
	bus_space_write_4(sc->mmio_tag, sc->mmio_handle, ISMT_MSTR_MCTRL,
	    ISMT_MCTRL_MEIE);

	/* initialize the Master Status Register (MSTS) */
	bus_space_write_4(sc->mmio_tag, sc->mmio_handle, ISMT_MSTR_MSTS, 0);

	/* initialize the Master Descriptor Size (MDS) */
	val = bus_space_read_4(sc->mmio_tag, sc->mmio_handle, ISMT_MSTR_MDS);
	val &= ~ISMT_MDS_MASK;
	val |= (ISMT_DESC_ENTRIES - 1);
	bus_space_write_4(sc->mmio_tag, sc->mmio_handle, ISMT_MSTR_MDS, val);

	if (pci_intr_alloc(pa, &sc->sc_pihp, NULL, 0)) {
		aprint_error_dev(self, "couldn't map interrupt\n");
		return;
	}
	intrstr = pci_intr_string(pa->pa_pc, sc->sc_pihp[0], intrbuf,
	    sizeof(intrbuf));
	sc->sc_ih = pci_intr_establish(pa->pa_pc, sc->sc_pihp[0], IPL_BIO,
	    ismt_intr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(sc->pcidev, "unable to establish %s\n",
		    (pci_intr_type(pa->pa_pc, sc->sc_pihp[0])
			== PCI_INTR_TYPE_MSI) ? "MSI" : "INTx");
		/* Polling */
	}

	if (pci_intr_type(pa->pa_pc, sc->sc_pihp[0]) == PCI_INTR_TYPE_MSI)
		sc->using_msi = 1;

	aprint_normal_dev(sc->pcidev, "interrupting at %s\n", intrstr);

	sc->smbdev = NULL;
	mutex_init(&sc->sc_i2c_mutex, MUTEX_DEFAULT, IPL_NONE);
	if (!pmf_device_register(self, NULL, NULL))
		aprint_error_dev(self, "couldn't establish power handler\n");

	config_interrupts(self, ismt_config_interrupts);
	return;

fail:
	ismt_detach(sc->pcidev, 0);

	return;
}

static int
ismt_match(device_t parent, cfdata_t match, void *aux)
{
	struct pci_attach_args *pa = aux;

	if (PCI_VENDOR(pa->pa_id) != PCI_VENDOR_INTEL)
		return 0;

	switch (PCI_PRODUCT(pa->pa_id)) {
	case PCI_PRODUCT_INTEL_S1200_SMBUS_0:
	case PCI_PRODUCT_INTEL_S1200_SMBUS_1:
	case PCI_PRODUCT_INTEL_C2000_SMBUS:
		break;
	default:
		return 0;
	}

	return 1;
}

static int
ismt_rescan(device_t self, const char *ifattr, const int *flags)
{
	struct ismt_softc *sc = device_private(self);
	struct i2cbus_attach_args iba;

	if (!ifattr_match(ifattr, "i2cbus"))
		return 0;

	if (sc->smbdev)
		return 0;

	/* Attach I2C bus */
	sc->sc_i2c_tag.ic_cookie = sc;
	sc->sc_i2c_tag.ic_acquire_bus = ismt_i2c_acquire_bus;
	sc->sc_i2c_tag.ic_release_bus = ismt_i2c_release_bus;
	sc->sc_i2c_tag.ic_exec = ismt_i2c_exec;

	memset(&iba, 0, sizeof(iba));
	iba.iba_type = I2C_TYPE_SMBUS;
	iba.iba_tag = &sc->sc_i2c_tag;
	sc->smbdev = config_found_ia(self, ifattr, &iba, iicbus_print);

	return 0;
}

static void
ismt_config_interrupts(device_t self)
{
	int flags = 0;

	ismt_rescan(self, "i2cbus", &flags);
}

static void
ismt_chdet(device_t self, device_t child)
{
	struct ismt_softc *sc = device_private(self);

	if (sc->smbdev == child)
		sc->smbdev = NULL;

}

MODULE(MODULE_CLASS_DRIVER, ismt, "pci");

#ifdef _MODULE
#include "ioconf.c"
#endif

static int
ismt_modcmd(modcmd_t cmd, void *opaque)
{
	int error = 0;

	switch (cmd) {
	case MODULE_CMD_INIT:
#ifdef _MODULE
		error = config_init_component(cfdriver_ioconf_ismt,
		    cfattach_ioconf_ismt, cfdata_ioconf_ismt);
#endif
		return error;
	case MODULE_CMD_FINI:
#ifdef _MODULE
		error = config_fini_component(cfdriver_ioconf_ismt,
		    cfattach_ioconf_ismt, cfdata_ioconf_ismt);
#endif
		return error;
	default:
		return ENOTTY;
	}
}
