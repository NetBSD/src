/*	$NetBSD: fwiso_ioctl.h,v 1.1 2002/12/04 00:28:44 haya Exp $	*/

/*-
 * Copyright (c) 2001
 *      HAYAKAWA Koichi.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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


#ifndef _DEV_IEEE1394_FWISO_IOCTL_H_
#define _DEV_IEEE1394_FWISO_IOCTL_H_

#define FWISO_IFNAMESIZ 15
struct fwiso_if {
	char fi_name[FWISO_IFNAMESIZ];
};

#define FWISOSETIF		_IOW('I', 1, struct fwiso_if)
#define FWISOGETIF		_IOWR('I', 2, struct fwiso_if)
#define FWISOSETMODE		_IOW('I', 3, int)
#define FWISOGETMODE		_IOR('I', 4, int)
#define FWISOSETCHANNEL		_IOW('I', 5, int)
#define FWISOGETCHANNEL		_IOR('I', 6, int)
#define FWISOSETTAG		_IOW('I', 7, int)
#define FWISOGETTAG		_IOR('I', 8, int)
#define FWISOSETSPD		_IOW('I', 9, int)
#define FWISOGETSPD		_IOR('I', 10, int)

/* for FWISOSETMODE and FWISOGETMODE */
#define FWISO_MODE_RAW		0
#define FWISO_MODE_DV		1 /* SD DV format without header */
#define FWISO_MODE_DV_RAW	2 /* SD DV format with header */
#define FWISO_MODE_MPEG2TS	3
#define FWISO_MODE_MAX		4

/* for FWISOSETCHANNEL and FWISOGETCHANNEL */
#define FWISO_CHANNEL_ANY	64

/* for FWISOSETTAG and FWISOGETTAG */
#define FWISO_TAG0		0x01
#define FWISO_TAG1		0x02
#define FWISO_TAG2		0x04
#define FWISO_TAG3		0x08

#define FWISO_CIF		FWISO_TAG1
#define FWISO_GASP		FWISO_TAG3

/*
 * for FWISOSETSPD and FWISOGETSPD.  Same as IEEE1394_SPD_S* in
 * ieee1394reg.h.
 */
#define FWISO_SPD_S100		0
#define FWISO_SPD_S200		1
#define FWISO_SPD_S400		2
#define FWISO_SPD_S800		3
#define FWISO_SPD_S1600		4
#define FWISO_SPD_S3200		5

/* header attached when FWISO_MODE_RAW is chosen */
struct fwiso_header {
	u_int32_t fh_header[4];
	/* actual header and data below */
};

#define fh_timestamp	fh_header[0]
#define fh_speed	fh_header[1] /* FWISO_SPD_S* */
#define fh_capture_size	fh_header[2] /* link layer header + data */
#define fh_iso_header	fh_header[3] /* link layer header */

#endif /*_DEV_IEEE1394_FWISO_IOCTL_H_ */

