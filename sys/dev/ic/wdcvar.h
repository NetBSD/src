/*	$NetBSD: wdcvar.h,v 1.2.2.3 1998/06/05 10:09:14 bouyer Exp $    */

/*
 * Copyright (c) 1994, 1995 Charles M. Hannum.  All rights reserved.
 *
 * DMA and multi-sector PIO handling are derived from code contributed by
 * Onno van der Linden.
 *
 * Atapi support added by Manuel Bouyer.
 *
 * bus_space-ified by Christopher G. Demetriou.
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
 *	must display the following acknowledgement:
 *  This product includes software developed by Charles M. Hannum.
 * 4. The name of the author may not be used to endorse or promote products
 *	derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#define	WAITTIME    (10 * hz)    /* time to wait for a completion */
	/* this is a lot for hard drives, but not for cdroms */

struct channel_queue {  /* per channel queue (may be shared) */
	TAILQ_HEAD(xferhead, wdc_xfer) sc_xfer;
};

struct channel_softc { /* Per channel data */
	/* Our location */
	int channel;
	/* Our controller's softc */
	struct wdc_softc *wdc;
	/* Our registers */
	bus_space_tag_t       cmd_iot;
	bus_space_handle_t    cmd_ioh;
	bus_space_tag_t       ctl_iot;
	bus_space_handle_t    ctl_ioh;
	/*
	 * XXX data access (normal and 32-bit) may need to be
	 * done via a separate iot/ioh on some systems.  Let's
	 * wait and see if that's the case before implementing
	 * it.
	 */
	/* Our state */
	int ch_flags;
#define WDCF_ACTIVE   0x01	/* channel is active */
#define WDCF_IRQ_WAIT 0x10	/* controller is waiting for irq */
	u_int8_t ch_status;         /* copy of status register */
	u_int8_t ch_error;          /* copy of error register */
	/* per-drive infos */
	struct ata_drive_datas ch_drive[2];

	/*
	 * channel queues. May be the same for all channels, if hw channels
	 * are not independants
	 */
	struct channel_queue *ch_queue;
};

struct wdc_softc { /* Per controller state */
	struct device sc_dev;
	/* manadatory fields */
	int           cap;
/* Capabilities supported by the controller */
#define	WDC_CAPABILITY_DATA32 0x01     /* 32-bit data access */
#define WDC_CAPABILITY_PIO    0x02	/* controller knows its PIO modes */
#define	WDC_CAPABILITY_DMA    0x04	/* DMA */
#define	WDC_CAPABILITY_UDMA   0x08	/* Ultra-DMA/33 */
#define	WDC_CAPABILITY_HWLOCK 0x10	/* Needs to lock HW */
	u_int8_t      pio_mode; /* hightest PIO mode supported */
	u_int8_t      dma_mode; /* hightest DMA mode supported */
	int nchannels;	/* Number of channels on this controller */
	struct channel_softc *channels;  /* channels-specific datas (array) */

	/* if WDC_CAPABILITY_DMA set in 'cap' */
	void            *dma_arg;
	int            (*dma_init) __P((void *, int, int, void *, size_t,
	                int));
	void           (*dma_start) __P((void *, int, int, int));
	int            (*dma_finish) __P((void *, int, int, int));

	/* if WDC_CAPABILITY_HWLOCK set in 'cap' */
	int            (*claim_hw) __P((void *, int));
	void            (*free_hw) __P((void *));

};

 /*
  * Description of a command to be handled by a controller.
  * These commands are queued in a list.
  */
struct wdc_xfer {
	volatile u_int c_flags;    
#define C_INUSE  	0x0001 /* xfer struct is in use */
#define C_ATAPI  	0x0002 /* xfer is ATAPI request */
#define C_TIMEOU  	0x0004 /* xfer processing timed out */
#define C_NEEDDONE  	0x0010 /* need to call upper-level done */

	/* Information about our location */
	u_int8_t drive;
	u_int8_t channel;

	/* Information about the current transfer  */
	void *cmd; /* wdc, ata or scsipi command structure */
	void *databuf;
	int c_bcount;      /* byte count left */
	int c_skip;        /* bytes already transferred */
	TAILQ_ENTRY(wdc_xfer) c_xferchain;
	LIST_ENTRY(wdc_xfer) free_list;
	void (*c_start) __P((struct channel_softc *, struct wdc_xfer *));
	int  (*c_intr)  __P((struct channel_softc *, struct wdc_xfer *));
};

/*
 * Public functions which can be called by ATA or ATAPI specific parts,
 * or bus-specific backends.
 */

int   wdcprobe __P((const struct channel_softc *));
void  wdcattach __P((struct channel_softc *));
int   wdcintr __P((void *));
void  wdc_exec_xfer __P((struct channel_softc *, struct wdc_xfer *));
struct wdc_xfer *wdc_get_xfer __P((int)); /* int = WDC_NOSLEEP/CANSLEEP */
#define WDC_CANSLEEP 0x00
#define WDC_NOSLEEP 0x01
void   wdc_free_xfer  __P((struct channel_softc *, struct wdc_xfer *));
void  wdcstart __P((struct wdc_softc *, int));
void  wdcrestart __P((void*));
int   wdcreset	__P((struct channel_softc *, int));
#define VERBOSE 1 
#define SILENT 0 /* wdcreset will not print errors */
int   wdcwait __P((struct channel_softc *, int));
void  wdcbit_bucket __P(( struct channel_softc *, int));
void  wdccommand __P((struct channel_softc *, u_int8_t, u_int8_t, u_int16_t,
	                  u_int8_t, u_int8_t, u_int8_t, u_int8_t));
void   wdccommandshort __P((struct channel_softc *, int, int));
void  wdctimeout	__P((void *arg));

/*	
 * ST506 spec says that if READY or SEEKCMPLT go off, then the read or write
 * command is aborted.
 */   
#define wait_for_drq(chp)	wdcwait(chp, WDCS_DRDY | WDCS_DSC | WDCS_DRQ)
#define wait_for_unbusy(chp)	wdcwait(chp, 0)
#define wait_for_ready(chp) 	wdcwait(chp, WDCS_DRDY | WDCS_DSC)

void wdc_atapibus_attach __P((struct channel_softc *));
void wdc_ata_attach __P((struct channel_softc *));
