/*	$NetBSD: xxboot.h,v 1.5 2003/05/23 21:56:37 leo Exp $	*/

/*
 * Copyright (c) 1995 Waldi Ravens
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
 *        This product includes software developed by Waldi Ravens.
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

#define	BIOS		13
#define	Bconin		0x02
#define	Bconout		0x03
#define	Kbshift		0x0b
#define	XBIOS		14
#define	Floprd		0x08
#define	DMAread		0x2a
#define	NVMaccess	0x2e

#define	_membot		0x0432
#define	_memtop		0x0436
#define	flock		0x043e
#define	_bootdev	0x0446
#define	_v_bas_ad	0x044e
#define	_hz_200		0x04ba
#define	_drvbits	0x04c2
#define	_sysbase	0x04f2
#define	pun_ptr		0x0516

#define	daccess		0x8604
#define	dmodus		0x8606
#define	dmahi		0x8609
#define	gpip		0xfa01

#define	rtcrnr		0x8961
#define	rtcdat		0x8963
#define	BOOTPREF	15

#define	idesdh		0xfff00019
#define	idedor		0xfff00005
#define	idedr		0xfff00000
#define	idecr		0xfff0001d
#define	idesr		0xfff0001d
#define	idesc		0xfff00009
#define	idesn		0xfff0000d
#define	idecl		0xfff00011
#define	idech		0xfff00015

/*
 * Boot block format (16 * 512 bytes)
 *
 * - first-stage loader	:  512 bytes
 * - magic number	:    4 bytes
 * - disk pack label	: 1020 bytes
 * - second-stage loader: 6656 bytes
 *
 * The boot code in ROM loads the first-stage loader, which
 * in turn loads the remaining 15 sectors and executes the
 * second-stage loader, which runs at address LOADADDR.
 */
#define	NSEC		15
#define	LBLSZ		1020
#define	BXXSZ		6656

#define	BXXST		(LOADADDR)
#define	LBLST		(BXXST-LBLSZ)

/*
 * Miminum/maximum value for the top/bottom  of the memory area
 * provided by the BIOS. The 1MB minimum memory limit is enough
 * to load a NetBSD kernel, not to run it. ;-) However, memory
 * reserved by the BIOS is not available to the boot loaders,
 * but will be claimed by the NetBSD kernel.
 */
#define	MAXBOT		(LBLST-4)
#define	MINTOP		(MAXBOT+0x100000)
