/*	$NetBSD: wdcvar.h,v 1.56 2004/04/13 19:51:06 bouyer Exp $	*/

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

/* XXX For scsipi_adapter and scsipi_channel. */
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/atapiconf.h>

#include <sys/callout.h>

#define	WAITTIME    (10 * hz)    /* time to wait for a completion */
	/* this is a lot for hard drives, but not for cdroms */

#define WDC_NREG	8 /* number of command registers */

/*
 * Per-channel data
 */
struct wdc_channel {
	struct callout ch_callout;	/* callout handle */
	int ch_channel;			/* location */
	struct wdc_softc *ch_wdc;	/* controller's softc */

	/* Our registers */
	bus_space_tag_t       cmd_iot;
	bus_space_handle_t    cmd_baseioh;
	bus_space_handle_t    cmd_iohs[WDC_NREG];
	bus_space_tag_t       ctl_iot;
	bus_space_handle_t    ctl_ioh;

	/* data32{iot,ioh} are only used for 32 bit data xfers */
	bus_space_tag_t         data32iot;
	bus_space_handle_t      data32ioh;

	/* Our state */
	volatile int ch_flags;
#define WDCF_ACTIVE   0x01	/* channel is active */
#define WDCF_SHUTDOWN 0x02	/* channel is shutting down */
#define WDCF_IRQ_WAIT 0x10	/* controller is waiting for irq */
#define WDCF_DMA_WAIT 0x20	/* controller is waiting for DMA */
#define	WDCF_DISABLED 0x80	/* channel is disabled */
#define WDCF_TH_RUN   0x100	/* the kenrel thread is working */
#define WDCF_TH_RESET 0x200	/* someone ask the thread to reset */
	u_int8_t ch_status;	/* copy of status register */
	u_int8_t ch_error;	/* copy of error register */

	/* per-drive info */
	struct ata_drive_datas ch_drive[2];

	struct device *atabus;	/* self */

	/* ATAPI children */
	struct device *atapibus;
	struct scsipi_channel ch_atapi_channel;

	/* ATA children */
	struct device *ata_drives[2];

	/*
	 * Channel queues.  May be the same for all channels, if hw
	 * channels are not independent.
	 */
	struct ata_queue *ch_queue;

	/* The channel kernel thread */
	struct proc *ch_thread;
};

/*
 * Per-controller data
 */
struct wdc_softc {
	struct device sc_dev;		/* generic device info */

	int           cap;		/* controller capabilities */
#define	WDC_CAPABILITY_DATA16 0x0001    /* can do  16-bit data access */
#define	WDC_CAPABILITY_DATA32 0x0002    /* can do 32-bit data access */
#define WDC_CAPABILITY_MODE   0x0004	/* controller knows its PIO/DMA modes */
#define	WDC_CAPABILITY_DMA    0x0008	/* DMA */
#define	WDC_CAPABILITY_UDMA   0x0010	/* Ultra-DMA/33 */
#define	WDC_CAPABILITY_HWLOCK 0x0020	/* Needs to lock HW */
#define	WDC_CAPABILITY_ATA_NOSTREAM 0x0040 /* Don't use stream funcs on ATA */
#define	WDC_CAPABILITY_ATAPI_NOSTREAM 0x0080 /* Don't use stream f on ATAPI */
#define WDC_CAPABILITY_NO_EXTRA_RESETS 0x0100 /* only reset once */
#define WDC_CAPABILITY_PREATA 0x0200	/* ctrl can be a pre-ata one */
#define WDC_CAPABILITY_IRQACK 0x0400	/* callback to ack interrupt */
#define WDC_CAPABILITY_NOIRQ  0x1000	/* Controller never interrupts */
#define WDC_CAPABILITY_SELECT  0x2000	/* Controller selects target */
#define	WDC_CAPABILITY_RAID   0x4000	/* Controller "supports" RAID */
	u_int8_t      PIO_cap;		/* highest PIO mode supported */
	u_int8_t      DMA_cap;		/* highest DMA mode supported */
	u_int8_t      UDMA_cap;		/* highest UDMA mode supported */
	int nchannels;			/* # channels on this controller */
	struct wdc_channel **channels;  /* channel-specific data (array) */

