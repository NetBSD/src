/* $NetBSD: uscsi_subr.c,v 1.5 2022/05/28 21:14:57 andvar Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum; Jason R. Thorpe of the Numerical Aerospace
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
 *
 * Small changes, generalisations and Linux support by Reinoud Zandijk
 * <reinoud@netbsd.org>.
 *
 */


/*
 * SCSI support subroutines.
 */

#include <sys/param.h>
#include <sys/ioctl.h>
#include <err.h> 
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <inttypes.h>
#include <assert.h>

#include "uscsilib.h"


int uscsilib_verbose = 0;


#ifdef USCSI_SCSIPI
	/*
	 * scsipi is a integrated SCSI and ATAPI layer under NetBSD and exists
	 * in a modified form under OpenBSD and possibly also under other
	 * operating systems.
	 */


#include <sys/scsiio.h>
#ifdef __OpenBSD__
#include <scsi/uscsi_all.h>
#else
#include <dev/scsipi/scsipi_all.h>
#endif


int
uscsi_open(struct uscsi_dev *disc)
{
	struct stat dstat;
 
	disc->fhandle = open(disc->dev_name, O_RDWR, 0); /* no create */
	if (disc->fhandle<0) {
		perror("Failure to open device or file");
		return ENODEV;
	}
 
	if (fstat(disc->fhandle, &dstat) < 0) {
		perror("Can't stat device or file");
		uscsi_close(disc);
		return ENODEV;
	}

	return 0;
}


int
uscsi_close(struct uscsi_dev * disc)
{
	close(disc->fhandle);
	disc->fhandle = -1;

	return 0;
}


int
uscsi_command(int flags, struct uscsi_dev *disc,
	void *cmd, size_t cmdlen, void *data, size_t datalen,
	uint32_t timeout, struct uscsi_sense *uscsi_sense)
{
	scsireq_t req;

	memset(&req, 0, sizeof(req));
	if (uscsi_sense)
		bzero(uscsi_sense, sizeof(struct uscsi_sense));

	memcpy(req.cmd, cmd, cmdlen);
	req.cmdlen = cmdlen;
	req.databuf = data;
	req.datalen = datalen;
	req.timeout = timeout;
	req.flags = flags;
	req.senselen = SENSEBUFLEN;

	if (ioctl(disc->fhandle, SCIOCCOMMAND, &req) == -1)
		err(1, "SCIOCCOMMAND");

	if (req.retsts == SCCMD_OK)
		return 0;

	/* Some problem; report it and exit. */
	if (req.retsts == SCCMD_TIMEOUT) {
		if (uscsilib_verbose)
			fprintf(stderr, "%s: SCSI command timed out\n",
				disc->dev_name);
		return EAGAIN;
	} else if (req.retsts == SCCMD_BUSY) {
		if (uscsilib_verbose)
			fprintf(stderr, "%s: device is busy\n",
				disc->dev_name);
		return EBUSY;
	} else if (req.retsts == SCCMD_SENSE) {
		if (uscsi_sense) {
			uscsi_sense->asc        =  req.sense[12];
			uscsi_sense->ascq       =  req.sense[13];
			uscsi_sense->skey_valid =  req.sense[15] & 128;
			uscsi_sense->sense_key  = (req.sense[16] << 8) |
						  (req.sense[17]);
		}
		if (uscsilib_verbose)
			uscsi_print_sense((char *) disc->dev_name,
				req.cmd, req.cmdlen,
				req.sense, req.senselen_used, 1);
		return EIO;
	} else
		if (uscsilib_verbose)
			fprintf(stderr, "%s: device had unknown status %x\n",
				disc->dev_name,
		  	  req.retsts);

	return EFAULT;
}


/*
 * The reasoning behind this explicit copy is for compatibility with changes
 * in our uscsi_addr structure.
 */
