/*	$NetBSD: scsipiconf.h,v 1.57.2.1 2001/09/07 04:45:31 thorpej Exp $	*/

/*-
 * Copyright (c) 1998, 1999, 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum; by Jason R. Thorpe of the Numerical Aerospace
 * Simulation Facility, NASA Ames Research Center.
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

#ifndef _DEV_SCSIPI_SCSIPICONF_H_
#define _DEV_SCSIPI_SCSIPICONF_H_

typedef	int	boolean;

#include <sys/callout.h>
#include <sys/queue.h>
#include <dev/scsipi/scsipi_debug.h>

struct buf;
struct proc;
struct device;
struct scsipi_channel;
struct scsipi_periph;
struct scsipi_xfer;

/*
 * The following defines the scsipi_xfer queue.
 */
TAILQ_HEAD(scsipi_xfer_queue, scsipi_xfer);

struct scsipi_generic {
	u_int8_t opcode;
	u_int8_t bytes[15];
};


/*
 * scsipi_async_event_t:
 *
 *	Asynchronous events from the adapter to the mid-layer and
 *	peripherial.
 *
 *	Arguments:
 *
 *	ASYNC_EVENT_MAX_OPENINGS	scsipi_max_openings * -- max
 *					openings, device specified in
 *					parameters
 *
 *	ASYNC_EVENT_XFER_MODE		scsipi_xfer_mode * -- xfer mode
 *					parameters changed for I_T Nexus
 *	ASYNC_EVENT_RESET		NULL - channel has been reset
 */
typedef enum {
	ASYNC_EVENT_MAX_OPENINGS,	/* set max openings on periph */
	ASYNC_EVENT_XFER_MODE,		/* xfer mode update for I_T */
	ASYNC_EVENT_RESET		/* channel reset */
} scsipi_async_event_t;

/*
 * scsipi_max_openings:
 *
 *	Argument for an ASYNC_EVENT_MAX_OPENINGS event.
 */
struct scsipi_max_openings {
	int	mo_target;		/* openings are for this target... */
	int	mo_lun;			/* ...and this lun */
	int	mo_openings;		/* openings value */
};

/*
 * scsipi_xfer_mode:
 *
 *	Argument for an ASYNC_EVENT_XFER_MODE event.
 */
struct scsipi_xfer_mode {
	int	xm_target;		/* target, for I_T Nexus */
	int	xm_mode;		/* PERIPH_CAP* bits */
	int	xm_period;		/* sync period */
	int	xm_offset;		/* sync offset */
};


/*
 * scsipi_adapter_req_t:
 *
 *	Requests that can be made of an adapter.
 *
 *	Arguments:
 *
 *	ADAPTER_REQ_RUN_XFER		scsipi_xfer * -- the xfer which
 *					is to be run
 *
 *	ADAPTER_REQ_GROW_RESOURCES	no argument
 *
 *	ADAPTER_REQ_SET_XFER_MODE	scsipi_xfer_mode * -- set the xfer
 *					mode for the I_T Nexus according to
 *					this
 */
typedef enum {
	ADAPTER_REQ_RUN_XFER,		/* run a scsipi_xfer */
	ADAPTER_REQ_GROW_RESOURCES,	/* grow xfer execution resources */
	ADAPTER_REQ_SET_XFER_MODE	/* set xfer mode */
} scsipi_adapter_req_t;


/*
 * scsipi_periphsw:
 *
 *	Callbacks into periph driver from midlayer.
 *
 *	psw_error	Called by the bustype's interpret-sense routine
 *			to do periph-specific sense handling.
 *
 *	psw_start	Called by midlayer to restart a device once
 *			more command openings become available.
 *
 *	psw_async	Called by midlayer when an asynchronous event
 *			from the adapter occurs.
 *
 *	psw_done	Called by the midlayer when an xfer has completed.
 */
struct scsipi_periphsw {
	int	(*psw_error) __P((struct scsipi_xfer *));
	void	(*psw_start) __P((struct scsipi_periph *));
	int	(*psw_async) __P((struct scsipi_periph *,
		    scsipi_async_event_t, void *));
	void	(*psw_done) __P((struct scsipi_xfer *));
};

struct disk_parms;
struct scsipi_inquiry_pattern;

