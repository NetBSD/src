/*	$NetBSD: atavar.h,v 1.40.2.1 2004/04/16 07:56:34 tron Exp $	*/

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
 */

#ifndef _DEV_ATA_ATAVAR_H_
#define	_DEV_ATA_ATAVAR_H_

#include <sys/lock.h>
#include <sys/queue.h>

/*
 * Description of a command to be handled by an ATA controller.  These
 * commands are queued in a list.
 */
struct ata_xfer {
	__volatile u_int c_flags;	/* command state flags */
	
	/* Channel and drive that are to process the request. */
	struct wdc_channel *c_chp;
	int	c_drive;

	void	*c_cmd;			/* private request structure pointer */
	void	*c_databuf;		/* pointer to data buffer */
	int	c_bcount;		/* byte count left */
	int	c_skip;			/* bytes already transferred */
	int	c_dscpoll;		/* counter for dsc polling (ATAPI) */

	/* Link on the command queue. */
	TAILQ_ENTRY(ata_xfer) c_xferchain;

	/* Low-level protocol handlers. */
	void	(*c_start)(struct wdc_channel *, struct ata_xfer *);
	int	(*c_intr)(struct wdc_channel *, struct ata_xfer *, int);
	void	(*c_kill_xfer)(struct wdc_channel *, struct ata_xfer *);
};

#define	C_ATAPI		0x0001		/* xfer is ATAPI request */
#define	C_TIMEOU	0x0002		/* xfer processing timed out */
#define	C_POLL		0x0004		/* command is polled */
#define	C_DMA		0x0008		/* command uses DMA */

/* Per-channel queue of ata_xfers.  May be shared by multiple channels. */
struct ata_queue {
	TAILQ_HEAD(, ata_xfer) queue_xfer;
	int queue_freeze;
};

/* ATA bus instance state information. */
struct atabus_softc {
	struct device sc_dev;
	struct wdc_channel *sc_chan;	/* XXXwdc */
};

/*
 * A queue of atabus instances, used to ensure the same bus probe order
 * for a given hardware configuration at each boot.
 */
struct atabus_initq {
	TAILQ_ENTRY(atabus_initq) atabus_initq;
	struct atabus_softc *atabus_sc;
};

#ifdef _KERNEL
TAILQ_HEAD(atabus_initq_head, atabus_initq);
extern struct atabus_initq_head atabus_initq_head;
extern struct simplelock atabus_interlock;
#endif /* _KERNEL */

/* High-level functions and structures used by both ATA and ATAPI devices */
struct ataparams;

/* Datas common to drives and controller drivers */
struct ata_drive_datas {
	u_int8_t drive;		/* drive number */
	int8_t ata_vers;	/* ATA version supported */
	u_int16_t drive_flags;	/* bitmask for drives present/absent and cap */

#define	DRIVE_ATA	0x0001
#define	DRIVE_ATAPI	0x0002
#define	DRIVE_OLD	0x0004 
#define	DRIVE		(DRIVE_ATA|DRIVE_ATAPI|DRIVE_OLD)
#define	DRIVE_CAP32	0x0008
#define	DRIVE_DMA	0x0010 
#define	DRIVE_UDMA	0x0020
#define	DRIVE_MODE	0x0040	/* the drive reported its mode */
#define	DRIVE_RESET	0x0080	/* reset the drive state at next xfer */
#define	DRIVE_DMAERR	0x0100	/* Udma transfer had crc error, don't try DMA */
#define	DRIVE_ATAPIST	0x0200	/* device is an ATAPI tape drive */

	/*
	 * Current setting of drive's PIO, DMA and UDMA modes.
	 * Is initialised by the disks drivers at attach time, and may be
	 * changed later by the controller's code if needed
	 */
	u_int8_t PIO_mode;	/* Current setting of drive's PIO mode */
	u_int8_t DMA_mode;	/* Current setting of drive's DMA mode */
	u_int8_t UDMA_mode;	/* Current setting of drive's UDMA mode */

	/* Supported modes for this drive */
	u_int8_t PIO_cap;	/* supported drive's PIO mode */
	u_int8_t DMA_cap;	/* supported drive's DMA mode */
	u_int8_t UDMA_cap;	/* supported drive's UDMA mode */

	/*
	 * Drive state.
	 * This is reset to 0 after a channel reset.
	 */
	u_int8_t state;

#define RESET          0
#define READY          1

	/* numbers of xfers and DMA errs. Used by ata_dmaerr() */
	u_int8_t n_dmaerrs;
	u_int32_t n_xfers;

	/* Downgrade after NERRS_MAX errors in at most NXFER xfers */
#define NERRS_MAX 4
#define NXFER 4000

	/* Callbacks into the drive's driver. */
	void	(*drv_done)(void *);	/* transfer is done */

	struct device *drv_softc;	/* ATA drives softc, if any */
	void *chnl_softc;		/* channel softc */
};

/* User config flags that force (or disable) the use of a mode */
#define ATA_CONFIG_PIO_MODES	0x0007
#define ATA_CONFIG_PIO_SET	0x0008
#define ATA_CONFIG_PIO_OFF	0
#define ATA_CONFIG_DMA_MODES	0x0070
#define ATA_CONFIG_DMA_SET	0x0080
#define ATA_CONFIG_DMA_DISABLE	0x0070
#define ATA_CONFIG_DMA_OFF	4
#define ATA_CONFIG_UDMA_MODES	0x0700
#define ATA_CONFIG_UDMA_SET	0x0800
#define ATA_CONFIG_UDMA_DISABLE	0x0700
#define ATA_CONFIG_UDMA_OFF	8

