/*	$NetBSD: wdvar.h,v 1.7.2.3 2002/06/23 17:45:55 jdolecek Exp $	*/

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
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
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

/* Params needed by the controller to perform an ATA bio */
struct ata_bio {
	volatile u_int16_t flags;/* cmd flags */
#define	ATA_NOSLEEP	0x0001	/* Can't sleep */   
#define	ATA_POLL	0x0002	/* poll for completion */
#define	ATA_ITSDONE	0x0004	/* the transfer is as done as it gets */
#define	ATA_SINGLE	0x0008	/* transfer must be done in singlesector mode */
#define	ATA_LBA		0x0010	/* transfer uses LBA adressing */
#define	ATA_READ	0x0020	/* transfer is a read (otherwise a write) */
#define	ATA_CORR	0x0040	/* transfer had a corrected error */
#define	ATA_LBA48	0x0080	/* transfer uses 48-bit LBA adressing */
	int		multi;	/* # of blocks to transfer in multi-mode */
	struct disklabel *lp;	/* pointer to drive's label info */
	daddr_t		blkno;	/* block addr */
	daddr_t		blkdone;/* number of blks transferred */
	daddr_t		nblks;	/* number of block currently transferring */
	int		nbytes;	/* number of bytes currently transferring */
	long		bcount;	/* total number of bytes */
	char		*databuf;/* data buffer adress */
	volatile int	error;
#define	NOERROR 	0	/* There was no error (r_error invalid) */
#define	ERROR		1	/* check r_error */
#define	ERR_DF		2	/* Drive fault */
#define	ERR_DMA		3	/* DMA error */
#define	TIMEOUT		4	/* device timed out */
#define	ERR_NODEV	5	/* device has been gone */
	u_int8_t	r_error;/* copy of error register */
	daddr_t		badsect[127];/* 126 plus trailing -1 marker */
};

/*
 * ata_bustype. The first field has to be compatible with scsipi_bustype,
 * as it's used for autoconfig by both ata and atapi drivers
 */
  
struct ata_bustype {
	int bustype_type;	/* symbolic name of type */
	int (*ata_bio) __P((struct ata_drive_datas*, struct ata_bio *));
	void (*ata_reset_channel) __P((struct ata_drive_datas *));
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

void wddone __P((void *));
