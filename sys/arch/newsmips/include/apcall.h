/*	$NetBSD: apcall.h,v 1.2.10.1 2002/06/23 17:38:51 jdolecek Exp $	*/

/*-
 * Copyright (C) 1999 SHIMIZU Ryo.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __MACHINE_APCALL_H__
#define __MACHINE_APCALL_H__

#include <machine/apbus.h>

#define APCALL (*(_sip->apbsi_call))

#define	APCALL_EXIT		1
#define	APCALL_READ		3
#define	APCALL_WRITE		4
#define	APCALL_OPEN		5
#define	APCALL_CLOSE		6
#define	APCALL_LSEEK		19
#define	APCALL_IOCTL		54
#define	APCALL_GETENV		1004
#define	APCALL_FLUSHCACHE	1006
#define	APCALL_GETTIMEOFDAY	1012

#define	APIOCEJECT		2003	/* eject floppy */
#define	APIOCGIFHWADDR		9200	/* get hardware address */

#define	apcall_exit(a)		APCALL(APCALL_EXIT,(a))
#define	apcall_read(a,b,c)	APCALL(APCALL_READ,(a),(b),(c))
#define	apcall_write(a,b,c)	APCALL(APCALL_WRITE,(a),(b),(c))
#define	apcall_open(a,b)	APCALL(APCALL_OPEN,(a),(b))
#define	apcall_close(a)		APCALL(APCALL_CLOSE,(a))
#define	apcall_lseek(a,b,c)	APCALL(APCALL_LSEEK,(a),(b),(c))
#define	apcall_ioctl(a,b,c)	APCALL(APCALL_IOCTL,(a),(b),(c))
#define	apcall_getenv(a)	APCALL(APCALL_GETENV,(a))
#define	apcall_flushcache()	APCALL(APCALL_FLUSHCACHE)
#define	apcall_gettimeofday(a)	APCALL(APCALL_GETTIMEOFDAY,(a))

#endif	/* !__MACHINE_APCALL_H__ */
