/*	$NetBSD: wdcvar.h,v 1.76 2004/08/19 23:25:35 thorpej Exp $	*/

/*-
 * Copyright (c) 1998, 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum, by Onno van der Linden and by Manuel Bouyer.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *	notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *	notice, this list of conditions and the following disclaimer in the
 *	documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#ifndef _DEV_IC_WDCVAR_H_
#define	_DEV_IC_WDCVAR_H_

#include <sys/callout.h>

#include <dev/ic/wdcreg.h>

#define	WAITTIME    (10 * hz)    /* time to wait for a completion */
	/* this is a lot for hard drives, but not for cdroms */

#define WDC_NREG	8 /* number of command registers */
#define	WDC_NSHADOWREG	2 /* number of command "shadow" registers */

struct wdc_regs {
	/* Our registers */
	bus_space_tag_t       cmd_iot;
	bus_space_handle_t    cmd_baseioh;
	bus_space_handle_t    cmd_iohs[WDC_NREG+WDC_NSHADOWREG];
	bus_space_tag_t       ctl_iot;
	bus_space_handle_t    ctl_ioh;

	/* data32{iot,ioh} are only used for 32-bit data xfers */
	bus_space_tag_t       data32iot;
	bus_space_handle_t    data32ioh;
};

/*
 * Per-controller data
 */
struct wdc_softc {
	struct device sc_dev;		/* generic device info */

	struct wdc_regs *regs;		/* register array (per-channel) */

	int           cap;		/* controller capabilities */
#define	WDC_CAPABILITY_DATA16	0x0001	/* can do 16-bit data access */
#define	WDC_CAPABILITY_DATA32	0x0002	/* can do 32-bit data access */
#define	WDC_CAPABILITY_DMA	0x0008	/* DMA */
#define	WDC_CAPABILITY_UDMA	0x0010	/* Ultra-DMA/33 */
#define	WDC_CAPABILITY_ATA_NOSTREAM 0x0040 /* Don't use stream funcs on ATA */
#define	WDC_CAPABILITY_ATAPI_NOSTREAM 0x0080 /* Don't use stream f on ATAPI */
#define WDC_CAPABILITY_NO_EXTRA_RESETS 0x0100 /* only reset once */
#define WDC_CAPABILITY_PREATA	0x0200	/* ctrl can be a pre-ata one */
#define WDC_CAPABILITY_NOIRQ	0x1000	/* Controller never interrupts */
#define	WDC_CAPABILITY_RAID	0x4000	/* Controller "supports" RAID */
	u_int8_t      PIO_cap;		/* highest PIO mode supported */
	u_int8_t      DMA_cap;		/* highest DMA mode supported */
	u_int8_t      UDMA_cap;		/* highest UDMA mode supported */
	int nchannels;			/* # channels on this controller */
	struct ata_channel **channels;  /* channel-specific data (array) */

	/*
	 * The reference count here is used for both IDE and ATAPI devices.
	 */
	struct atapi_adapter sc_atapi_adapter;

	/* Function used to probe for drives. */
	void		(*drv_probe)(struct ata_channel *);

	/* if WDC_CAPABILITY_DMA set in 'cap' */
	void            *dma_arg;
	int            (*dma_init)(void *, int, int, void *, size_t, int);
	void           (*dma_start)(void *, int, int);
	int            (*dma_finish)(void *, int, int, int);
/* flags passed to dma_init */
#define WDC_DMA_READ	0x01
#define WDC_DMA_IRQW	0x02
#define WDC_DMA_LBA48	0x04

/* values passed to dma_finish */
#define WDC_DMAEND_END	0	/* check for proper end of a DMA xfer */
#define WDC_DMAEND_ABRT 1	/* abort a DMA xfer, verbose */
#define WDC_DMAEND_ABRT_QUIET 2	/* abort a DMA xfer, quiet */

