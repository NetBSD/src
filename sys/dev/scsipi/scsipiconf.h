/*	$NetBSD: scsipiconf.h,v 1.1.2.3 1997/07/17 15:00:06 bouyer Exp $	*/

/*
 * Copyright (c) 1993, 1994, 1995 Charles Hannum.  All rights reserved.
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
 *	This product includes software developed by Charles Hannum.
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

/*
 * Originally written by Julian Elischer (julian@tfs.com)
 * for TRW Financial Systems for use under the MACH(2.5) operating system.
 *
 * TRW Financial Systems, in accordance with their agreement with Carnegie
 * Mellon University, makes this software available to CMU to distribute
 * or use in any manner that they see fit as long as this message is kept with
 * the software. For this reason TFS also grants any other persons or
 * organisations permission to use or modify this software.
 *
 * TFS supplies this software to be publicly redistributed
 * on the understanding that TFS is not responsible for the correct
 * functioning of this software in any circumstances.
 *
 * Ported to run under 386BSD by Julian Elischer (julian@tfs.com) Sept 1992
 */

#ifndef	SCSI_BUSCONF_H
#define SCSI_BUSCONF_H 1

typedef	int	boolean;

#include <sys/queue.h>
#include <machine/cpu.h>
#include <dev/scsipi/scsipi_debug.h>

/*
 * The following documentation tries to describe the relationship between the
 * various structures defined in this file:
 *
 * each adapter type has a scsipi_adapter struct. This describes the adapter and
 *    identifies routines that can be called to use the adapter.
 * each device type has a scsipi_device struct. This describes the device and
 *    identifies routines that can be called to use the device.
 * each existing device position (scsibus + target + lun or atapibus + drive) 
 *    can be described by a scsipi_link struct.
 *    Only scsipi positions that actually have devices, have a scsipi_link
 *    structure assigned. so in effect each device has scsipi_link struct.
 *    The scsipi_link structure contains information identifying both the
 *    device driver and the adapter driver for that position on that scsipi bus,
 *    and can be said to 'link' the two.
 * each individual scsipi bus has an array that points to all the scsipi_link
 *    structs associated with that scsipi bus. Slots with no device have
 *    a NULL pointer.
 * each individual device also knows the address of it's own scsipi_link
 *    structure.
 *
 *				-------------
 *
 * The key to all this is the scsipi_link structure which associates all the
 * other structures with each other in the correct configuration.  The
 * scsipi_link is the connecting information that allows each part of the
 * scsipi system to find the associated other parts.
 */

struct buf;
struct scsipi_xfer;

struct scsipi_generic {
	u_int8_t opcode;
	u_int8_t bytes[15];
};


/*
 * return values for scsipi_cmd()
 */
#define SUCCESSFULLY_QUEUED	0
#define TRY_AGAIN_LATER		1
#define	COMPLETE		2
#define	ESCAPE_NOT_SUPPORTED	3

/*
 * These entry points are called by the low-end drivers to get services from
 * whatever high-end drivers they are attached to.  Each device type has one
 * of these statically allocated.
 */
struct scsipi_device {
	int	(*err_handler) __P((struct scsipi_xfer *));
			/* returns -1 to say err processing done */
	void	(*start) __P((void *));

	int	(*async) __P((void));
	/*
	 * When called with `0' as the second argument, we expect status
	 * back from the upper-level driver.  When called with a `1',
	 * we're simply notifying the upper-level driver that the command
	 * is complete and expect no status back.
	 */
	void	(*done)  __P((struct scsipi_xfer *));
};

/* 
 * These entrypoints are called by the high-end drivers to get services from
 * whatever low-end drivers they are attached to each adapter type has one of 
 * these statically allocated. 
 */
struct scsipi_adapter {
	int	(*scsipi_cmd) __P((struct scsipi_xfer *)); 
	void	(*scsipi_minphys) __P((struct buf *));
	int	(*open_target_lu) __P((void));
	int	(*close_target_lu) __P((void)); 
};

/*
 * This structure describes the connection between an adapter driver and
 * a device driver, and is used by each to call services provided by
 * the other, and to allow generic scsipi glue code to call these services
 * as well.
 *
 * XXX Given the way NetBSD's autoconfiguration works, this is ...
 * XXX nasty.
 */

