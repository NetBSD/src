/*	$NetBSD: umassvar.h,v 1.2 2001/04/13 12:51:43 augustss Exp $	*/
/*-
 * Copyright (c) 1999 MAEKAWA Masahide <bishop@rr.iij4u.or.jp>,
 *		      Nick Hibma <n_hibma@freebsd.org>
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
 *
 *     $FreeBSD: src/sys/dev/usb/umass.c,v 1.13 2000/03/26 01:39:12 n_hibma Exp $
 */

#ifdef UMASS_DEBUG
#define DIF(m, x)	if (umassdebug & (m)) do { x ; } while (0)
#define DPRINTF(m, x)	if (umassdebug & (m)) logprintf x
#define UDMASS_UPPER	0x00008000	/* upper layer */
#define UDMASS_GEN	0x00010000	/* general */
#define UDMASS_SCSI	0x00020000	/* scsi */
#define UDMASS_UFI	0x00040000	/* ufi command set */
#define UDMASS_8070	0x00080000	/* 8070i command set */
#define UDMASS_USB	0x00100000	/* USB general */
#define UDMASS_BBB	0x00200000	/* Bulk-Only transfers */
#define UDMASS_CBI	0x00400000	/* CBI transfers */
#define UDMASS_ALL	0xffff0000	/* all of the above */

#define UDMASS_XFER	0x40000000	/* all transfers */
#define UDMASS_CMD	0x80000000

extern int umassdebug;
#else
#define DIF(m, x)	/* nop */
#define DPRINTF(m, x)	/* nop */
#endif

/* Generic definitions */

#define UFI_COMMAND_LENGTH 12

/* Direction for umass_*_transfer */
#define DIR_NONE	0
#define DIR_IN		1
#define DIR_OUT		2

/* The transfer speed determines the timeout value */
#define UMASS_DEFAULT_TRANSFER_SPEED	150	/* in kb/s, conservative est. */
#define UMASS_FLOPPY_TRANSFER_SPEED	20
#define UMASS_ZIP100_TRANSFER_SPEED	650

#define UMASS_SPINUP_TIME 10000	/* ms */

#define MS_TO_TICKS(ms) ((ms) * hz / 1000)			      


/* Bulk-Only features */

#define UR_BBB_RESET	0xff		/* Bulk-Only reset */
#define	UR_BBB_GET_MAX_LUN	0xfe

/* Command Block Wrapper */
typedef struct {
	uDWord		dCBWSignature;
#	define CBWSIGNATURE	0x43425355
	uDWord		dCBWTag;
	uDWord		dCBWDataTransferLength;
	uByte		bCBWFlags;
#	define CBWFLAGS_OUT	0x00
#	define CBWFLAGS_IN	0x80
	uByte		bCBWLUN;
	uByte		bCDBLength;
#	define CBWCDBLENGTH	16
	uByte		CBWCDB[CBWCDBLENGTH];
} umass_bbb_cbw_t;
#define UMASS_BBB_CBW_SIZE	31

/* Command Status Wrapper */
typedef struct {
	uDWord		dCSWSignature;
#	define CSWSIGNATURE	0x53425355
	uDWord		dCSWTag;
	uDWord		dCSWDataResidue;
	uByte		bCSWStatus;
#	define CSWSTATUS_GOOD	0x0
#	define CSWSTATUS_FAILED 0x1
#	define CSWSTATUS_PHASE	0x2
} umass_bbb_csw_t;
#define UMASS_BBB_CSW_SIZE	13

/* CBI features */

#define UR_CBI_ADSC	0x00

typedef unsigned char umass_cbi_cbl_t[16];	/* Command block */

typedef union {
	struct {
		unsigned char	type;
		#define IDB_TYPE_CCI		0x00
		unsigned char	value;
		#define IDB_VALUE_PASS		0x00
		#define IDB_VALUE_FAIL		0x01
		#define IDB_VALUE_PHASE		0x02
		#define IDB_VALUE_PERSISTENT	0x03
		#define IDB_VALUE_STATUS_MASK	0x03
	} common;

	struct {
		unsigned char	asc;
		unsigned char	ascq;
	} ufi;
} umass_cbi_sbl_t;

