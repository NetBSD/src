/*	$NetBSD: linux_cdrom.h,v 1.3.14.1 1999/11/15 00:40:04 fvdl Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Eric Haszlakiewicz.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

#ifndef _LINUX_CDROM_H
#define _LINUX_CDROM_H

#define LINUX_CDROMPAUSE	0x5301
#define LINUX_CDROMRESUME	0x5302
#define LINUX_CDROMPLAYMSF	0x5303	/* (struct linux_cdrom_msf) */
#define LINUX_CDROMPLAYTRKIND	0x5304	/* (struct linux_cdrom_ti) */
#define LINUX_CDROMREADTOCHDR	0x5305	/* (struct linux_cdrom_tochdr) */
#define LINUX_CDROMREADTOCENTRY	0x5306	/* (struct linux_cdrom_tocentry) */
#define LINUX_CDROMSTOP		0x5307
#define LINUX_CDROMSTART	0x5308
#define LINUX_CDROMEJECT	0x5309
#define LINUX_CDROMVOLCTRL	0x530a	/* (struct linux_cdrom_volctrl) */
#define LINUX_CDROMSUBCHNL	0x530b	/* (struct linux_cdrom_subchnl) */
#define LINUX_CDROMEJECT_SW	0x530f	/* arg: 0 or 1 */
#define LINUX_CDROMRESET	0x5312
#define LINUX_CDROMVOLREAD	0x5313	/* (struct linux_cdrom_volctrl) */
#define LINUX_CDROMPLAYBLK	0x5317	/* (struct linux_cdrom_blk) */

/* DVD-ROM Specific ioctls */              
#define	LINUX_DVD_READ_STRUCT	0x5390	/* Read structure */
#define	LINUX_DVD_WRITE_STRUCT	0x5391	/* Write structure */ 
#define	LINUX_DVD_AUTH		0x5392	/* Authentication */

struct linux_cdrom_blk {
	unsigned from;
	unsigned short len;
};

struct linux_cdrom_msf {
	u_char	cdmsf_min0;	/* start minute */
	u_char	cdmsf_sec0;	/* start second */
	u_char	cdmsf_frame0;	/* start frame */
	u_char	cdmsf_min1;	/* end minute */
	u_char	cdmsf_sec1;	/* end second */
	u_char	cdmsf_frame1;	/* end frame */
};

struct linux_cdrom_ti {
	u_char	cdti_trk0;	/* start track */
	u_char	cdti_ind0;	/* start index */
	u_char	cdti_trk1;	/* end track */
	u_char	cdti_ind1;	/* end index */
};
 
struct linux_cdrom_tochdr {
	u_char	cdth_trk0;	/* start track */
	u_char	cdth_trk1;	/* end track */
};

struct linux_cdrom_msf0 {
	u_char	minute;
	u_char	second;
	u_char	frame;
};

union linux_cdrom_addr {
	struct	linux_cdrom_msf0 msf;
	int	lba;
};

struct linux_cdrom_tocentry {
	u_char	cdte_track;
	u_char	cdte_adr	:4;
	u_char	cdte_ctrl	:4;
	u_char	cdte_format;
	union	linux_cdrom_addr cdte_addr;
	u_char	cdte_datamode;
};

struct linux_cdrom_subchnl {
	u_char	cdsc_format;
	u_char	cdsc_audiostatus;
	u_char	cdsc_adr:	4;
	u_char	cdsc_ctrl:	4;
	u_char	cdsc_trk;
	u_char	cdsc_ind;
	union	linux_cdrom_addr cdsc_absaddr;
	union	linux_cdrom_addr cdsc_reladdr;
};

struct linux_cdrom_volctrl {
	u_char	channel0;
	u_char	channel1;
	u_char	channel2;
	u_char	channel3;
};

#endif /* !_LINUX_CDROM_H */