struct scsipi_link {
	u_int8_t type;			/* device type, i.e. SCSI, ATAPI, ... */
#define BUS_SCSI 0
#define BUS_ATAPI 1
	u_int8_t openings;		/* available operations */
	u_int8_t active;		/* operations in progress */
	u_int8_t flags;			/* flags that all devices have */
#define	SDEV_REMOVABLE	 	0x01	/* media is removable */
#define	SDEV_MEDIA_LOADED 	0x02	/* device figures are still valid */
#define	SDEV_WAITING	 	0x04	/* a process is waiting for this */
#define	SDEV_OPEN	 	0x08	/* at least 1 open session */
#define	SDEV_DBX		0xf0	/* debuging flags (scsipi_debug.h) */
	u_int16_t quirks;		/* per-device oddities */
#define	SDEV_AUTOSAVE		0x0001	/* do implicit SAVEDATAPOINTER on
					   disconnect */
#define	SDEV_NOSYNCWIDE		0x0002	/* does not grok SDTR or WDTR */
#define	SDEV_NOLUNS		0x0004	/* does not grok LUNs */
#define	SDEV_FORCELUNS		0x0008	/* prehistoric drive/ctlr groks LUNs */
#define SDEV_NOMODESENSE	0x0010	/* removable media/optical drives */
#define SDEV_NOSTARTUNIT	0x0020	/* do not issue start unit requests
					   in sd.c */
#define ADEV_CDROM		0x0100	/* device is a CD-ROM */  
#define ADEV_LITTLETOC		0x0200	/* Audio TOC uses wrong byte order */  
#define ADEV_NOCAPACITY		0x0400	/* no READ_CD_CAPACITY command */
#define ADEV_NOTUR		0x0800	/* no TEST_UNIT_READY command */
#define ADEV_NODOORLOCK		0x1000	/* device can't look door */


	struct	scsipi_device *device;	/* device entry points etc. */
	void	*device_softc;		/* needed for call to foo_start */
	struct scsipi_adapter *adapter;	/* adapter entry points etc. */
	void    *adapter_softc;		/* needed for call to foo_scsipi_cmd */
	union {				/* needed for call to foo_scsipi_cmd */
		struct scsi_link {
			int  channel;	/* channel, i.e. bus # on controller */ 
 
			u_int8_t scsi_version;	/* SCSI-I, SCSI-II, etc. */
			u_int8_t scsibus;	/* the Nth scsibus */
			u_int8_t target;	/* targ of this dev */
			u_int8_t lun;		/* lun of this dev */
			u_int8_t adapter_target;/* what are we on the scsi
						   bus */
			int max_target;		/* XXX max target supported
						   by adapter */
		} scsipi_scsi;
		struct atascsipi_link {		/* XXX this must be in sync
						   with wd_link */
			u_int8_t type;		/* 0 = ATA, 1 = ATAPI, from
						   wdlink.h */
			u_int8_t channel;	/* channel, i.e. bus # on
						   controller */
			u_int8_t drive; 	/* drive number on the bus */
			u_int8_t atapibus;
			u_int8_t cap;		/* drive capability */
#define ACAP_DRQ_MPROC      0x0000  /* microprocessor DRQ */
#define ACAP_DRQ_INTR       0x0100  /* interrupt DRQ */ 
#define ACAP_DRQ_ACCEL      0x0200  /* accelerated DRQ */
#define ACAP_LEN            0x0400  /* 16 bit commands */
		} scsipi_atapi;
	} _scsipi_link;
	int (*scsipi_cmd) __P((struct scsipi_link *, struct scsipi_generic *,
				int cmdlen, u_char *data_addr,
				int datalen, int retries,
				int timeout, struct buf *bp,
				int flags));
	int (*scsipi_interpret_sense) __P((struct scsipi_xfer *));
	void (*sc_print_addr) __P((struct scsipi_link *sc_link));
};
#define scsipi_scsi _scsipi_link.scsipi_scsi
#define scsipi_atapi _scsipi_link.scsipi_atapi

/*
 * Each scsipi transaction is fully described by one of these structures
 * It includes information about the source of the command and also the
 * device and adapter for which the command is destined.
 * (via the scsipi_link structure)
 */
struct scsipi_xfer {
	LIST_ENTRY(scsipi_xfer) free_list;
	int	flags;			/* 0x00ff0000 reserved for ATAPI */
	struct	scsipi_link *sc_link;	/* all about our device and adapter */
	int	retries;		/* the number of times to retry */
	int	timeout;		/* in milliseconds */
	struct	scsipi_generic *cmd;	/* The scsipi command to execute */
	int	cmdlen;			/* how long it is */
	u_char	*data;			/* dma address OR a uio address */
	int	datalen;		/* data len (blank if uio) */
	int	resid;			/* how much buffer was not touched */
	int	error;			/* an error value */
	struct	buf *bp;		/* If we need to associate with a buf */
	union {
		struct  scsipi_sense_data scsi_sense; /* 32 bytes */
		u_int32_t atapi_sense;
	} sense;
	/*
	 * Believe it or not, Some targets fall on the ground with
	 * anything but a certain sense length.
	 */
	int	req_sense_length;	/* Explicit request sense length */
	u_int8_t status;		/* SCSI status */
	struct	scsipi_generic cmdstore;/* stash the command in here */
};