struct umass_softc;		/* see below */

typedef void (*transfer_cb_f)(struct umass_softc *sc, void *priv,
			      int residue, int status);
#define STATUS_CMD_OK		0	/* everything ok */
#define STATUS_CMD_UNKNOWN	1	/* will have to fetch sense */
#define STATUS_CMD_FAILED	2	/* transfer was ok, command failed */
#define STATUS_WIRE_FAILED	3	/* couldn't even get command across */

typedef void (*wire_reset_f)(struct umass_softc *sc, int status);
typedef void (*wire_transfer_f)(struct umass_softc *sc, int lun,
				void *cmd, int cmdlen, void *data, int datalen, 
				int dir, transfer_cb_f cb, void *priv);
typedef void (*wire_state_f)(usbd_xfer_handle xfer,
			     usbd_private_handle priv, usbd_status err);


/* the per device structure */
struct umass_softc {
	USBBASEDEVICE		sc_dev;		/* base device */
	usbd_device_handle	sc_udev;	/* device */

	unsigned char		drive;
#	define DRIVE_GENERIC		0	/* use defaults for this one */
#	define ZIP_100			1	/* to be used for quirks */
#	define ZIP_250			2
#	define SHUTTLE_EUSB		3
#	define INSYSTEM_USBCABLE	4
	unsigned char		quirks;
	/* The drive does not support Test Unit Ready. Convert to
	 * Start Unit.
	 * Y-E Data
	 * ZIP 100
	 */
#	define NO_TEST_UNIT_READY	0x01
	/* The drive does not reset the Unit Attention state after
	 * REQUEST SENSE has been sent. The INQUIRY command does not reset
	 * the UA either, and so CAM runs in circles trying to retrieve the
	 * initial INQUIRY data.
	 * Y-E Data
	 */
#	define RS_NO_CLEAR_UA		0x02	/* no REQUEST SENSE on INQUIRY*/
	/* The drive does not support START_STOP.
	 * Shuttle E-USB
	 */
#	define NO_START_STOP		0x04
	/* Don't ask for full inquiry data (255 bytes).
	 * Yano ATAPI-USB
	 */
#       define FORCE_SHORT_INQUIRY      0x08

	unsigned int		proto;
#	define PROTO_UNKNOWN	0x0000		/* unknown protocol */
#	define PROTO_BBB	0x0001		/* USB wire protocol */
#	define PROTO_CBI	0x0002
#	define PROTO_CBI_I	0x0004
#	define PROTO_WIRE	0x00ff		/* USB wire protocol mask */
#	define PROTO_SCSI	0x0100		/* command protocol */
#	define PROTO_ATAPI	0x0200
#	define PROTO_UFI	0x0400
#	define PROTO_RBC	0x0800
#	define PROTO_COMMAND	0xff00		/* command protocol mask */

	u_char			subclass;	/* interface subclass */
	u_char			protocol;	/* interface protocol */

	usbd_interface_handle	iface;		/* Mass Storage interface */
	int			ifaceno;	/* MS iface number */

	u_int8_t		bulkin;		/* bulk-in Endpoint Address */
	u_int8_t		bulkout;	/* bulk-out Endpoint Address */
	u_int8_t		intrin;		/* intr-in Endp. (CBI) */
	usbd_pipe_handle	bulkin_pipe;
	usbd_pipe_handle	bulkout_pipe;
	usbd_pipe_handle	intrin_pipe;

	/* Reset the device in a wire protocol specific way */
	wire_reset_f		reset;

	/* The start of a wire transfer. It prepares the whole transfer (cmd,
	 * data, and status stage) and initiates it. It is up to the state
	 * machine (below) to handle the various stages and errors in these
	 */
	wire_transfer_f		transfer;