/*
 * scsipi_adapter:
 *
 *	This structure describes an instance of a SCSIPI adapter.
 *
 *	Note that `adapt_openings' is used by (the common case of) adapters
 *	which have per-adapter resources.  If an adapter's command resources
 *	are associated with a channel, the the 	`chan_openings' below will
 *	be used instead.
 *
 *	Note that all adapter entry points take a pointer to a channel,
 *	as an adapter may have more than one channel, and the channel
 *	structure contains the channel number.
 */
struct scsipi_adapter {
	struct device *adapt_dev;	/* pointer to adapter's device */
	int	adapt_nchannels;	/* numnber of adapter channels */
	int	adapt_refcnt;		/* adapter's reference count */
	int	adapt_openings;		/* total # of command openings */
	int	adapt_max_periph;	/* max openings per periph */
	int	adapt_flags;

	void	(*adapt_request) __P((struct scsipi_channel *,
		    scsipi_adapter_req_t, void *));
	void	(*adapt_minphys) __P((struct buf *));
	int	(*adapt_ioctl) __P((struct scsipi_channel *, u_long,
		    caddr_t, int, struct proc *));
	int	(*adapt_enable) __P((struct device *, int));
	int	(*adapt_getgeom) __P((struct scsipi_periph *,
			struct disk_parms *, u_long));
	int	(*adapt_accesschk) __P((struct scsipi_periph *,
			struct scsipi_inquiry_pattern *));
};

/* adapt_flags */
#define SCSIPI_ADAPT_POLL_ONLY	0x01 /* Adaptor can't do interrupts. */

#define	scsipi_adapter_minphys(chan, bp)				\
	(*(chan)->chan_adapter->adapt_minphys)((bp))

#define	scsipi_adapter_request(chan, req, arg)				\
	(*(chan)->chan_adapter->adapt_request)((chan), (req), (arg))

#define	scsipi_adapter_ioctl(chan, cmd, data, flag, p)			\
	(*(chan)->chan_adapter->adapt_ioctl)((chan), (cmd), (data), (flag), (p))

#define	scsipi_adapter_enable(chan, enable)				\
	(*(chan)->chan_adapt->adapt_enable)((chan), (enable))


/*
 * scsipi_bustype:
 *
 *	This structure describes a SCSIPI bus type.
 */
struct scsipi_bustype {
	int	bustype_type;		/* symbolic name of type */
	
	int	(*bustype_cmd) __P((struct scsipi_periph *,
		    struct scsipi_generic *, int, void *, size_t, int,
		    int, struct buf *, int));
	int	(*bustype_interpret_sense) __P((struct scsipi_xfer *));
	void	(*bustype_printaddr) __P((struct scsipi_periph *));
	void	(*bustype_kill_pending) __P((struct scsipi_periph *));
};

/* bustype_type */
#define	SCSIPI_BUSTYPE_SCSI	0
#define	SCSIPI_BUSTYPE_ATAPI	1


/*
 * scsipi_channel:
 *
 *	This structure describes a single channel of a SCSIPI adapter.
 *	An adapter may have one or more channels.  See the comment above
 *	regarding the resource counter.
 */
struct scsipi_channel {
	u_int8_t type; /* XXX will die, compat with ata_atapi_attach for umass */
#define BUS_SCSI                0
#define BUS_ATAPI               1
/*define BUS_ATA                2*/

	struct scsipi_adapter *chan_adapter; /* pointer to our adapter */

	const struct scsipi_bustype *chan_bustype; /* channel's bus type */

	/*
	 * Periphs for this channel.  2-dimensional array is dynamically
	 * allocated.
	 *
	 * XXX Consider a different data structure to save space.
	 */
	struct scsipi_periph ***chan_periphs;

	int	chan_channel;		/* channel number */
	int	chan_flags;		/* channel flags */
	int	chan_openings;		/* number of command openings */
	int	chan_max_periph;	/* max openings per periph */

	int	chan_ntargets;		/* number of targets */
	int	chan_nluns;		/* number of luns */
	int	chan_id;		/* adapter's ID for this channel */

	int	chan_defquirks;		/* default device's quirks */

	struct proc *chan_thread;	/* completion thread */

	int	chan_qfreeze;		/* freeze count for queue */

	/* Job queue for this channel. */
	struct scsipi_xfer_queue chan_queue;

	/* Completed (async) jobs. */
	struct scsipi_xfer_queue chan_complete;