/*
 * Per-request Flag values
 */
#define	SCSI_NOSLEEP	0x0001	/* don't sleep */
#define	SCSI_POLL	0x0002	/* poll for completion */
#define	SCSI_AUTOCONF	0x0003	/* shorthand for SCSI_POLL | SCSI_NOSLEEP */
#define	SCSI_USER	0x0004	/* Is a user cmd, call scsipi_user_done	*/
#define	ITSDONE		0x0008	/* the transfer is as done as it gets	*/
#define	INUSE		0x0010	/* The scsipi_xfer block is in use	*/
#define	SCSI_SILENT	0x0020	/* don't announce NOT READY or MEDIA CHANGE */
#define	SCSI_IGNORE_NOT_READY		0x0040	/* ignore NOT READY */
#define	SCSI_IGNORE_MEDIA_CHANGE	0x0080	/* ignore MEDIA CHANGE */
#define	SCSI_IGNORE_ILLEGAL_REQUEST	0x0100	/* ignore ILLEGAL REQUEST */
#define	SCSI_RESET	0x0200	/* Reset the device in question		*/
#define	SCSI_DATA_UIO	0x0400	/* The data address refers to a UIO	*/
#define	SCSI_DATA_IN	0x0800	/* expect data to come INTO memory	*/
#define	SCSI_DATA_OUT	0x1000	/* expect data to flow OUT of memory	*/
#define	SCSI_TARGET	0x2000	/* This defines a TARGET mode op.	*/
#define	SCSI_ESCAPE	0x4000	/* Escape operation			*/

/*
 * Error values an adapter driver may return
 */
#define XS_NOERROR	0	/* there is no error, (sense is invalid)  */
#define XS_SENSE	1	/* Check the returned sense for the error */
#define	XS_DRIVER_STUFFUP 2	/* Driver failed to perform operation	  */
#define XS_SELTIMEOUT	3	/* The device timed out.. turned off?	  */
#define XS_TIMEOUT	4	/* The Timeout reported was caught by SW  */
#define XS_BUSY		5	/* The device busy, try again later?	  */

/*
 * This describes matching information for scsipi_inqmatch().  The more things
 * match, the higher the configuration priority.
 */
struct scsipi_inquiry_pattern {
	u_int8_t type;
	boolean removable;
	char *vendor;
	char *product;
	char *revision;
};

/*
 * This is used to pass information from the high-level configuration code
 * to the device-specific drivers.
 */

struct scsipibus_attach_args {
	struct scsipi_link *sa_sc_link;
	struct scsipi_inquiry_pattern sa_inqbuf;
	union { /* bus-type specific infos */
		u_int8_t scsi_version; /* SCSI version */
	} scsipi_info;
};

/* 
 * this describes a quirk entry
 */

struct scsi_quirk_inquiry_pattern {
	struct scsipi_inquiry_pattern pattern;
	u_int16_t quirks;
};


caddr_t	scsipi_inqmatch __P((struct scsipi_inquiry_pattern *, caddr_t,
	    int, int, int *));
char	*scsipi_dtype __P((int));
void	scsipi_strvis __P((u_char *, u_char *, int));
int	scsipi_execute_xs __P((struct scsipi_xfer *));
u_long	scsipi_size __P((struct scsipi_link *, int));
int	scsipi_test_unit_ready __P((struct scsipi_link *, int));
int	scsipi_prevent __P((struct scsipi_link *, int, int));
int	scsipi_inquire __P((struct scsipi_link *,
	    struct scsipi_inquiry_data *, int));
int	scsipi_start __P((struct scsipi_link *, int, int));
void	scsipi_done __P((struct scsipi_xfer *));
void	scsipi_user_done __P((struct scsipi_xfer *));
int	scsipi_do_ioctl __P((struct scsipi_link *, dev_t, u_long, caddr_t,
	    int, struct proc *));

void	show_scsipi_xs __P((struct scsipi_xfer *));
void	show_scsipi_cmd __P((struct scsipi_xfer *));
void	show_mem __P((u_char *, int));

static __inline void _lto2b __P((u_int32_t val, u_int8_t *bytes))
	__attribute__ ((unused));
static __inline void _lto3b __P((u_int32_t val, u_int8_t *bytes))
	__attribute__ ((unused));
