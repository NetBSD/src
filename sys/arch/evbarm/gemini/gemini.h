/*	$NetBSD: gemini.h,v 1.1.2.1 2009/01/19 13:16:04 skrll Exp $	*/

/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas <matt@3am-software.com>
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

#ifndef _EVBARM_GEMINI_GEMINI_H
#define _EVBARM_GEMINI_GEMINI_H

/*
 * sanity check opt_gemini.h
 */
#include "opt_gemini.h"
#if !defined(GEMINI)
# error must define GEMINI to use gemini.h
#endif
#if !defined(GEMINI_SINGLE) && !defined(GEMINI_MASTER) && !defined(GEMINI_SLAVE)
# error must define one of GEMINI_SINGLE, GEMINI_MASTER, or GEMINI_SLAVE
#endif
#if defined(GEMINI_SINGLE)
# if defined(GEMINI_MASTER) || defined(GEMINI_SLAVE)
#  error GEMINI_SINGLE can not define either GEMINI_MASTER or GEMINI_SLAVE
# endif
#else
# if defined(GEMINI_MASTER) && defined(GEMINI_SLAVE)
#  error can not define both GEMINI_MASTER and GEMINI_SLAVE
# endif
#endif

#include <machine/vmparam.h>
#include <arm/gemini/gemini_reg.h>


/*
 * Kernel VM space: 192MB at KERNEL_VM_BASE
 */
#define	KERNEL_VM_BASE		((KERNEL_BASE + 0x01000000) & ~(0x400000-1))
#define KERNEL_VM_SIZE		0x0C000000

/*
 * We devmap IO starting at KERNEL_VM_BASE + KERNEL_VM_SIZE
 */
#define	GEMINI_KERNEL_IO_VBASE	(KERNEL_VM_BASE + KERNEL_VM_SIZE)
#define	GEMINI_GLOBAL_VBASE	GEMINI_KERNEL_IO_VBASE
#define	GEMINI_WATCHDOG_VBASE	(GEMINI_GLOBAL_VBASE   + L1_S_SIZE)
#define	GEMINI_UART_VBASE	(GEMINI_WATCHDOG_VBASE + L1_S_SIZE)
#define	GEMINI_LPCHC_VBASE	(GEMINI_UART_VBASE     + L1_S_SIZE)
#define	GEMINI_LPCIO_VBASE	(GEMINI_LPCHC_VBASE    + L1_S_SIZE)
#define	GEMINI_TIMER_VBASE	(GEMINI_LPCIO_VBASE    + L1_S_SIZE)
#define	GEMINI_DRAMC_VBASE	(GEMINI_TIMER_VBASE    + L1_S_SIZE)

/*
 * mapping of physical RAM
 */
#define	GEMINI_RAMDISK_VBASE	(GEMINI_DRAMC_VBASE    + L1_S_SIZE)
#define	GEMINI_RAMDISK_PBASE	0x00800000
#define	GEMINI_RAMDISK_SIZE	0x00300000
#define	GEMINI_RAMDISK_PEND	(GEMINI_RAMDISK_PBASE + GEMINI_RAMDISK_SIZE)

#define	GEMINI_IPMQ_VBASE 					\
	((GEMINI_RAMDISK_VBASE + GEMINI_RAMDISK_SIZE		\
	  + (L1_S_SIZE * 4) - 1) & ~((L1_S_SIZE * 4) - 1))
		/* round up for l2pt alignment */
#ifdef GEMINI_SLAVE
# define	GEMINI_IPMQ_PBASE	(GEMINI_RAMDISK_PEND + (GEMINI_BUSBASE * 1024 * 1024))
#else
# define	GEMINI_IPMQ_PBASE	GEMINI_RAMDISK_PEND
#endif
#if 0
# define	GEMINI_IPMQ_SIZE	(2 * sizeof(ipm_queue_t))
#else
# define	GEMINI_IPMQ_SIZE	(2 * 4096)
#endif
#define	GEMINI_IPMQ_PEND	(GEMINI_IPMQ_PBASE + GEMINI_IPMQ_SIZE)

/*
 * reserve physical RAM, as needed
 *
 * NOTE: the RAM used for the IPM queues is owned by the MASTER
 * so MASTER needs to reserve those pages from VM; the slave does not.
 * Hence, GEMINI_RAM_RESV_PEND is adjusted for the MASTER but not the SLAVE.
 */
#define GEMINI_RAM_RESV_PBASE	0
#define GEMINI_RAM_RESV_PEND	0
#if defined(MEMORY_DISK_DYNAMIC)
# undef  GEMINI_RAM_RESV_PBASE
# undef  GEMINI_RAM_RESV_PEND
# define GEMINI_RAM_RESV_PBASE	GEMINI_RAMDISK_PBASE
# define GEMINI_RAM_RESV_PEND	GEMINI_RAMDISK_PEND
#endif
#if (NGEMINIIPM > 0) && !defined(GEMINI_SLAVE)
# if (NGEMINIIPM > 1)
#  error unexpected NGEMINIIPM > 1
# endif
# if (GEMINI_RAM_RESV_PBASE == 0)
#  undef  GEMINI_RAM_RESV_PBASE
#  define GEMINI_RAM_RESV_PBASE	GEMINI_IPMQ_PBASE
# endif
# undef  GEMINI_RAM_RESV_PEND
# define GEMINI_RAM_RESV_PEND	GEMINI_IPMQ_PEND
#endif

/*
 * to map all of memory, including RAM owned by both core.
 * we start at nice round vbase to simplify conversion
 * from VA to PA and back
 */
#define GEMINI_ALLMEM_PBASE	0
#define GEMINI_ALLMEM_VBASE	0xf0000000
#define GEMINI_ALLMEM_SIZE	128		/* units of MB */


#endif /* _EVBARM_GEMINI_GEMINI_H */