	/* callback we may have to call from completion thread */
	void (*chan_callback) __P((struct scsipi_channel *, void *));
	void *chan_callback_arg;
};

/* chan_flags */
#define	SCSIPI_CHAN_SHUTDOWN	0x01	/* channel is shutting down */
#define	SCSIPI_CHAN_OPENINGS	0x02	/* use chan_openings */
#define	SCSIPI_CHAN_CANGROW	0x04	/* channel can grow resources */
#define	SCSIPI_CHAN_NOSETTLE	0x08	/* don't wait for devices to settle */
#define	SCSIPI_CHAN_CALLBACK	0x10	/* has to call chan_callback() */

#define	SCSIPI_CHAN_MAX_PERIPH(chan)					\
	(((chan)->chan_flags & SCSIPI_CHAN_OPENINGS) ?			\
	 (chan)->chan_max_periph : (chan)->chan_adapter->adapt_max_periph)


#define	scsipi_printaddr(periph)					\
	(*(periph)->periph_channel->chan_bustype->bustype_printaddr)((periph))

#define	scsipi_periph_bustype(periph)					\
	(periph)->periph_channel->chan_bustype->bustype_type


/*
 * Number of tag words in a periph structure:
 *
 *	n_tag_words = ((256 / NBBY) / sizeof(u_int32_t))
 */
#define	PERIPH_NTAGWORDS	((256 / 8) / sizeof(u_int32_t))


/*
 * scsipi_periph:
 *
 *	This structure describes the path between a peripherial device
 *	and an adapter.  It contains a pointer to the adapter channel
 *	which in turn contains a pointer to the adapter.
 *
 * XXX Given the way NetBSD's autoconfiguration works, this is ...
 * XXX nasty.
 *
 *	Well, it's a lot nicer than it used to be, but there could
 *	still be an improvement.
 */
struct scsipi_periph {
	struct device *periph_dev;	/* pointer to peripherial's device */
	struct scsipi_channel *periph_channel; /* channel we're connected to */

	const struct scsipi_periphsw *periph_switch; /* peripherial's entry
							points */
	int	periph_openings;	/* max # of outstanding commands */
	int	periph_active;		/* current # of outstanding commands */
	int	periph_sent;		/* current # of commands sent to adapt*/

	int	periph_mode;		/* operation modes, CAP bits */
	int	periph_period;		/* sync period (factor) */
	int	periph_offset;		/* sync offset */

	/*
	 * Information gleaned from the inquiry data.
	 */
	u_int8_t periph_type;		/* basic device type */
	int	periph_cap;		/* capabilities */
	int	periph_quirks;		/* device's quirks */

	int	periph_flags;		/* misc. flags */
	int	periph_dbflags;		/* debugging flags */

	int	periph_target;		/* target ID (drive # on ATAPI) */
	int	periph_lun;		/* LUN (not used on ATAPI) */

	int	periph_version;		/* ANSI SCSI version */

	int	periph_qfreeze;		/* queue freeze count */

	/* Bitmap of free command tags. */
	u_int32_t periph_freetags[PERIPH_NTAGWORDS];

	/* Pending scsipi_xfers on this peripherial. */
	struct scsipi_xfer_queue periph_xferq;

	struct callout periph_callout;

	/* xfer which has a pending CHECK_CONDITION */
	struct scsipi_xfer *periph_xscheck;

};

/*
 * Macro to return the current xfer mode of a periph.
 */
#define	PERIPH_XFER_MODE(periph)					\
	(((periph)->periph_flags & PERIPH_MODE_VALID) ?			\
	 (periph)->periph_mode : 0)

/* periph_cap */
#define	PERIPH_CAP_ANEC		0x0001	/* async event notification */
#define	PERIPH_CAP_TERMIOP	0x0002	/* terminate i/o proc. messages */
#define	PERIPH_CAP_RELADR	0x0004	/* relative addressing */
#define	PERIPH_CAP_WIDE32	0x0008	/* wide-32 transfers */
#define	PERIPH_CAP_WIDE16	0x0010	/* wide-16 transfers */
		/*	XXX	0x0020	   reserved for ATAPI_CFG_DRQ_MASK */
		/*	XXX	0x0040	   reserved for ATAPI_CFG_DRQ_MASK */