static __inline void _lto4b __P((u_int32_t val, u_int8_t *bytes))
	__attribute__ ((unused));
static __inline u_int32_t _2btol __P((u_int8_t *bytes))
	__attribute__ ((unused));
static __inline u_int32_t _3btol __P((u_int8_t *bytes))
	__attribute__ ((unused));
static __inline u_int32_t _4btol __P((u_int8_t *bytes))
	__attribute__ ((unused));

static __inline void _lto2l __P((u_int32_t val, u_int8_t *bytes))
	__attribute__ ((unused));
static __inline void _lto3l __P((u_int32_t val, u_int8_t *bytes))
	__attribute__ ((unused));
static __inline void _lto4l __P((u_int32_t val, u_int8_t *bytes))
	__attribute__ ((unused));
static __inline u_int32_t _2ltol __P((u_int8_t *bytes))
	__attribute__ ((unused));
static __inline u_int32_t _3ltol __P((u_int8_t *bytes))
	__attribute__ ((unused));
static __inline u_int32_t _4ltol __P((u_int8_t *bytes))
	__attribute__ ((unused));
static __inline void bswap __P((char *, int))
	__attribute__ ((unused));

static __inline void
_lto2b(val, bytes)
	u_int32_t val;
	u_int8_t *bytes;
{

	bytes[0] = (val >> 8) & 0xff;
	bytes[1] = val & 0xff;
}

static __inline void
_lto3b(val, bytes)
	u_int32_t val;
	u_int8_t *bytes;
{

	bytes[0] = (val >> 16) & 0xff;
	bytes[1] = (val >> 8) & 0xff;
	bytes[2] = val & 0xff;
}

static __inline void
_lto4b(val, bytes)
	u_int32_t val;
	u_int8_t *bytes;
{

	bytes[0] = (val >> 24) & 0xff;
	bytes[1] = (val >> 16) & 0xff;
	bytes[2] = (val >> 8) & 0xff;
	bytes[3] = val & 0xff;
}

static __inline u_int32_t
_2btol(bytes)
	u_int8_t *bytes;
{
	register u_int32_t rv;

	rv = (bytes[0] << 8) |
	     bytes[1];
	return (rv);
}

static __inline u_int32_t
_3btol(bytes)
	u_int8_t *bytes;
{
	register u_int32_t rv;

	rv = (bytes[0] << 16) |
	     (bytes[1] << 8) |
	     bytes[2];
	return (rv);
}

static __inline u_int32_t
_4btol(bytes)
	u_int8_t *bytes;
{
	register u_int32_t rv;

	rv = (bytes[0] << 24) |
	     (bytes[1] << 16) |
	     (bytes[2] << 8) |
	     bytes[3];
	return (rv);
}

static __inline void
_lto2l(val, bytes)
	u_int32_t val;
	u_int8_t *bytes;
{

	bytes[0] = val & 0xff;
	bytes[1] = (val >> 8) & 0xff;
}

static __inline void
_lto3l(val, bytes)
	u_int32_t val;
	u_int8_t *bytes;
{

	bytes[0] = val & 0xff;
	bytes[1] = (val >> 8) & 0xff;
	bytes[2] = (val >> 16) & 0xff;
}

static __inline void
_lto4l(val, bytes)
	u_int32_t val;
	u_int8_t *bytes;
{

	bytes[0] = val & 0xff;
	bytes[1] = (val >> 8) & 0xff;
	bytes[2] = (val >> 16) & 0xff;
	bytes[3] = (val >> 24) & 0xff;
}

static __inline u_int32_t
_2ltol(bytes)
	u_int8_t *bytes;
{
	register u_int32_t rv;

	rv = bytes[0] |
	     (bytes[1] << 8);
	return (rv);
}

static __inline u_int32_t
_3ltol(bytes)
	u_int8_t *bytes;
{
	register u_int32_t rv;

	rv = bytes[0] |
	     (bytes[1] << 8) |
	     (bytes[2] << 16);
	return (rv);
}

static __inline u_int32_t
_4ltol(bytes)
	u_int8_t *bytes;
{
	register u_int32_t rv;

	rv = bytes[0] |
	     (bytes[1] << 8) |
	     (bytes[2] << 16) |
	     (bytes[3] << 24);
	return (rv);
}

static __inline void
bswap (buf, len)
    char *buf;
	int len;
{
	u_int16_t *p = (u_int16_t *)(buf + len);

	while (--p >= (u_int16_t *)buf)
		*p = (*p & 0xff) << 8 | (*p >> 8 & 0xff);
}

#endif /* SCSI_BUSCONF_H */
