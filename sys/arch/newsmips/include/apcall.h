/*	$NetBSD: apcall.h,v 1.1 1999/12/22 05:53:21 tsubai Exp $	*/

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

#define APCALL		(*(_sip->apbsi_call))

#define	APCALL_EXIT		1
#define	APCALL_READ		3
#define	APCALL_WRITE		4
#define	APCALL_OPEN		5
#define	APCALL_CLOSE		6
#define	APCALL_LSEEK		19
#define	APCALL_IOCTL		54

#define	APCALL_GETBASE		1001
#define	APCALL_BOOT		1002
#define	APCALL_BADADDR		1003
#define	APCALL_GETENV		1004
#define	APCALL_SETENV		1005
#define	APCALL_FLCACHE		1006
#define	APCALL_SLEEP		1007
#define	APCALL_USLEEP		1008
#define	APCALL_PHYSADDR		1009
#define	APCALL_MALLOC		1010
#define	APCALL_FREE		1011
#define	APCALL_NSECTIME		1012
#define	APCALL_IOMAP		1013
#define	APCALL_DMAMAP		1014
#define	APCALL_OPENBFS		1015
#define	APCALL_READBFS		1016
#define	APCALL_CLOSEBFS		1017

#define	APCALL_PRINTF		1101
#define	APCALL_FPRINTF		1103
#define	APCALL_SPRINTF		1104
#define	APCALL_BCOPY		1105
#define	APCALL_BCMP		1106
#define	APCALL_BZERO		1107

#define	APCALL_PANIC		2001
#define	APCALL_READDIPSW	2002
#define	APCALL_STRCPY		2003
#define	APCALL_STRCAT		2004
#define	APCALL_STRCMP		2005
#define	APCALL_STRLEN		2006
#define	APCALL_STRNCPY		2007
#define	APCALL_STRNCAT		2008
#define	APCALL_STRNCMP		2009
#define	APCALL_STRTOL		2010
#define	APCALL_LTOSTR		2011
#define	APCALL_EPRINTF		2012
#define	APCALL_FONTADDR		2013

#define	APIOCGIFHWADDR		9200	/* get hardware address */

#define	apcall_exit(a)		APCALL(APCALL_EXIT,(a))
#define	apcall_read(a,b,c)	APCALL(APCALL_READ,(a),(b),(c))
#define	apcall_write(a,b,c)	APCALL(APCALL_WRITE,(a),(b),(c))
#define	apcall_open(a,b)	APCALL(APCALL_OPEN,(a),(b))
#define	apcall_close(a)		APCALL(APCALL_CLOSE,(a))
#define	apcall_lseek(a,b,c)	APCALL(APCALL_LSEEK,(a),(b),(c))
#define	apcall_ioctl(a,b,c)	APCALL(APCALL_IOCTL,(a),(b),(c))

#define	apcall_getbase(a,b,c)	APCALL(APCALL_GETBASE,(a),(b),(c))
#define	apcall_boot(a)		APCALL(APCALL_BOOT,(a))
#define	apcall_badaddr(a,b)	APCALL(APCALL_BADADDR,(a),(b))
#define	apcall_getenv(a)	APCALL(APCALL_GETENV,(a))
#define	apcall_setenv(a,b)	APCALL(APCALL_SETENV,(a),(b))
#define	apcall_flush_cache()	APCALL(APCALL_FLCACHE)
#define	apcall_sleep(a)		APCALL(APCALL_SLEEP,(a))
#define	apcall_usleep(a)	APCALL(APCALL_USLEEP,(a))
#define	apcall_physaddr(a,b,c)	APCALL(APCALL_PHYSADDR,(a),(b),(c))
#define	apcall_malloc(a)	APCALL(APCALL_MALLOC,(a))
#define	apcall_free(a)		APCALL(APCALL_FREE,(a))
#define	apcall_nsectime(a)	APCALL(APCALL_NSECTIME,(a))
#define	apcall_iomap(a,b)	APCALL(APCALL_IOMAP,(a),(b))
#define	apcall_dmamap(a,b)	APCALL(APCALL_DMAMAP,(a),(b))
#define	apcall_openbfs(a,b)	APCALL(APCALL_OPENBFS,(a),(b))
#define	apcall_readbfs(a,b)	APCALL(APCALL_READBFS,(a),(b))
#define	apcall_closebfs()	APCALL(APCALL_CLOSEBFS)

#define	apcall_bcopy(a,b,c)	APCALL(APCALL_BCOPY,(a),(b),(c))
#define	apcall_bcmp(a,b,c)	APCALL(APCALL_BCMP,(a),(b),(c))
#define	apcall_bzero(a,b)	APCALL(APCALL_BZERO,(a),(b))

#define	apcall_panic(a,b)	APCALL(APCALL_PANIC,(a),(b))
#define	apcall_readdipsw()	APCALL(APCALL_READDIPSW)
#define	apcall_strcpy(a,b)	APCALL(APCALL_STRCPY,(a),(b))
#define	apcall_strcat(a,b)	APCALL(APCALL_STRCAT,(a),(b))
#define	apcall_strcmp(a,b)	APCALL(APCALL_STRCMP,(a),(b))
#define	apcall_strlen(a)	APCALL(APCALL_STRLEN,(a))
#define	apcall_strncpy(a,b,c)	APCALL(APCALL_STRNCPY,(a),(b),(c))
#define	apcall_strncat(a,b,c)	APCALL(APCALL_STRNCAT,(a),(b),(c))
#define	apcall_strncmp(a,b,c)	APCALL(APCALL_STRNCMP,(a),(b),(c))
#define	apcall_getsx(a,b)	APCALL(APCALL_GETSX,(a),(b))
#define	apcall_strtol(a,b,c)	APCALL(APCALL_STRTOL,(a),(b),(c))
#define	apcall_ltostr(a,b,c)	APCALL(APCALL_LTOSTR,(a),(b),(c))
#define	apcall_fontaddr(a,b)	APCALL(APCALL_FONTADDR,(a),(b))

#endif	/* !__MACHINE_APCALL_H__ */
