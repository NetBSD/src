/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
 *
 * Slightly modified for the VAX port /IC
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
 *	from: @(#)vmparam.h	5.9 (Berkeley) 5/12/91
 *	$Id: vmparam.h,v 1.2 1994/08/16 23:41:57 ragge Exp $
 */
#ifndef ASSEMBLER
#include <vm/vm_param.h>
#endif

/*
 * Machine dependent constants for VAX.
 */

/*
 * Virtual address space arrangement. On 386, both user and kernel
 * share the address space, not unlike the vax.
 * USRTEXT is the start of the user text/data space, while USRSTACK
 * is the top (end) of the user stack. Immediately above the user stack
 * resides the user structure, which is UPAGES long and contains the
 * kernel stack.
 *
 */

#define	USRTEXT		0
#define	USRSTACK	0x80000000

/*
 * Virtual memory related constants, all in bytes
 */

#ifndef MAXTSIZ
#define	MAXTSIZ		(4*1024*1024)		/* max text size */
#endif
#ifndef MAXDSIZ
#define	MAXDSIZ		(10*1024*1024)		/* max data size */
#endif
#ifndef	MAXSSIZ
#define	MAXSSIZ		(2*1024*1024)		/* max stack size */
#endif
#ifndef DFLDSIZ
#define	DFLDSIZ		(4*1024*1024)		/* initial data size limit */
#endif
#ifndef	DFLSSIZ
#define	DFLSSIZ		(512*1024)		/* initial stack size limit */
#endif

/* (We think) the page table will only need to grow this much */
#define VAX_MAX_PT_SIZE ((MAXTSIZ+MAXDSIZ+MAXSSIZ)>>7)
/* XXX Text size is already set to a predefined size, why alloc 
       more page tables for it than needed??? */

/*
 * Default sizes of swap allocation chunks (see dmap.h).
 * The actual values may be changed in vminit() based on MAXDSIZ.
 * With MAXDSIZ of 16Mb and NDMAP of 38, dmmax will be 1024.
 */

#define	DMMIN	32			/* smallest swap allocation */
#define	DMMAX	4096			/* largest potential swap allocation */
#define	DMTEXT	1024			/* swap allocation for text */

/* 
 * Size of shared memory map
 */

#ifndef SHMMAXPGS
#define SHMMAXPGS	64		/* XXXX should be 1024 */
#endif

/*
 * Sizes of the system and user portions of the system page table.
 */
/*

#define	SYSPTSIZE 	(2*NPTEPG)
#define	USRPTSIZE 	(2*NPTEPG)

*/
/*
 * Size of User Raw I/O map
 */
/* #define	USRIOSIZE 	300 */

/*
 * The size of the clock loop.
 */

/* #define	LOOPPAGES	(maxfree - firstfree) */

/*
 * The time for a process to be blocked before being very swappable.
 * This is a number of seconds which the system takes as being a non-trivial
 * amount of real time.  You probably shouldn't change this;
 * it is used in subtle ways (fractions and multiples of it are, that is, like
 * half of a ``long time'', almost a long time, etc.)
 * It is related to human patience and other factors which don't really
 * change over time.
 */

#define	MAXSLP 		20

/*
 * A swapped in process is given a small amount of core without being bothered
 * by the page replacement algorithm.  Basically this says that if you are
 * swapped in you deserve some resources.  We protect the last SAFERSS
 * pages against paging and will just swap you out rather than paging you.
 * Note that each process has at least UPAGES+CLSIZE pages which are not
 * paged anyways (this is currently 8+2=10 pages or 5k bytes), so this
 * number just means a swapped in process is given around 25k bytes.
 * Just for fun: current memory prices are 4600$ a megabyte on VAX (4/22/81),
 * so we loan each swapped in process memory worth 100$, or just admit
 * that we don't consider it worthwhile and swap it out to disk which costs
 * $30/mb or about $0.75.
 */

#define	SAFERSS		8		/* nominal ``small'' resident set size
					   protected against replacement */

/*
 * DISKRPM is used to estimate the number of paging i/o operations
 * which one can expect from a single disk controller.
 */

/* #define	DISKRPM		60 */

/*
 * Klustering constants.  Klustering is the gathering
 * of pages together for pagein/pageout, while clustering
 * is the treatment of hardware page size as though it were
 * larger than it really is.
 *
 * KLMAX gives maximum cluster size in CLSIZE page (cluster-page)
 * units.  Note that KLMAX*CLSIZE must be <= DMMIN in dmap.h.
 */

/*#define	KLMAX	(4/CLSIZE)      /*                              */
/*#define	KLSEQL	(2/CLSIZE)	/* in klust if vadvise(VA_SEQL) */
/*#define	KLIN	(4/CLSIZE)	/* default data/stack in klust  */
/*#define	KLTXT	(4/CLSIZE)	/* default text in klust        */
/*#define	KLOUT	(4/CLSIZE)      /*                              */

/*
 * KLSDIST is the advance or retard of the fifo reclaim for sequential
 * processes data space.
 */

/*#define	KLSDIST	3	    /* klusters advance/retard for seq. fifo */

/*
 * Paging thresholds (see vm_sched.c).
 * Strategy of 1/19/85:
 *	lotsfree is 512k bytes, but at most 1/4 of memory
 *	desfree is 200k bytes, but at most 1/8 of memory
 *	minfree is 64k bytes, but at most 1/2 of desfree
 */

#define	LOTSFREE	(512 * 1024)
#define	LOTSFREEFRACT	4
#define	DESFREE		(200 * 1024)
#define	DESFREEFRACT	8
#define	MINFREE		(64 * 1024)
#define	MINFREEFRACT	2

/*
 * There are two clock hands, initially separated by HANDSPREAD bytes
 * (but at most all of user memory).  The amount of time to reclaim
 * a page once the pageout process examines it increases with this
 * distance and decreases as the scan rate rises.
 */

#define	HANDSPREAD	(2 * 1024 * 1024)

/*
 * The number of times per second to recompute the desired paging rate
 * and poke the pagedaemon.
 */

#define	RATETOSCHEDPAGING	4

/*
 * Believed threshold (in megabytes) for which interleaved
 * swapping area is desirable.
 */

#define	LOTSOFMEM	2

#define	mapin(pte, v, pfnum, prot) \
	{(*(int *)(pte) = ((pfnum)<<PGSHIFT) | (prot)) ; }

/*
 * Mach derived constants
 */

/* # of kernel PT pages */

#define VM_KERNEL_PT_PAGES	2*1024      /* XXX: SYSPTSIZE */
                                            /* Will give this # times 64 */
                                            /* kilobytes virtual memory */

/* user/kernel map constants */
#define VM_MIN_ADDRESS		((vm_offset_t)0)
#define VM_MAXUSER_ADDRESS	((vm_offset_t)0x80000000)
#define VM_MAX_ADDRESS		((vm_offset_t)0xC0000000)
#define VM_MIN_KERNEL_ADDRESS	((vm_offset_t)0x80000000)
#define VM_MAX_KERNEL_ADDRESS	((vm_offset_t)(VM_MIN_KERNEL_ADDRESS+\
				 (VM_KERNEL_PT_PAGES*0x10000)))

/* virtual sizes (bytes) for various kernel submaps */
#define VM_MBUF_SIZE		(NMBCLUSTERS*MCLBYTES)
#define VM_KMEM_SIZE		(NKMEMCLUSTERS*CLBYTES)
#define VM_PHYS_SIZE		(USRIOSIZE*CLBYTES)

/* pcb base */
#define	pcbb(p)		((u_int)(p)->p_addr)