/*
 * Parameters/state needed by the controller to perform an ATA bio.
 */
struct ata_bio {
	volatile u_int16_t flags;/* cmd flags */
#define	ATA_NOSLEEP	0x0001	/* Can't sleep */   
#define	ATA_POLL	0x0002	/* poll for completion */
#define	ATA_ITSDONE	0x0004	/* the transfer is as done as it gets */
#define	ATA_SINGLE	0x0008	/* transfer must be done in singlesector mode */
#define	ATA_LBA		0x0010	/* transfer uses LBA addressing */
#define	ATA_READ	0x0020	/* transfer is a read (otherwise a write) */
#define	ATA_CORR	0x0040	/* transfer had a corrected error */
#define	ATA_LBA48	0x0080	/* transfer uses 48-bit LBA addressing */
	int		multi;	/* # of blocks to transfer in multi-mode */
	struct disklabel *lp;	/* pointer to drive's label info */
	daddr_t		blkno;	/* block addr */
	daddr_t		blkdone;/* number of blks transferred */
	daddr_t		nblks;	/* number of block currently transferring */
	int		nbytes;	/* number of bytes currently transferring */
	long		bcount;	/* total number of bytes */
	char		*databuf;/* data buffer address */
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
 * ATA/ATAPI commands description 
 *
 * This structure defines the interface between the ATA/ATAPI device driver
 * and the controller for short commands. It contains the command's parameter,
 * the len of data's to read/write (if any), and a function to call upon
 * completion.
 * If no sleep is allowed, the driver can poll for command completion.
 * Once the command completed, if the error registed is valid, the flag
 * AT_ERROR is set and the error register value is copied to r_error .
 * A separate interface is needed for read/write or ATAPI packet commands
 * (which need multiple interrupts per commands).
 */
struct wdc_command {
	u_int8_t r_command;	/* Parameters to upload to registers */
	u_int8_t r_head;
	u_int16_t r_cyl;
	u_int8_t r_sector;
	u_int8_t r_count;
	u_int8_t r_precomp;
	u_int8_t r_st_bmask;	/* status register mask to wait for before
				   command */
	u_int8_t r_st_pmask;	/* status register mask to wait for after
				   command */
	u_int8_t r_error;	/* error register after command done */
	volatile u_int16_t flags;

#define AT_READ     0x0001 /* There is data to read */
#define AT_WRITE    0x0002 /* There is data to write (excl. with AT_READ) */
#define AT_WAIT     0x0008 /* wait in controller code for command completion */
#define AT_POLL     0x0010 /* poll for command completion (no interrupts) */
#define AT_DONE     0x0020 /* command is done */
#define AT_XFDONE   0x0040 /* data xfer is done */
#define AT_ERROR    0x0080 /* command is done with error */
#define AT_TIMEOU   0x0100 /* command timed out */
#define AT_DF       0x0200 /* Drive fault */
#define AT_READREG  0x0400 /* Read registers on completion */

	int timeout;		/* timeout (in ms) */
	void *data;		/* Data buffer address */
	int bcount;		/* number of bytes to transfer */
	void (*callback)(void *); /* command to call once command completed */
	void *callback_arg;	/* argument passed to *callback() */
};

/*
 * ata_bustype.  The first field must be compatible with scsipi_bustype,
 * as it's used for autoconfig by both ata and atapi drivers.
 */
struct ata_bustype {
	int	bustype_type;	/* symbolic name of type */
	int	(*ata_bio)(struct ata_drive_datas *, struct ata_bio *);
	void	(*ata_reset_channel)(struct ata_drive_datas *, int);
	int	(*ata_exec_command)(struct ata_drive_datas *,
				    struct wdc_command *);

#define	WDC_COMPLETE	0x01
#define	WDC_QUEUED	0x02
#define	WDC_TRY_AGAIN	0x03

	int	(*ata_get_params)(struct ata_drive_datas *, u_int8_t,
				  struct ataparams *);
	int	(*ata_addref)(struct ata_drive_datas *);
	void	(*ata_delref)(struct ata_drive_datas *);
	void	(*ata_killpending)(struct ata_drive_datas *);
};

/* bustype_type */	/* XXX XXX XXX */
/* #define SCSIPI_BUSTYPE_SCSI	0 */
/* #define SCSIPI_BUSTYPE_ATAPI	1 */
#define	SCSIPI_BUSTYPE_ATA	2

/*
 * Describe an ATA device.  Has to be compatible with scsipi_channel, so
 * start with a pointer to ata_bustype.
 */
struct ata_device {
	const struct ata_bustype *adev_bustype;
	int adev_channel;
	int adev_openings;
	struct ata_drive_datas *adev_drv_data;
};

#ifdef _KERNEL
int	atabusprint(void *aux, const char *);
int	ataprint(void *aux, const char *);

int	wdc_downgrade_mode(struct ata_drive_datas *, int);

struct ataparams;
int	ata_get_params(struct ata_drive_datas *, u_int8_t, struct ataparams *);
int	ata_set_mode(struct ata_drive_datas *, u_int8_t, u_int8_t);
/* return code for these cmds */
#define CMD_OK    0
#define CMD_ERR   1
#define CMD_AGAIN 2

void	ata_dmaerr(struct ata_drive_datas *, int);
#endif /* _KERNEL */

#endif /* _DEV_ATA_ATAVAR_H_ */