	/*
	 * The reference count here is used for both IDE and ATAPI devices.
	 */
	struct atapi_adapter sc_atapi_adapter;

	/* Function used to probe for drives. */
	void		(*drv_probe)(struct wdc_channel *);

	/* if WDC_CAPABILITY_DMA set in 'cap' */
	void            *dma_arg;
	int            (*dma_init)(void *, int, int, void *, size_t, int);
	void           (*dma_start)(void *, int, int);
	int            (*dma_finish)(void *, int, int, int);
/* flags passed to dma_init */
#define WDC_DMA_READ	0x01
#define WDC_DMA_IRQW	0x02
#define WDC_DMA_LBA48	0x04

	int		dma_status; /* status returned from dma_finish() */
#define WDC_DMAST_NOIRQ	0x01	/* missing IRQ */
#define WDC_DMAST_ERR	0x02	/* DMA error */
#define WDC_DMAST_UNDER	0x04	/* DMA underrun */

	/* if WDC_CAPABILITY_HWLOCK set in 'cap' */
	int            (*claim_hw)(void *, int);
	void            (*free_hw)(void *);

	/* if WDC_CAPABILITY_MODE set in 'cap' */
	void 		(*set_modes)(struct wdc_channel *);

	/* if WDC_CAPABILITY_SELECT set in 'cap' */
	void		(*select)(struct wdc_channel *,int);

	/* if WDC_CAPABILITY_IRQACK set in 'cap' */
	void		(*irqack)(struct wdc_channel *);
};

/*
 * Public functions which can be called by ATA or ATAPI specific parts,
 * or bus-specific backends.
 */

int	wdcprobe(struct wdc_channel *);
void	wdcattach(struct wdc_channel *);
int	wdcdetach(struct device *, int);
int	wdcactivate(struct device *, enum devact);
int	wdcintr(void *);
void	wdc_exec_xfer(struct wdc_channel *, struct ata_xfer *);

struct ata_xfer *wdc_get_xfer(int); /* int = WDC_NOSLEEP/CANSLEEP */
#define WDC_CANSLEEP 0x00
#define WDC_NOSLEEP 0x01

void	wdc_free_xfer (struct wdc_channel *, struct ata_xfer *);
void	wdcstart(struct wdc_channel *);
void	wdcrestart(void*);

int	wdcreset(struct wdc_channel *, int);
#define RESET_POLL 1 
#define RESET_SLEEP 0 /* wdcreset will use tsleep() */

int	wdcwait(struct wdc_channel *, int, int, int, int);
#define WDCWAIT_OK	0  /* we have what we asked */
#define WDCWAIT_TOUT	-1 /* timed out */
#define WDCWAIT_THR	1  /* return, the kernel thread has been awakened */

int	wdc_dmawait(struct wdc_channel *, struct ata_xfer *, int);
void	wdcbit_bucket( struct wdc_channel *, int);
void	wdccommand(struct wdc_channel *, u_int8_t, u_int8_t, u_int16_t,
		   u_int8_t, u_int8_t, u_int8_t, u_int8_t);
void	wdccommandext(struct wdc_channel *, u_int8_t, u_int8_t, u_int64_t,
		      u_int16_t);
void	wdccommandshort(struct wdc_channel *, int, int);
void	wdctimeout(void *arg);
void	wdc_reset_channel(struct ata_drive_datas *, int);

int	wdc_exec_command(struct ata_drive_datas *, struct wdc_command*);
#define WDC_COMPLETE 0x01
#define WDC_QUEUED   0x02
#define WDC_TRY_AGAIN 0x03

int	wdc_addref(struct wdc_channel *);
void	wdc_delref(struct wdc_channel *);
void	wdc_kill_pending(struct wdc_channel *);

void	wdc_print_modes (struct wdc_channel *);
void	wdc_probe_caps(struct ata_drive_datas*);

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
