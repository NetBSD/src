/*	$NetBSD: wdvar.h,v 1.22 2003/12/14 05:03:28 thorpej Exp $	*/

/*
 * Copyright (c) 1998, 2001 Manuel Bouyer.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Manuel Bouyer.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
 *
 */

/*
 * ata_bustype. The first field has to be compatible with scsipi_bustype,
 * as it's used for autoconfig by both ata and atapi drivers
 */
  
struct ata_bustype {
	int bustype_type;	/* symbolic name of type */
	int (*ata_bio) __P((struct ata_drive_datas*, struct ata_bio *));
	void (*ata_reset_channel) __P((struct ata_drive_datas *, int));
	int (*ata_exec_command) __P((struct ata_drive_datas *,
					struct wdc_command *));
#define WDC_COMPLETE 0x01
#define WDC_QUEUED   0x02
#define WDC_TRY_AGAIN 0x03
	int (*ata_get_params) __P((struct ata_drive_datas*, u_int8_t,
					struct ataparams *));
	int (*ata_addref) __P((struct ata_drive_datas *));
	void (*ata_delref) __P((struct ata_drive_datas *));
	void (*ata_killpending) __P((struct ata_drive_datas *));
};
/* bustype_type */
/* #define SCSIPI_BUSTYPE_SCSI	0 */
/* #define SCSIPI_BUSTYPE_ATAPI	1 */
#define SCSIPI_BUSTYPE_ATA	2

/*
 * describe an ATA device. Has to be compatible with scsipi_channel, so start
 * with a pointer to ata_bustype
 */
struct ata_device {
	const struct ata_bustype *adev_bustype;
	int adev_channel;
	int adev_openings;
	struct ata_drive_datas *adev_drv_data;
};

#ifdef __ATA_DISK_PRIVATE

struct wd_softc {
	/* General disk infos */
	struct device sc_dev;
	struct disk sc_dk;
	struct lock sc_lock;
	struct bufq_state sc_q;
	struct callout sc_restart_ch;
	int sc_quirks;			/* any quirks drive might have */
	/* IDE disk soft states */
	struct ata_bio sc_wdc_bio; /* current transfer */
	struct buf *sc_bp; /* buf being transfered */
	struct ata_drive_datas *drvp; /* Our controller's infos */
	const struct ata_bustype *atabus;
	int openings;
	struct ataparams sc_params;/* drive characteristics found */
	int sc_flags;	  
#define	WDF_WLABEL	0x004 /* label is writable */
#define	WDF_LABELLING	0x008 /* writing label */
/*
 * XXX Nothing resets this yet, but disk change sensing will when ATA-4 is
 * more fully implemented.
 */
#define WDF_LOADED	0x010 /* parameters loaded */
#define WDF_WAIT	0x020 /* waiting for resources */
#define WDF_LBA		0x040 /* using LBA mode */
#define WDF_KLABEL	0x080 /* retain label after 'full' close */
#define WDF_LBA48	0x100 /* using 48-bit LBA mode */
	u_int64_t sc_capacity;
	int cyl; /* actual drive parameters */
	int heads;
	int sectors;
	int retries; /* number of xfer retry */

	void *sc_sdhook;		/* our shutdown hook */

	SLIST_HEAD(, disk_badsectors)	sc_bslist;
	u_int sc_bscount;

#if NRND > 0
	rndsource_element_t	rnd_source;
#endif
};

#define sc_drive sc_wdc_bio.drive
#define sc_mode sc_wdc_bio.mode
#define sc_multi sc_wdc_bio.multi
#define sc_badsect sc_wdc_bio.badsect

#endif /* __ATA_DISK_PRIVATE */

void wddone __P((void *));