int
uscsi_identify(struct uscsi_dev *disc, struct uscsi_addr *saddr) 
{
	struct scsi_addr raddr;
	int error;

	bzero(saddr, sizeof(struct scsi_addr));
	error = ioctl(disc->fhandle, SCIOCIDENTIFY, &raddr);
	if (error) return error;

#ifdef __NetBSD__
	/* scsi and atapi are split up like in uscsi_addr */
	if (raddr.type == 0) {
		saddr->type = USCSI_TYPE_SCSI;
		saddr->addr.scsi.scbus  = raddr.addr.scsi.scbus;
		saddr->addr.scsi.target = raddr.addr.scsi.target;
		saddr->addr.scsi.lun    = raddr.addr.scsi.lun;
	} else {
		saddr->type = USCSI_TYPE_ATAPI;
		saddr->addr.atapi.atbus = raddr.addr.atapi.atbus;
		saddr->addr.atapi.drive = raddr.addr.atapi.drive;
	}
#endif
#ifdef __OpenBSD__
	/* atapi's are shown as SCSI devices */
	if (raddr.type == 0) {
		saddr->type = USCSI_TYPE_SCSI;
		saddr->addr.scsi.scbus  = raddr.scbus;
		saddr->addr.scsi.target = raddr.target;
		saddr->addr.scsi.lun    = raddr.lun;
	} else {
		saddr->type = USCSI_TYPE_ATAPI;
		saddr->addr.atapi.atbus = raddr.scbus;	/* overload */
		saddr->addr.atapi.drive = raddr.target;	/* overload */
	}
#endif

	return 0;
}


int
uscsi_check_for_scsi(struct uscsi_dev *disc) 
{
	struct uscsi_addr	saddr;

	return uscsi_identify(disc, &saddr);
}
#endif	/* SCSILIB_SCSIPI */




#ifdef USCSI_LINUX_SCSI
	/*
	 * Support code for Linux SCSI code. It uses the ioctl() way of
	 * communicating since this is more close to the original NetBSD
	 * scsipi implementation.
	 */
#include <scsi/sg.h>
#include <scsi/scsi.h>

#define SENSEBUFLEN 48


int
uscsi_open(struct uscsi_dev * disc)
{
	int flags;
	struct stat stat;

	/* in Linux we are NOT allowed to open it blocking */
	/* no create! */
	disc->fhandle = open(disc->dev_name, O_RDWR | O_NONBLOCK, 0);
	if (disc->fhandle<0) {
		perror("Failure to open device or file");
		return ENODEV;
	}

	/* explicitly mark it non blocking (again) (silly Linux) */
	flags = fcntl(disc->fhandle, F_GETFL);
	flags &= ~O_NONBLOCK;
	fcntl(disc->fhandle, F_SETFL, flags);

	if (fstat(disc->fhandle, &stat) < 0) {
		perror("Can't stat device or file");
		uscsi_close(disc);
		return ENODEV;
	}

	return 0;
}


int
uscsi_close(struct uscsi_dev * disc)
{ 
	close(disc->fhandle);
	disc->fhandle = -1;

	return 0;
}


int
uscsi_command(int flags, struct uscsi_dev *disc,
	void *cmd, size_t cmdlen,
	void *data, size_t datalen,
	uint32_t timeout, struct uscsi_sense *uscsi_sense) 
{
	struct sg_io_hdr req;
	uint8_t sense_buffer[SENSEBUFLEN];
	int error;

	bzero(&req, sizeof(req));
	if (flags == SG_DXFER_FROM_DEV) bzero(data, datalen);

	req.interface_id    = 'S';
	req.dxfer_direction = flags;
	req.cmd_len	    = cmdlen;
	req.mx_sb_len	    = SENSEBUFLEN;
	req.iovec_count	    = 0;
	req.dxfer_len	    = datalen;
	req.dxferp	    = data;
	req.cmdp	    = cmd;
	req.sbp		    = sense_buffer;
	req.flags	    = 0;
	req.timeout	    = timeout;

	error = ioctl(disc->fhandle, SG_IO, &req);

	if (req.status) {
		/* Is this OK? */
		if (uscsi_sense) {
			uscsi_sense->asc        =  sense_buffer[12];
			uscsi_sense->ascq       =  sense_buffer[13];
			uscsi_sense->skey_valid =  sense_buffer[15] & 128;
			uscsi_sense->sense_key  = (sense_buffer[16] << 8) |
						  (sense_buffer[17]);
		}
		if (uscsilib_verbose) {
			uscsi_print_sense((char *) disc->dev_name,
				cmd, cmdlen, sense_buffer, req.sb_len_wr, 1);
		}
	}

	return error;
}