#define	PERIPH_CAP_SYNC		0x0080	/* synchronous transfers */
#define	PERIPH_CAP_LINKCMDS	0x0100	/* linked commands */
#define	PERIPH_CAP_TQING	0x0200	/* tagged queueing */
#define	PERIPH_CAP_SFTRESET	0x0400	/* soft RESET condition response */
#define	PERIPH_CAP_CMD16	0x0800	/* 16 byte commands (ATAPI) */

/* periph_flags */
#define	PERIPH_REMOVABLE	0x0001	/* media is removable */
#define	PERIPH_MEDIA_LOADED	0x0002	/* media is loaded */
#define	PERIPH_WAITING		0x0004	/* process waiting for opening */
#define	PERIPH_OPEN		0x0008	/* device is open */
#define	PERIPH_WAITDRAIN	0x0010	/* waiting for pending xfers to drain */
#define	PERIPH_GROW_OPENINGS	0x0020	/* allow openings to grow */
#define	PERIPH_MODE_VALID	0x0040	/* periph_mode is valid */
#define	PERIPH_RECOVERING	0x0080	/* periph is recovering */
#define	PERIPH_RECOVERY_ACTIVE	0x0100	/* a recovery command is active */
#define PERIPH_KEEP_LABEL	0x0200	/* retain label after 'full' close */
#define	PERIPH_SENSE		0x0400	/* periph has sense pending */
#define PERIPH_UNTAG		0x0800	/* untagged command running */

/* periph_quirks */
#define	PQUIRK_AUTOSAVE		0x00000001	/* do implicit SAVE POINTERS */
#define	PQUIRK_NOSYNC		0x00000002	/* does not grok SDTR */
#define	PQUIRK_NOWIDE		0x00000004	/* does not grok WDTR */
#define	PQUIRK_NOTAG		0x00000008	/* does not grok tagged cmds */
#define	PQUIRK_NOLUNS		0x00000010	/* DTWT with LUNs */
#define	PQUIRK_FORCELUNS	0x00000020	/* prehistoric device groks
						   LUNs */
#define	PQUIRK_NOMODESENSE	0x00000040	/* device doesn't do MODE SENSE
						   properly */
#define	PQUIRK_NOSTARTUNIT	0x00000080	/* do not issue START UNIT */
#define	PQUIRK_NOSYNCCACHE	0x00000100	/* do not issue SYNC CACHE */
#define	PQUIRK_CDROM		0x00000200	/* device is a CD-ROM, no
						   matter what else it claims */
#define	PQUIRK_LITTLETOC	0x00000400	/* audio TOC is little-endian */
#define	PQUIRK_NOCAPACITY	0x00000800	/* no READ CD CAPACITY */
#define	PQUIRK_NOTUR		0x00001000	/* no TEST UNIT READY */
#define	PQUIRK_NODOORLOCK	0x00002000	/* can't lock door */
#define	PQUIRK_NOSENSE		0x00004000	/* can't REQUEST SENSE */
#define PQUIRK_ONLYBIG		0x00008000	/* only use SCSI_{R,W}_BIG */
#define PQUIRK_BYTE5_ZERO	0x00010000	/* byte5 in capacity is wrong */
#define PQUIRK_NO_FLEX_PAGE	0x00020000	/* does not support flex geom page */
#define PQUIRK_NOBIGMODESENSE	0x00040000	/* has no big mode-sense op */


/*
 * Error values an adapter driver may return
 */
typedef enum {
	XS_NOERROR,		/* there is no error, (sense is invalid)  */
	XS_SENSE,		/* Check the returned sense for the error */
	XS_SHORTSENSE,		/* Check the ATAPI sense for the error	  */
	XS_DRIVER_STUFFUP,	/* Driver failed to perform operation     */
	XS_RESOURCE_SHORTAGE,	/* adapter resource shortage		  */
	XS_SELTIMEOUT,		/* The device timed out.. turned off?     */
	XS_TIMEOUT,		/* The Timeout reported was caught by SW  */
	XS_BUSY,		/* The device busy, try again later?      */
	XS_RESET,		/* bus was reset; possible retry command  */
	XS_REQUEUE		/* requeue this command */
} scsipi_xfer_result_t;

