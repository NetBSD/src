/*	$NetBSD: stvar.h,v 1.1 2001/05/04 07:48:57 bouyer Exp $ */

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum.
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
 * major changes by Julian Elischer (julian@jules.dialix.oz.au) May 1993
 *
 * A lot of rewhacking done by mjacob (mjacob@nas.nasa.gov).
 */

#include "rnd.h"
#if NRND > 0
#include <sys/rnd.h>
#endif

#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>

struct modes {
	u_int quirks;			/* same definitions as in quirkdata */
	int blksize;
	u_int8_t density;
};

struct quirkdata {
	u_int quirks;
#define	ST_Q_FORCE_BLKSIZE	0x0001
#define	ST_Q_SENSE_HELP		0x0002	/* must do READ for good MODE SENSE */
#define	ST_Q_IGNORE_LOADS	0x0004
#define	ST_Q_BLKSIZE		0x0008	/* variable-block media_blksize > 0 */
#define	ST_Q_UNIMODAL		0x0010	/* unimode drive rejects mode select */
#define	ST_Q_NOPREVENT		0x0020	/* does not support PREVENT */
#define	ST_Q_ERASE_NOIMM	0x0040	/* drive rejects ERASE/w Immed bit */
	u_int page_0_size;
#define	MAX_PAGE_0_SIZE	64
	struct modes modes[4];
};

struct st_quirk_inquiry_pattern {
	struct scsipi_inquiry_pattern pattern;
	struct quirkdata quirkdata;
};

struct st_softc {
	struct device sc_dev;
/*--------------------present operating parameters, flags etc.---------------*/
	int flags;		/* see below                         */
	u_int quirks;		/* quirks for the open mode          */
	int blksize;		/* blksize we are using              */
	u_int8_t density;	/* present density                   */
	u_int page_0_size;	/* size of page 0 data		     */
	u_int last_dsty;	/* last density opened               */
	short mt_resid;		/* last (short) resid                */
	short mt_erreg;		/* last error (sense key) seen       */
#define	mt_key	mt_erreg
	u_int8_t asc;		/* last asc code seen		     */
	u_int8_t ascq;		/* last asc code seen		     */
/*--------------------device/scsi parameters---------------------------------*/
	struct scsipi_periph *sc_periph;/* our link to the adpter etc.       */
/*--------------------parameters reported by the device ---------------------*/
	int blkmin;		/* min blk size                       */
	int blkmax;		/* max blk size                       */
	struct quirkdata *quirkdata;	/* if we have a rogue entry          */
/*--------------------parameters reported by the device for this media-------*/
	u_long numblks;		/* nominal blocks capacity            */
	int media_blksize;	/* 0 if not ST_FIXEDBLOCKS            */
	u_int8_t media_density;	/* this is what it said when asked    */
/*--------------------quirks for the whole drive-----------------------------*/
	u_int drive_quirks;	/* quirks of this drive               */
/*--------------------How we should set up when opening each minor device----*/
	struct modes modes[4];	/* plus more for each mode            */
	u_int8_t  modeflags[4];	/* flags for the modes                */
#define DENSITY_SET_BY_USER	0x01
#define DENSITY_SET_BY_QUIRK	0x02
#define BLKSIZE_SET_BY_USER	0x04
#define BLKSIZE_SET_BY_QUIRK	0x08
/*--------------------storage for sense data returned by the drive-----------*/
	u_char sense_data[MAX_PAGE_0_SIZE];	/*
						 * additional sense data needed
						 * for mode sense/select.
						 */
	struct buf_queue buf_queue;	/* the queue of pending IO */
					/* operations */
#if NRND > 0
	rndsource_element_t	rnd_source;
#endif
};


void	stattach __P((struct device *, struct device *, void *));

extern struct cfdriver st_cd;
