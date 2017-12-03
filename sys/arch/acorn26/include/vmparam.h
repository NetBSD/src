/*	$NetBSD: vmparam.h,v 1.12.12.1 2017/12/03 11:35:44 jdolecek Exp $	*/

/*
 * Copyright (c) 1988 The Regents of the University of California.
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
 * 3. Neither the name of the University nor the names of its contributors
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
 */

#ifndef	_ARM26_VMPARAM_H_
#define	_ARM26_VMPARAM_H_

#define __USE_TOPDOWN_VM

/*
 * Page size on the 26-bit ARM CPUs is not variable in the traditional
 * sense.  We override the PAGE_* definitions to compile-time constants.
 */
#define	PAGE_SHIFT	15
#define	PAGE_SIZE	(1 << PAGE_SHIFT)
#define	PAGE_MASK	(PAGE_SIZE - 1)

#define	USRSTACK	VM_MAXUSER_ADDRESS

#define	MAXTSIZ		(0x007f8000)		/* max text size */
#ifndef	DFLDSIZ
#define	DFLDSIZ		(0x01800000)		/* initial data size limit */
#endif
#ifndef	MAXDSIZ
#define	MAXDSIZ		(0x01800000)		/* max data size */
#endif
#ifndef	DFLSSIZ
#define	DFLSSIZ		(512*1024)		/* initial stack size limit */
#endif
#ifndef	MAXSSIZ
#define	MAXSSIZ		(0x00200000)		/* max stack size */
#endif

/*
 * Override the default pager_map size, there's not enough KVA.
 */
#define PAGER_MAP_DEFAULT_SIZE          (1 * 1024 * 1024)

/*
 * Mach derived constants
 */

/* Need to link some of these with some in param.h */

/*
 * These specify the range of virtual pages available to user
 * processes and to the kernel.  This is a bit of a delicate balancing
 * act, as we've only got 32Mb between them.  There should probably be
 * an option to have separate kernel and user spaces to ease this.
 */

#define KVM_SIZE		0x00800000 /* 8Mb */

/* User VM range */
#define	VM_MIN_ADDRESS		((vaddr_t)0x00008000)
#define	VM_MAX_ADDRESS		((vaddr_t)0x02000000 - KVM_SIZE)
#define	VM_MAXUSER_ADDRESS	VM_MAX_ADDRESS

/* Kernel VM range */
#define	VM_MIN_KERNEL_ADDRESS	VM_MAX_ADDRESS
#define	VM_MAX_KERNEL_ADDRESS	((vaddr_t)0x02000000)
#define	VM_MAXKERN_ADDRESS	VM_MAX_KERNEL_ADDRESS

/* Physical memory parameters */

#define VM_PHYSSEG_MAX		3

#define VM_PHYSSEG_STRAT	VM_PSTRAT_BIGFIRST

#define VM_NFREELIST		2
#define VM_FREELIST_LOW		1 /* DMA-able memory (bottom 512k phys) */
#define VM_FREELIST_DEFAULT	0 /* The rest */

#endif /* _ARM26_VMPARAM_H_ */

/* End of vmparam.h */