/*
 * Each scsipi transaction is fully described by one of these structures
 * It includes information about the source of the command and also the
 * device and adapter for which the command is destined.
 *
 * Before the HBA is given this transaction, channel_q is the linkage on
 * the related channel's chan_queue.
 *
 * When the this transaction is taken off the channel's chan_queue and
 * the HBA's request entry point is called with this transaction, the
 * HBA can use the channel_q tag for whatever it likes until it calls
 * scsipi_done for this transaction, at which time it has to stop
 * using channel_q.
 *
 * After scsipi_done is called with this transaction and if there was an
 * error on it, channel_q then becomes the linkage on the related channel's
 * chan_complete cqueue.
 *
 * The device_q member is maintained by the scsipi middle layer.  When
 * a device issues a command, the xfer is placed on that device's
 * pending commands queue.  When an xfer is done and freed, it is taken
 * off the device's queue.  This allows for a device to wait for all of
 * its pending commands to complete.
 */
struct scsipi_xfer {
	TAILQ_ENTRY(scsipi_xfer) channel_q; /* entry on channel queue */
	TAILQ_ENTRY(scsipi_xfer) device_q;  /* device's pending xfers */
	struct callout xs_callout;	/* callout for adapter use */
	int	xs_control;		/* control flags */
	__volatile int xs_status;	/* status flags */
	struct scsipi_periph *xs_periph;/* peripherial doing the xfer */
	int	xs_retries;		/* the number of times to retry */
	int	xs_requeuecnt;		/* number of requeues */
	int	timeout;		/* in milliseconds */
	struct	scsipi_generic *cmd;	/* The scsipi command to execute */
	int	cmdlen;			/* how long it is */
	u_char	*data;			/* dma address OR a uio address */
	int	datalen;		/* data len (blank if uio) */
	int	resid;			/* how much buffer was not touched */
	scsipi_xfer_result_t error;	/* an error value */
	struct	buf *bp;		/* If we need to associate with */
					/* a buf */
	union {
		struct  scsipi_sense_data scsi_sense; /* 32 bytes */
		u_int32_t atapi_sense;
	} sense;

	struct scsipi_xfer *xs_sensefor;/* we are requesting sense for this */
					/* xfer */

	u_int8_t status;		/* SCSI status */

	/*
	 * Info for tagged command queueing.  This may or may not
	 * be used by a given adapter driver.  These are the same
	 * as the bytes in the tag message.
	 */
	u_int8_t xs_tag_type;		/* tag type */
	u_int8_t xs_tag_id;		/* tag ID */

	struct	scsipi_generic cmdstore
	    __attribute__ ((aligned (4)));/* stash the command in here */
};

/*
 * scsipi_xfer control flags
 *
 * To do:
 *
 *	- figure out what to do with XS_CTL_ESCAPE
 *
 *	- replace XS_CTL_URGENT with an `xs_priority' field?
 */
#define	XS_CTL_NOSLEEP		0x00000001	/* don't sleep */
#define	XS_CTL_POLL		0x00000002	/* poll for completion */
#define	XS_CTL_DISCOVERY	0x00000004	/* doing device discovery */
#define	XS_CTL_ASYNC		0x00000008	/* command completes
						   asynchronously */
#define	XS_CTL_USERCMD		0x00000010	/* user issued command */
#define	XS_CTL_SILENT		0x00000020	/* don't print sense info */
#define	XS_CTL_IGNORE_NOT_READY	0x00000040	/* ignore NOT READY */
#define	XS_CTL_IGNORE_MEDIA_CHANGE 					\
				0x00000080	/* ignore media change */
#define	XS_CTL_IGNORE_ILLEGAL_REQUEST					\
				0x00000100	/* ignore ILLEGAL REQUEST */
#define	XS_CTL_SILENT_NODEV	0x00000200	/* don't print sense info
						   if sense info is nodev */
#define	XS_CTL_RESET		0x00000400	/* reset the device */
#define	XS_CTL_DATA_UIO		0x00000800	/* xs_data points to uio */
#define	XS_CTL_DATA_IN		0x00001000	/* data coming into memory */
#define	XS_CTL_DATA_OUT		0x00002000	/* data going out of memory */
#define	XS_CTL_TARGET		0x00004000	/* target mode operation */
#define	XS_CTL_ESCAPE		0x00008000	/* escape operation */
#define	XS_CTL_URGENT		0x00010000	/* urgent (recovery)
						   operation */
