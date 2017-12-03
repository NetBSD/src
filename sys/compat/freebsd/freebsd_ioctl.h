/*	$NetBSD: freebsd_ioctl.h,v 1.6.78.1 2017/12/03 11:36:53 jdolecek Exp $	*/

/*
 * Copyright (c) 1995 Frank van der Linden
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed for the NetBSD Project
 *      by Frank van der Linden
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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

#ifndef _FREEBSD_IOCTL_H
#define _FREEBSD_IOCTL_H

#include <sys/ioccom.h>

#define	FREEBSD_IOCGROUP(x)	(((x) >> 8) & 0xff)

#define FREEBSD_IOC_DIRMASK	0xe0000000
#define FREEBSD_IOC_VOID	0x20000000
#define FREEBSD_IOC_OUT		0x40000000
#define FREEBSD_IOC_IN		0x80000000
#define FREEBSD_IOC_INOUT	(FREEBSD_IOC_IN|FREEBSD_IOC_OUT)

#define FREEBSD_SIOCALIFADDR	 _IOW('i', 27, struct if_laddrreq)
#define FREEBSD_SIOCGLIFADDR	_IOWR('i', 28, struct if_laddrreq)
#define FREEBSD_SIOCDLIFADDR	 _IOW('i', 29, struct if_laddrreq)
#define FREEBSD_SIOCGIFMTU	_IOWR('i', 51, struct oifreq)
#define FREEBSD_SIOCSIFMTU	 _IOW('i', 52, struct oifreq)

#endif /* !_FREEBSD_IOCTL_H */