	int		dma_status; /* status returned from dma_finish() */
#define WDC_DMAST_NOIRQ	0x01	/* missing IRQ */
#define WDC_DMAST_ERR	0x02	/* DMA error */
#define WDC_DMAST_UNDER	0x04	/* DMA underrun */

	/* Optional callbacks to lock/unlock hardware. */
	int            (*claim_hw)(void *, int);
	void            (*free_hw)(void *);

	/*
	 * Optional callback to set drive mode.  Required for anything
	 * but basic PIO operation.
	 */
	void 		(*set_modes)(struct ata_channel *);

	/* Optional callback to select drive. */
	void		(*select)(struct ata_channel *,int);

	/* Optional callback to ack IRQ. */
	void		(*irqack)(struct ata_channel *);

	/* overridden if the backend has a different data transfer method */
	void	(*datain_pio)(struct ata_channel *, int, void *, size_t);
	void	(*dataout_pio)(struct ata_channel *, int, void *, size_t);
};

/* Given an ata_channel, get the wdc_softc. */
#define	CHAN_TO_WDC(chp)	((chp)->ch_wdc)

/* Given an ata_channel, get the wdc_regs. */
#define	CHAN_TO_WDC_REGS(chp)	(&CHAN_TO_WDC(chp)->regs[(chp)->ch_channel])

/*
 * Public functions which can be called by ATA or ATAPI specific parts,
 * or bus-specific backends.
 */

void	wdc_allocate_regs(struct wdc_softc *);
void	wdc_init_shadow_regs(struct ata_channel *);

int	wdcprobe(struct ata_channel *);
void	wdcattach(struct ata_channel *);
int	wdcdetach(struct device *, int);
int	wdcactivate(struct device *, enum devact);
int	wdcintr(void *);

void	wdcrestart(void*);

int	wdcreset(struct ata_channel *, int);
#define RESET_POLL 1 
#define RESET_SLEEP 0 /* wdcreset will use tsleep() */

int	wdcwait(struct ata_channel *, int, int, int, int);
#define WDCWAIT_OK	0  /* we have what we asked */
#define WDCWAIT_TOUT	-1 /* timed out */
#define WDCWAIT_THR	1  /* return, the kernel thread has been awakened */

void	wdc_datain_pio(struct ata_channel *, int, void *, size_t);
void	wdc_dataout_pio(struct ata_channel *, int, void *, size_t);
void	wdcbit_bucket(struct ata_channel *, int);

int	wdc_dmawait(struct ata_channel *, struct ata_xfer *, int);
void	wdccommand(struct ata_channel *, u_int8_t, u_int8_t, u_int16_t,
		   u_int8_t, u_int8_t, u_int8_t, u_int8_t);
void	wdccommandext(struct ata_channel *, u_int8_t, u_int8_t, u_int64_t,
		      u_int16_t);
void	wdccommandshort(struct ata_channel *, int, int);
void	wdctimeout(void *arg);
void	wdc_reset_drive(struct ata_drive_datas *, int);
void	wdc_reset_channel(struct ata_channel *, int);

int	wdc_exec_command(struct ata_drive_datas *, struct ata_command*);

/*	
 * ST506 spec says that if READY or SEEKCMPLT go off, then the read or write
 * command is aborted.
 */   
#define wdc_wait_for_drq(chp, timeout, flags) \
		wdcwait((chp), WDCS_DRQ, WDCS_DRQ, (timeout), (flags))
#define wdc_wait_for_unbusy(chp, timeout, flags) \
		wdcwait((chp), 0, 0, (timeout), (flags))
#define wdc_wait_for_ready(chp, timeout, flags) \
		wdcwait((chp), WDCS_DRDY, WDCS_DRDY, (timeout), (flags))

/* ATA/ATAPI specs says a device can take 31s to reset */
#define WDC_RESET_WAIT 31000

void	wdc_atapibus_attach(struct atabus_softc *);

/* XXX */
struct atabus_softc;
void	atabusconfig(struct atabus_softc *);

#endif /* _DEV_IC_WDCVAR_H_ */
