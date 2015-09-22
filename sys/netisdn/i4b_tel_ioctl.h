/*
 * Copyright (c) 1997, 2000  Hellmuth Michaelis. All rights reserved.
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
 *---------------------------------------------------------------------------
 *
 *	i4b_tel_ioctl.h telephony interface ioctls
 *	------------------------------------------
 *
 *	$Id: i4b_tel_ioctl.h,v 1.4.140.1 2015/09/22 12:06:12 skrll Exp $
 *
 * $FreeBSD$
 *
 *      last edit-date: [Wed Jan 12 15:47:11 2000]
 *
 *---------------------------------------------------------------------------*/

#ifndef _NETISDN_I4B_TEL_IOCTL_H_
#define _NETISDN_I4B_TEL_IOCTL_H_

#include <sys/ioccom.h>

/*===========================================================================*
 *	/dev/i4btel<n> devices (audio data)
 *===========================================================================*/

/* supported audio format conversions */

#define CVT_NONE	0		/* no A-law/mu-law conversion     */
#define CVT_ALAW2ULAW	1		/* ISDN line: A-law, user: mu-law */
#define CVT_ULAW2ALAW	2		/* ISDN line: mu-law, user: A-law */

/*---------------------------------------------------------------------------*
 *	get / set audio format
 *---------------------------------------------------------------------------*/

#define	I4B_TEL_GETAUDIOFMT	_IOR('A', 0, int)
#define	I4B_TEL_SETAUDIOFMT	_IOW('A', 1, int)
#define	I4B_TEL_EMPTYINPUTQUEUE	_IOW('A', 2, int)

/*---------------------------------------------------------------------------*
 *	request version and release info from kernel part
 *---------------------------------------------------------------------------*/

#define I4B_TEL_VR_REQ		_IOR('A', 3, msg_vr_req_t)

/*---------------------------------------------------------------------------*
 *	send tones out of the tel interface
 *---------------------------------------------------------------------------*/

#define I4B_TEL_MAXTONES 32

struct i4b_tel_tones {
	int frequency[I4B_TEL_MAXTONES];
	int duration[I4B_TEL_MAXTONES];
};

#define I4B_TEL_TONES		_IOR('A', 4, struct i4b_tel_tones)

/*===========================================================================*
 *	/dev/i4bteld<n> devices (dialer interface)
 *===========================================================================*/

/* dialer commands */

#define CMD_DIAL        'D'     /* dial the following number string */
#define CMD_HUP         'H'     /* hangup */

/* dialer responses */

#define RSP_CONN        '0'     /* connect */
#define RSP_BUSY        '1'     /* busy */
#define RSP_HUP         '2'     /* hangup */
#define RSP_NOA         '3'     /* no answer */

#endif /* !_NETISDN_I4B_TEL_IOCTL_H_ */