int
uscsi_identify(struct uscsi_dev *disc, struct uscsi_addr *saddr) 
{
	struct sg_scsi_id sg_scsi_id;
	struct sg_id {
		/* target | lun << 8 | channel << 16 | low_ino << 24 */
		uint32_t tlci;
		uint32_t uniq_id;
	} sg_id;
	int emulated;
	int error;

	/* clean result */
	bzero(saddr, sizeof(struct uscsi_addr));

	/* check if its really SCSI or emulated SCSI (ATAPI f.e.) */
	saddr->type = USCSI_TYPE_SCSI;
	ioctl(disc->fhandle, SG_EMULATED_HOST, &emulated);
	if (emulated) saddr->type = USCSI_TYPE_ATAPI;

	/* try 2.4 kernel or older */
	error = ioctl(disc->fhandle, SG_GET_SCSI_ID, &sg_scsi_id);
	if (!error) {
		saddr->addr.scsi.target = sg_scsi_id.scsi_id;
		saddr->addr.scsi.lun    = sg_scsi_id.lun;
		saddr->addr.scsi.scbus  = sg_scsi_id.channel;

		return 0;
	}

	/* 2.6 kernel or newer */
 	error = ioctl(disc->fhandle, SCSI_IOCTL_GET_IDLUN, &sg_id);
	if (error) return error;

	saddr->addr.scsi.target = (sg_id.tlci      ) & 0xff;
	saddr->addr.scsi.lun    = (sg_id.tlci >>  8) & 0xff;
	saddr->addr.scsi.scbus  = (sg_id.tlci >> 16) & 0xff;

	return 0;
}


int uscsi_check_for_scsi(struct uscsi_dev *disc) {
	struct uscsi_addr saddr;

	return uscsi_identify(disc, &saddr);
}
#endif	/* USCSI_LINUX_SCSI */




#ifdef USCSI_FREEBSD_CAM

int
uscsi_open(struct uscsi_dev *disc) 
{
	disc->devhandle = cam_open_device(disc->dev_name, O_RDWR);

	if (disc->devhandle == NULL) {
		disc->fhandle = open(disc->dev_name, O_RDWR | O_NONBLOCK, 0);
		if (disc->fhandle < 0) {
			perror("Failure to open device or file");
			return ENODEV;
		}
	}

	return 0;
}


int
uscsi_close(struct uscsi_dev *disc) 
{
	if (disc->devhandle != NULL) {
		cam_close_device(disc->devhandle);
		disc->devhandle = NULL;
	} else {
		close(disc->fhandle);
		disc->fhandle = -1;
	}

	return 0;
}


int
uscsi_command(int flags, struct uscsi_dev *disc,
	void *cmd, size_t cmdlen,
	void *data, size_t datalen,
	uint32_t timeout, struct uscsi_sense *uscsi_sense) 
{
	struct cam_device *cam_dev;
	struct scsi_sense_data *cam_sense_data;
	union ccb ccb;
	uint32_t cam_sense;
	uint8_t *keypos;
	int camflags;

	memset(&ccb, 0, sizeof(ccb));
	cam_dev = (struct cam_device *) disc->devhandle;

	if (datalen == 0) flags = SCSI_NODATACMD;
	/* optional : */
	/* if (data) assert(flags == SCSI_NODATACMD); */

	camflags = CAM_DIR_NONE;
	if (flags & SCSI_READCMD)
		camflags = CAM_DIR_IN;
	if (flags & SCSI_WRITECMD)
		camflags = CAM_DIR_OUT;

	cam_fill_csio(
		&ccb.csio,
		0,			/* retries */
		NULL,			/* cbfcnp */
		camflags,		/* flags */
		MSG_SIMPLE_Q_TAG,	/* tag_action */
		(u_int8_t *) data,	/* data_ptr */
		datalen,		/* dxfer_len */
		SSD_FULL_SIZE,		/* sense_len */
		cmdlen,			/* cdb_len */
		timeout			/* timeout */
	);
 
	/* Disable freezing the device queue */
	ccb.ccb_h.flags |= CAM_DEV_QFRZDIS;
 
	memcpy(ccb.csio.cdb_io.cdb_bytes, cmd, cmdlen);

	/* Send the command down via the CAM interface */
	if (cam_send_ccb(cam_dev, &ccb) < 0) {
		err(1, "cam_send_ccb");
	}

	if ((ccb.ccb_h.status & CAM_STATUS_MASK) == CAM_REQ_CMP)
		return 0;

	/* print error using the uscsi_sense routines? */

	cam_sense = (ccb.ccb_h.status & (CAM_STATUS_MASK | CAM_AUTOSNS_VALID));
	if (cam_sense != (CAM_SCSI_STATUS_ERROR | CAM_AUTOSNS_VALID))
		return EFAULT;

	/* drive responds with sense information */
	if (!uscsilib_verbose)
		return EFAULT;

	/* print sense info */
	cam_sense_data = &ccb.csio.sense_data;
	if (uscsi_sense) {
		uscsi_sense->asc  = cam_sense_data->add_sense_code;
		uscsi_sense->ascq = cam_sense_data->add_sense_code_qual;
		keypos  = cam_sense_data->sense_key_spec;
		uscsi_sense->skey_valid =  keypos[0] & 128;
		uscsi_sense->sense_key  = (keypos[1] << 8) | (keypos[2]);
	}

	uscsi_print_sense((char *) disc->dev_name,
		cmd, cmdlen,
		(uint8_t *) cam_sense_data, 8 + cam_sense_data->extra_len, 1);

	return EFAULT;
}