#define	XS_CTL_SIMPLE_TAG	0x00020000	/* use a Simple Tag */
#define	XS_CTL_ORDERED_TAG	0x00040000	/* use an Ordered Tag */
#define	XS_CTL_HEAD_TAG		0x00080000	/* use a Head of Queue Tag */
#define	XS_CTL_THAW_PERIPH	0x00100000	/* thaw periph once enqueued */
#define	XS_CTL_FREEZE_PERIPH	0x00200000	/* freeze periph when done */
#define XS_CTL_DATA_ONSTACK	0x00400000	/* data is alloc'ed on stack */
#define XS_CTL_REQSENSE		0x00800000	/* xfer is a request sense */

#define	XS_CTL_TAGMASK	(XS_CTL_SIMPLE_TAG|XS_CTL_ORDERED_TAG|XS_CTL_HEAD_TAG)

#define	XS_CTL_TAGTYPE(xs)	((xs)->xs_control & XS_CTL_TAGMASK)

/*
 * scsipi_xfer status flags
 */
#define	XS_STS_DONE		0x00000001	/* scsipi_xfer is done */
#define	XS_STS_PRIVATE		0xf0000000	/* reserved for HBA's use */

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
	struct scsipi_periph *sa_periph;
	struct scsipi_inquiry_pattern sa_inqbuf;
	struct scsipi_inquiry_data *sa_inqptr;
	union {				/* bus-type specific infos */
		u_int8_t scsi_version;	/* SCSI version */
	} scsipi_info;
};

/*
 * this describes a quirk entry
 */
struct scsi_quirk_inquiry_pattern {
	struct scsipi_inquiry_pattern pattern;
	int quirks;
};

/*
 * Default number of retries, used for generic routines.
 */
#define SCSIPIRETRIES 4


#ifdef _KERNEL
void	scsipi_init __P((void));
int	scsipi_command __P((struct scsipi_periph *,
	    struct scsipi_generic *, int, u_char *, int,
	    int, int, struct buf *, int));
void	scsipi_create_completion_thread __P((void *));
caddr_t	scsipi_inqmatch __P((struct scsipi_inquiry_pattern *, caddr_t,
	    int, int, int *));
char	*scsipi_dtype __P((int));
void	scsipi_strvis __P((u_char *, int, u_char *, int));
int	scsipi_execute_xs __P((struct scsipi_xfer *));
u_long	scsipi_size __P((struct scsipi_periph *, int));
int	scsipi_test_unit_ready __P((struct scsipi_periph *, int));
int	scsipi_prevent __P((struct scsipi_periph *, int, int));
int	scsipi_inquire __P((struct scsipi_periph *,
	    struct scsipi_inquiry_data *, int));
int	scsipi_mode_select __P((struct scsipi_periph *, int,
	    struct scsipi_mode_header *, int, int, int, int));
int	scsipi_mode_select_big __P((struct scsipi_periph *, int,
	    struct scsipi_mode_header_big *, int, int, int, int));
int	scsipi_mode_sense __P((struct scsipi_periph *, int, int,
	    struct scsipi_mode_header *, int, int, int, int));
int	scsipi_mode_sense_big __P((struct scsipi_periph *, int, int,
	    struct scsipi_mode_header_big *, int, int, int, int));
int	scsipi_start __P((struct scsipi_periph *, int, int));
void	scsipi_done __P((struct scsipi_xfer *));
void	scsipi_user_done __P((struct scsipi_xfer *));
int	scsipi_interpret_sense __P((struct scsipi_xfer *));
void	scsipi_wait_drain __P((struct scsipi_periph *));
void	scsipi_kill_pending __P((struct scsipi_periph *));
struct scsipi_periph *scsipi_alloc_periph __P((int));
#ifdef SCSIVERBOSE
void	scsipi_print_sense __P((struct scsipi_xfer *, int));
void	scsipi_print_sense_data __P((struct scsipi_sense_data *, int));
char   *scsipi_decode_sense __P((void *, int));
#endif
int	scsipi_thread_call_callback __P((struct scsipi_channel *,
	    void (*callback) __P((struct scsipi_channel *, void *)),
	    void *));
void	scsipi_async_event __P((struct scsipi_channel *,
	    scsipi_async_event_t, void *));
int	scsipi_do_ioctl __P((struct scsipi_periph *, struct vnode *, u_long,
	    caddr_t, int, struct proc *));

