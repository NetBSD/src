/* $NetBSD: param.h,v 1.1.18.2 2017/12/03 11:36:34 jdolecek Exp $ */

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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

#ifndef	_OR1K_PARAM_H_
#define	_OR1K_PARAM_H_

/*
 * Machine dependent constants for all OpenRISC processors
 */

/*
 * For KERNEL code:
 *	MACHINE must be defined by the individual port.  This is so that
 *	uname returns the correct thing, etc.
 *
 * For non-KERNEL code:
 *	If ELF, MACHINE and MACHINE_ARCH are forced to "or1k/or1k".
 */

#define	_MACHINE_ARCH	or1k
#define	MACHINE_ARCH	"or1k"
#define	_MACHINE	or1k
#define	MACHINE		"or1k"

#define	MID_MACHINE	MID_OR1K

/* OR1K-specific macro to align a stack pointer (downwards). */
#define STACK_ALIGNBYTES	(__BIGGEST_ALIGNMENT__ - 1)
#define	ALIGNBYTES32	__BIGGEST_ALIGNMENT__

#define	DEV_BSHIFT	9		/* log2(DEV_BSIZE) */
#define	DEV_BSIZE	(1 << DEV_BSHIFT)
#define	BLKDEV_IOSIZE	2048

#ifndef MAXPHYS
#define	MAXPHYS		65536		/* max I/O transfer size */
#endif

#define NKMEMPAGES_MAX_DEFAULT	(2048UL * 1024 * 1024)
#define NKMEMPAGES_MIN_DEFAULT	(128UL * 1024 * 1024)

#define PGSHIFT		13
#define	NBPG		(1 << PGSHIFT)
#define PGOFSET		(NBPG - 1)

/*
 * Constants related to network buffer management.
 * MCLBYTES must be no larger than NBPG (the software page size), and
 * NBPG % MCLBYTES must be zero.
 */
#define	MSIZE		512		/* size of an mbuf */

#ifndef MCLSHIFT
#define	MCLSHIFT	11		/* convert bytes to m_buf clusters */
					/* 2K cluster can hold Ether frame */
#endif	/* MCLSHIFT */

#define	MCLBYTES	(1 << MCLSHIFT)	/* size of a m_buf cluster */

#ifdef _KERNEL
void delay(unsigned long);
#define	DELAY(x)	delay(x)
#endif

#endif /* _OR1K_PARAM_H_ */