	/* The state machine, handling the various states during a transfer */
	wire_state_f		state;

	/* Bulk specific variables for transfers in progress */
	umass_bbb_cbw_t		cbw;	/* command block wrapper */
	umass_bbb_csw_t		csw;	/* command status wrapper*/
	/* CBI specific variables for transfers in progress */
	umass_cbi_cbl_t		cbl;	/* command block */ 
	umass_cbi_sbl_t		sbl;	/* status block */

	/* generic variables for transfers in progress */
	/* ctrl transfer requests */
	usb_device_request_t	request;

	/* xfer handles
	 * Most of our operations are initiated from interrupt context, so
	 * we need to avoid using the one that is in use. We want to avoid
	 * allocating them in the interrupt context as well.
	 */
	/* indices into array below */
#	define XFER_BBB_CBW		0	/* Bulk-Only */
#	define XFER_BBB_DATA		1
#	define XFER_BBB_DCLEAR		2
#	define XFER_BBB_CSW1		3
#	define XFER_BBB_CSW2		4
#	define XFER_BBB_SCLEAR		5
#	define XFER_BBB_RESET1		6
#	define XFER_BBB_RESET2		7
#	define XFER_BBB_RESET3		8
	
#	define XFER_CBI_CB		0	/* CBI */
#	define XFER_CBI_DATA		1
#	define XFER_CBI_STATUS		2
#	define XFER_CBI_DCLEAR		3
#	define XFER_CBI_SCLEAR		4
#	define XFER_CBI_RESET1		5
#	define XFER_CBI_RESET2		6
#	define XFER_CBI_RESET3		7

#	define XFER_NR			9	/* maximum number */

	usbd_xfer_handle	transfer_xfer[XFER_NR]; /* for ctrl xfers */

	void			*data_buffer;

	int			transfer_dir;		/* data direction */
	void			*transfer_data;		/* data buffer */
	int			transfer_datalen;	/* (maximum) length */
	int			transfer_actlen;	/* actual length */ 
	transfer_cb_f		transfer_cb;		/* callback */
	void			*transfer_priv;		/* for callback */
	int			transfer_status;

	int			transfer_state;
#	define TSTATE_IDLE			0
#	define TSTATE_BBB_COMMAND		1	/* CBW transfer */
#	define TSTATE_BBB_DATA			2	/* Data transfer */
#	define TSTATE_BBB_DCLEAR		3	/* clear endpt stall */
#	define TSTATE_BBB_STATUS1		4	/* clear endpt stall */
#	define TSTATE_BBB_SCLEAR		5	/* clear endpt stall */
#	define TSTATE_BBB_STATUS2		6	/* CSW transfer */
#	define TSTATE_BBB_RESET1		7	/* reset command */
#	define TSTATE_BBB_RESET2		8	/* in clear stall */
#	define TSTATE_BBB_RESET3		9	/* out clear stall */
#	define TSTATE_CBI_COMMAND		10	/* command transfer */
#	define TSTATE_CBI_DATA			11	/* data transfer */
#	define TSTATE_CBI_STATUS		12	/* status transfer */
#	define TSTATE_CBI_DCLEAR		13	/* clear ep stall */
#	define TSTATE_CBI_SCLEAR		14	/* clear ep stall */
#	define TSTATE_CBI_RESET1		15	/* reset command */
#	define TSTATE_CBI_RESET2		16	/* in clear stall */
#	define TSTATE_CBI_RESET3		17	/* out clear stall */
#	define TSTATE_STATES			18	/* # of states above */


	int			transfer_speed;		/* in kb/s */
	int			timeout;		/* in msecs */

	u_int8_t		maxlun;			/* max lun supported */

#ifdef UMASS_DEBUG
	struct timeval tv;
#endif

	int			sc_xfer_flags;
	char			sc_dying;

	struct umassbus_softc	bus;
};

