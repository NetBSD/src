/*	$NetBSD: param.h,v 1.12 2001/10/07 12:45:03 bjh21 Exp $	*/

/*
 * Copyright (c) 1994,1995 Mark Brinicombe.
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
 *	This product includes software developed by the RiscBSD team.
 * 4. The name "RiscBSD" nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY RISCBSD ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL RISCBSD OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef	_ARM26_PARAM_H_
#define	_ARM26_PARAM_H_

#ifdef _KERNEL
#include <machine/cpu.h>  /* XXX why? */
#include <machine/intr.h> /* XXX for spl(9) */
#endif

#define	_MACHINE	arm26
#define	MACHINE		"arm26"
#define	_MACHINE_ARCH	arm26
#define	MACHINE_ARCH	"arm26"

#define POOL_SUBPAGE 4096

#define	PGSHIFT		15		/* LOG2(NBPG) */
#define	NBPG		(1 << PGSHIFT)	/* bytes/page */
#define	PGOFSET		(NBPG-1)	/* byte offset into page */

#define	CLSIZELOG2	0
#define	CLSIZE		(1 << CLSIZELOG2)

/* NOTE: SSIZE, SINCR and UPAGES must be multiples of CLSIZE */
#define SSIZE           1               /* initial stack size/NBPG */
#define SINCR           1               /* increment of stack/NBPG */
#define UPAGES          1               /* pages of u-area */
#define USPACE          (UPAGES * NBPG) /* total size of u-area */

#ifndef MSGBUFSIZE
#define MSGBUFSIZE	NBPG		/* default message buffer size */
#endif

#ifndef NMBCLUSTERS
#ifdef GATEWAY
#define	NMBCLUSTERS	512		/* map size, max cluster allocation */
#else
#define	NMBCLUSTERS	256		/* map size, max cluster allocation */
#endif
#endif

/*
 * Defaults for lower- and upper-bounds for the kmem_map page count.
 * Can be overridden by kernel config options.
 */
#define NKMEMPAGES_MIN_DEFAULT 0
#define NKMEMPAGES_MAX_DEFAULT 32

/*
 * Reduce UBC KVM usage from its default (8Mb when I looked).
 * Also make sure the window size is at least the page size.
 */
#ifndef UBC_WINSHIFT
#define UBC_WINSHIFT PGSHIFT
#endif
#ifndef UBC_NWINS
#define UBC_NWINS 32
#endif

#ifdef _KERNEL
#ifndef _LOCORE
void	delay __P((unsigned));
#define DELAY(x)	delay(x)
#endif
#endif

#include <arm/param.h>

#endif	/* _ARM_PARAM_H_ */
