/* $NetBSD: uscsilib.h,v 1.1.2.2 2008/05/18 12:36:22 yamt Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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
 * Small changes made by Reinoud Zandijk <reinoud@netbsd.org>
 *
 */

#ifndef _SCSILIB_H_
#define _SCSILIB_H_


#define SCSI_CMD_LEN  12
typedef unsigned char scsicmd[SCSI_CMD_LEN];

#include "defs.h"
#include "uscsilib_machdep.h"
extern	int uscsilib_verbose;


/*
 * Unified structure copied and modified from NetBSD's <sys/scsiio.h> for ease
 */

struct  uscsi_addr {
	int type;	/* bus type */
#define USCSI_TYPE_SCSI  0
#define USCSI_TYPE_ATAPI 1
	union {
		struct {
			int scbus;  /* -1 if wildcard */
			int target; /* -1 if wildcard */
			int lun;    /* -1 if wildcard */
		} scsi;
		struct {
			int atbus;  /* -1 if wildcard */
			int drive;  /* -1 if wildcard */
		} atapi;
	} addr;
};


struct uscsi_sense {
	int asc;		/* Additional sense code */
	int ascq;		/* Additional sense code quality */
	int skey_valid;		/* sense key valid */
	int sense_key;		/* sense key; interpret on (asc, ascq) pair */
};


struct uscsi_dev {
	char	*dev_name;
	int	 fhandle;
	void	*devhandle;	/* for if a fhandle is not enough */
};


/* uscsi_sense.c */
extern char *uscsi_decode_sense(void *sinfo, int flag);
extern void  uscsi_print_sense(const char *name, u_char *req_cmd,
	int req_cmdlen, u_char *req_sense, int req_senselen_used,
	int verbosity);


/* scsi_subr.c */
extern int  uscsi_open(struct uscsi_dev *);
extern int  uscsi_close(struct uscsi_dev *);
extern int  uscsi_command(int flags, struct uscsi_dev *disc,
	void *cmd, size_t cmdlen, void *data, size_t datalen,
	uint32_t timeout, struct uscsi_sense *uscsi_sense);
extern int  uscsi_check_for_scsi(struct uscsi_dev *);
extern int  uscsi_identify(struct uscsi_dev *, struct uscsi_addr *saddr);

extern int  uscsi_mode_sense(struct uscsi_dev *, u_int8_t, u_int8_t,
	void *, size_t);
extern int  uscsi_mode_select(struct uscsi_dev *, u_int8_t, void *, size_t);
extern int  uscsi_request_sense(struct uscsi_dev *, void *, size_t);


#endif	/* _SCSILIB_H_ */