void	scsipi_print_xfer_mode __P((struct scsipi_periph *));
void	scsipi_set_xfer_mode __P((struct scsipi_channel *, int, int));

int	scsipi_channel_init __P((struct scsipi_channel *));
void	scsipi_channel_shutdown __P((struct scsipi_channel *));

void	scsipi_insert_periph __P((struct scsipi_channel *,
	    struct scsipi_periph *));
void	scsipi_remove_periph __P((struct scsipi_channel *,
	    struct scsipi_periph *));
struct scsipi_periph *scsipi_lookup_periph __P((struct scsipi_channel *,
	    int, int));
int	scsipi_target_detach __P((struct scsipi_channel *, int, int, int));

int	scsipi_adapter_addref __P((struct scsipi_adapter *));
void	scsipi_adapter_delref __P((struct scsipi_adapter *));

void	scsipi_channel_freeze __P((struct scsipi_channel *, int));
void	scsipi_channel_thaw __P((struct scsipi_channel *, int));
void	scsipi_channel_timed_thaw __P((void *));

void	scsipi_periph_freeze __P((struct scsipi_periph *, int));
void	scsipi_periph_thaw __P((struct scsipi_periph *, int));
void	scsipi_periph_timed_thaw __P((void *));

int	scsipi_sync_period_to_factor __P((int));
int	scsipi_sync_factor_to_period __P((int));
int	scsipi_sync_factor_to_freq __P((int));

void	show_scsipi_xs __P((struct scsipi_xfer *));
void	show_scsipi_cmd __P((struct scsipi_xfer *));
void	show_mem __P((u_char *, int));
#endif /* _KERNEL */

static __inline void _lto2b __P((u_int32_t val, u_int8_t *bytes))
	__attribute__ ((unused));
static __inline void _lto3b __P((u_int32_t val, u_int8_t *bytes))
	__attribute__ ((unused));
static __inline void _lto4b __P((u_int32_t val, u_int8_t *bytes))
	__attribute__ ((unused));
static __inline u_int32_t _2btol __P((const u_int8_t *bytes))
	__attribute__ ((unused));
static __inline u_int32_t _3btol __P((const u_int8_t *bytes))
	__attribute__ ((unused));
static __inline u_int32_t _4btol __P((const u_int8_t *bytes))
	__attribute__ ((unused));

static __inline void _lto2l __P((u_int32_t val, u_int8_t *bytes))
	__attribute__ ((unused));
static __inline void _lto3l __P((u_int32_t val, u_int8_t *bytes))
	__attribute__ ((unused));
static __inline void _lto4l __P((u_int32_t val, u_int8_t *bytes))
	__attribute__ ((unused));
static __inline u_int32_t _2ltol __P((const u_int8_t *bytes))
	__attribute__ ((unused));
static __inline u_int32_t _3ltol __P((const u_int8_t *bytes))
	__attribute__ ((unused));
static __inline u_int32_t _4ltol __P((const u_int8_t *bytes))
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
	const u_int8_t *bytes;
{
	u_int32_t rv;

	rv = (bytes[0] << 8) |
	     bytes[1];
	return (rv);
}

static __inline u_int32_t
_3btol(bytes)
	const u_int8_t *bytes;
{
	u_int32_t rv;

	rv = (bytes[0] << 16) |
	     (bytes[1] << 8) |
	     bytes[2];
	return (rv);
}

static __inline u_int32_t
_4btol(bytes)
	const u_int8_t *bytes;
{
	u_int32_t rv;

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
	const u_int8_t *bytes;
{
	u_int32_t rv;

	rv = bytes[0] |
	     (bytes[1] << 8);
	return (rv);
}

static __inline u_int32_t
_3ltol(bytes)
	const u_int8_t *bytes;
{
	u_int32_t rv;

	rv = bytes[0] |
	     (bytes[1] << 8) |
	     (bytes[2] << 16);
	return (rv);
}

static __inline u_int32_t
_4ltol(bytes)
	const u_int8_t *bytes;
{
	u_int32_t rv;

	rv = bytes[0] |
	     (bytes[1] << 8) |
	     (bytes[2] << 16) |
	     (bytes[3] << 24);
	return (rv);
}

#endif /* _DEV_SCSIPI_SCSIPICONF_H_ */