int
uscsi_identify(struct uscsi_dev *disc, struct uscsi_addr *saddr)
{
	struct cam_device *cam_dev;
	
	/* clean result */
	bzero(saddr, sizeof(struct uscsi_addr));

	cam_dev = (struct cam_device *) disc->devhandle;
	if (!cam_dev) return ENODEV;

	/* check if its really SCSI or emulated SCSI (ATAPI f.e.) ? */
	saddr->type = USCSI_TYPE_SCSI;
	saddr->addr.scsi.target = cam_dev->target_id;
	saddr->addr.scsi.lun    = cam_dev->target_lun;
	saddr->addr.scsi.scbus  = cam_dev->bus_id;

	return 0;
}


int
uscsi_check_for_scsi(struct uscsi_dev *disc) 
{
	struct uscsi_addr saddr;

	return uscsi_identify(disc, &saddr);
}

#endif	/* USCSI_FREEBSD_CAM */



/*
 * Generic SCSI functions also used by the sense printing functionality.
 * FreeBSD support has it already asked for by the CAM.
 */

int
uscsi_mode_sense(struct uscsi_dev *dev,
	uint8_t pgcode, uint8_t pctl, void *buf, size_t len) 
{
	scsicmd cmd;

	bzero(buf, len);		/* initialise receiving buffer	*/

	bzero(cmd, SCSI_CMD_LEN);
	cmd[ 0] = 0x1a;			/* MODE SENSE			*/
	cmd[ 1] = 0;			/* -				*/
	cmd[ 2] = pgcode | pctl;	/* page code and control flags	*/
	cmd[ 3] = 0;			/* -				*/
	cmd[ 4] = len;			/* length of receive buffer	*/
	cmd[ 5] = 0;			/* control			*/

	return uscsi_command(SCSI_READCMD, dev, &cmd, 6, buf, len, 10000, NULL);
}


int
uscsi_mode_select(struct uscsi_dev *dev,
	uint8_t byte2, void *buf, size_t len) 
{
	scsicmd cmd;

	bzero(cmd, SCSI_CMD_LEN);
	cmd[ 0] = 0x15;			/* MODE SELECT			*/
	cmd[ 1] = 0x10 | byte2;		/* SCSI-2 page format select	*/
	cmd[ 4] = len;			/* length of page settings	*/
	cmd[ 5] = 0;			/* control			*/

	return uscsi_command(SCSI_WRITECMD, dev, &cmd, 6, buf, len,
			10000, NULL);
}


int
uscsi_request_sense(struct uscsi_dev *dev, void *buf, size_t len) 
{
	scsicmd cmd;

	bzero(buf, len);		/* initialise receiving buffer	*/

	bzero(cmd, SCSI_CMD_LEN);
	cmd[ 0] = 0x03;			/* REQUEST SENSE		*/
	cmd[ 4] = len;			/* length of data to be read	*/
	cmd[ 5] = 0;			/* control			*/

	return uscsi_command(SCSI_WRITECMD, dev, &cmd, 6, buf, len,
			10000, NULL);
}


/* end of uscsi_subr.c */

