/* $Id: imx23_apbdmavar.h,v 1.1.6.3 2017/12/03 11:35:53 jdolecek Exp $ */

/*
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Petri Laakso.
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

#ifndef _ARM_IMX_IMX23_APBDMAVAR_H_
#define _ARM_IMX_IMX23_APBDMAVAR_H_

#include <sys/cdefs.h>
#include <sys/types.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/mutex.h>

/* DMA command control register bits. */
#define APBDMA_CMD_XFER_COUNT		__BITS(31, 16)
#define APBDMA_CMD_CMDPIOWORDS		__BITS(15, 12)
#define APBDMA_CMD_RESERVED		__BITS(11, 9)
#define APBDMA_CMD_HALTONTERMINATE	__BIT(8)
#define APBDMA_CMD_WAIT4ENDCMD		__BIT(7)
#define APBDMA_CMD_SEMAPHORE		__BIT(6)
#define APBDMA_CMD_NANDWAIT4READY	__BIT(5)
#define APBDMA_CMD_NANDLOCK		__BIT(4)
#define APBDMA_CMD_IRQONCMPLT		__BIT(3)
#define APBDMA_CMD_CHAIN		__BIT(2)
#define APBDMA_CMD_COMMAND		__BITS(1, 0)

/* DMA command types. */
#define APBDMA_CMD_NO_DMA_XFER		0
#define APBDMA_CMD_DMA_WRITE		1
#define APBDMA_CMD_DMA_READ		2
#define APBDMA_CMD_DMA_SENSE		3

/* Flags. */
#define F_APBH_DMA			__BIT(0)
#define F_APBX_DMA			__BIT(1)

/* Number of channels. */
#define AHBH_DMA_CHANNELS		8
#define AHBX_DMA_CHANNELS		16

/* APBH DMA channel assignments. */
#define APBH_DMA_CHANNEL_RES0		0	/* Reserved. */
#define APBH_DMA_CHANNEL_SSP1		1	/* SSP1. */
#define APBH_DMA_CHANNEL_SSP2		2	/* SSP2. */
#define APBH_DMA_CHANNEL_RES1		3	/* Reserved. */
#define APBH_DMA_CHANNEL_NAND_DEVICE0	4	/* NAND_DEVICE0. */
#define APBH_DMA_CHANNEL_NAND_DEVICE1	5	/* NAND_DEVICE1. */
#define APBH_DMA_CHANNEL_NAND_DEVICE2	6	/* NAND_DEVICE2. */
#define APBH_DMA_CHANNEL_NAND_DEVICE3	7	/* NAND_DEVICE3. */

/* APBX DMA channel assignments. */
#define APBX_DMA_CHANNEL_AUDIO_ADC	0	/* Audio ADCs. */
#define APBX_DMA_CHANNEL_AUDIO_DAC	1	/* Audio DACs. */
#define APBX_DMA_CHANNEL_SPDIF_TX	2	/* SPDIF TX. */
#define APBX_DMA_CHANNEL_I2C		3	/* I2C. */
#define APBX_DMA_CHANNEL_SAIF1		4	/* SAIF1. */
#define APBX_DMA_CHANNEL_RES0		5	/* Reserved. */
#define APBX_DMA_CHANNEL_UART1_RX	6	/* UART1 RX, IrDA RX. */
#define APBX_DMA_CHANNEL_UART1_TX	7	/* UART1 TX, IrDA TX. */
#define APBX_DMA_CHANNEL_UART2_RX	8	/* UART2 RX. */
#define APBX_DMA_CHANNEL_UART2_TX	9	/* UART2 TX. */
#define APBX_DMA_CHANNEL_SAIF2		10	/* SAIF2. */
#define APBX_DMA_CHANNEL_RES1		11	/* Reserved. */
#define APBX_DMA_CHANNEL_RES2		12	/* Reserved. */
#define APBX_DMA_CHANNEL_RES3		13	/* Reserved. */
#define APBX_DMA_CHANNEL_RES4		14	/* Reserved. */
#define APBX_DMA_CHANNEL_RES5		15	/* Reserved. */

/* Return codes for apbdma_intr_status() */
#define DMA_IRQ_CMDCMPLT		0
#define DMA_IRQ_TERM			1
#define DMA_IRQ_BUS_ERROR		2

#define PIO_WORDS_MAX			15

/*
 * How many PIO words apbdma_command structure has.
 *
 * XXX: If you change this value, make sure drivers are prepared for that.
 * That means you have to allocate enough DMA memory for command chains.
 */
#define PIO_WORDS			3

typedef struct apbdma_softc {
	device_t sc_dev;
	bus_dma_tag_t sc_dmat;
	bus_space_handle_t sc_ioh;
	bus_space_tag_t sc_iot;
	kmutex_t sc_lock;
	u_int flags;
} *apbdma_softc_t;

typedef struct apbdma_command {
	void *next;		/* Physical address. */
	uint32_t control;
	void *buffer;		/* Physical address. */
	uint32_t pio_words[PIO_WORDS];
} *apbdma_command_t;

void apbdma_cmd_chain(apbdma_command_t, apbdma_command_t, void *, bus_dmamap_t);
void apbdma_cmd_buf(apbdma_command_t, bus_addr_t, bus_dmamap_t);
void apbdma_chan_init(struct apbdma_softc *, unsigned int);
void apbdma_chan_set_chain(struct apbdma_softc *, unsigned int, bus_dmamap_t);
void apbdma_run(struct apbdma_softc *, unsigned int);
void apbdma_ack_intr(struct apbdma_softc *, unsigned int);
void apbdma_ack_error_intr(struct apbdma_softc *, unsigned int);
unsigned int apbdma_intr_status(struct apbdma_softc *, unsigned int);
void apbdma_chan_reset(struct apbdma_softc *, unsigned int);
void apbdma_wait(struct apbdma_softc *, unsigned int);

#endif /* !_ARM_IMX_IMX23_APBDMAVAR_H_ */
