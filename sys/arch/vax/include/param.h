/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * Modified for VAX 940213/Ragge
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	from: @(#)param.h	5.8 (Berkeley) 6/28/91
 *	$Id: param.h,v 1.2 1994/08/16 23:41:54 ragge Exp $
 */

#ifndef PARAM_H
#define PARAM_H

#include "psl.h"

/*
 * Machine dependent constants for VAX.
 */

#define MACHINE		"vax"
#define MID_MACHINE	MID_VAX
#define UNIX		"vmunix"

/*
 * Round p (pointer or byte index) up to a correctly-aligned value
 * for all data types (int, long, ...).   The result is u_int and
 * must be cast to any desired pointer type.
 */

#define ALIGNBYTES	(sizeof(int) - 1)
#define ALIGN(p)	(((u_int)(p) + ALIGNBYTES) &~ ALIGNBYTES)

#define	PGSHIFT	 9                             /* LOG2(NBPG) */
#define	NBPG     (1<<PGSHIFT)                  /* (1 << PGSHIFT) bytes/page */
#define	PGOFSET	 (NBPG-1)	               /* byte offset into page */
#define	NPTEPG	 (NBPG/(sizeof (struct pte)))

#define	KERNBASE     0x80000000	               /* start of kernel virtual */
#define	BTOPKERNBASE ((u_long)KERNBASE >> PGSHIFT)

#define	DEV_BSHIFT   9		               /* log2(DEV_BSIZE) */
#define	DEV_BSIZE    (1 << DEV_BSHIFT)

#define BLKDEV_IOSIZE 2048
#define	MAXPHYS	      (63 * 1024)     /* max raw I/O transfer size */

#define	CLSIZELOG2    1
#define	CLSIZE	      2

/* NOTE: SSIZE, SINCR and UPAGES must be multiples of CLSIZE */
#define	SSIZE	4		/* initial stack size/NBPG */
#define	SINCR	4		/* increment of stack/NBPG */

#define	UPAGES	16		/* pages of u-area */

/*
 * Constants related to network buffer management.
 * MCLBYTES must be no larger than CLBYTES (the software page size), and,
 * on machines that exchange pages of input or output buffers with mbuf
 * clusters (MAPPED_MBUFS), MCLBYTES must also be an integral multiple
 * of the hardware page size.
 */

#ifndef	MSIZE
#define	MSIZE		128		/* size of an mbuf */
#endif	/* MSIZE */

#ifndef	MCLSHIFT
#define	MCLSHIFT	10		/* convert bytes to m_buf clusters */
#endif	/* MCLSHIFT */
#define	MCLBYTES	(1 << MCLSHIFT)	/* size of an m_buf cluster */
#define	MCLOFSET	(MCLBYTES - 1)	/* offset within an m_buf cluster */

#ifndef NMBCLUSTERS
#ifdef GATEWAY
#define	NMBCLUSTERS	512		/* map size, max cluster allocation */
#else
#define	NMBCLUSTERS	256		/* map size, max cluster allocation */
#endif	/* GATEWAY */
#endif	/* NMBCLUSTERS */

/*
 * Size of kernel malloc arena in CLBYTES-sized logical pages
 */ 

#ifndef NKMEMCLUSTERS
#define	NKMEMCLUSTERS	(512*1024/CLBYTES)
#endif

/*
 * Some macros for units conversion
 */

/* Core clicks (512 bytes) to segments and vice versa */

#define	ctos(x)	(x)
#define	stoc(x)	(x)

/* Core clicks (512 bytes) to disk blocks */

#define	ctod(x)	((x)<<(PGSHIFT-DEV_BSHIFT))
#define	dtoc(x)	((x)>>(PGSHIFT-DEV_BSHIFT))
#define	dtob(x)	((x)<<DEV_BSHIFT)

/* clicks to bytes */

#define	ctob(x)	((x)<<PGSHIFT)

/* bytes to clicks */

#define	btoc(x)	(((unsigned)(x)+(NBPG-1))>>PGSHIFT)

#define	btodb(bytes)	 		/* calculates (bytes / DEV_BSIZE) */ \
	((unsigned)(bytes) >> DEV_BSHIFT)
#define	dbtob(db)			/* calculates (db * DEV_BSIZE) */ \
	((unsigned)(db) << DEV_BSHIFT)

/*
 * Map a ``block device block'' to a file system block.
 * This should be device dependent, and will be if we
 * add an entry to cdevsw/bdevsw for that purpose.
 * For now though just use DEV_BSIZE.
 */

#define	bdbtofsb(bn)	((bn) / (BLKDEV_IOSIZE/DEV_BSIZE))

/*
 * Mach derived conversion macros
 */

#define vax_round_pdr(x)	((((unsigned)(x)) + NBPDR - 1) & ~(NBPDR-1))
#define vax_trunc_pdr(x)	((unsigned)(x) & ~(NBPDR-1))
#define vax_round_page(x)	((((unsigned)(x)) + NBPG - 1) & ~(NBPG-1))
#define vax_trunc_page(x)	((unsigned)(x) & ~(NBPG-1))
#define vax_btod(x)		((unsigned)(x) >> PDRSHIFT)
#define vax_dtob(x)		((unsigned)(x) << PDRSHIFT)
#define vax_btop(x)		((unsigned)(x) >> PGSHIFT)
#define vax_ptob(x)		((unsigned)(x) << PGSHIFT)

#define DELAY(x)		kern_delay(x)

/*
#define _spl(s) \
({ \
        register int _spl_r; \
\
        asm __volatile ("mfpr $18,%0; mtpr $%1,$18; " : \
                "&=r" (_spl_r) : "ir" (s)); \
        _spl_r; \
})
*/

/* spl0 requires checking for software interrupts */


#define splsoftclock()  _spl(PSL2IPL(PSL_IPL08))
#define splnet()        _spl(PSL2IPL(PSL_IPL0C))
#define spl4()          _spl(PSL2IPL(PSL_IPL14))
#define spltty()        _spl(PSL2IPL(PSL_IPL15))
#define splbio()        _spl(PSL2IPL(PSL_IPL15))
#define splimp()        _spl(PSL2IPL(PSL_IPL16))
#define splclock()      _spl(PSL2IPL(PSL_IPL18))
#define splhigh()       _spl(PSL2IPL(PSL_IPL1F))
#define	splstatclock()	splclock()
/*
#define splvm()         _spl(PSL2IPL(PSL_IPL14))
#define splsched()      _spl(PSL2IPL(PSL_IPL14))
*/

/* watch out for side effects */
#define splx(s)         (s ? _spl(s) : spl0())

#define	ovbcopy(x,y,z)	bcopy(x,y,z)	/* This should work i hope... */

#endif
