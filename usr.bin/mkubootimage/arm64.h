/* $NetBSD: arm64.h,v 1.2 2018/02/04 17:33:34 jmcneill Exp $ */

/*-
 * Copyright (c) 2018 Jared McNeill <jmcneill@invisible.ca>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _HAVE_ARM64_H
#define _HAVE_ARM64_H

/*
 * AArch64 Linux kernel image header, as specified in
 * https://www.kernel.org/doc/Documentation/arm64/booting.txt
 */

/* 64-byte kernel image header */
struct arm64_image_header {
	uint32_t	code0;		/* Executable code */
	uint32_t	code1;		/* Executable code */
	uint64_t	text_offset;	/* Image load offset */
	uint64_t	image_size;	/* Effective image size */
	uint64_t	flags;		/* kernel flags */
	uint64_t	res2;		/* reserved */
	uint64_t	res3;		/* reserved */
	uint64_t	res4;		/* reserved */
	uint32_t	magic;		/* Magic number ("ARM\x64") */
	uint32_t	res5;		/* reserved (used for PE COFF offset) */
};

/* Kernel flags */
#define	ARM64_FLAGS_ENDIAN_BE		(1 << 0)
#define	ARM64_FLAGS_PAGE_SIZE		(3 << 1)
#define	 ARM64_FLAGS_PAGE_SIZE_UNSPEC		(0 << 1)
#define	 ARM64_FLAGS_PAGE_SIZE_4K		(1 << 1)
#define	 ARM64_FLAGS_PAGE_SIZE_16K		(2 << 1)
#define	 ARM64_FLAGS_PAGE_SIZE_64K		(3 << 1)
#define	ARM64_FLAGS_PHYS_PLACEMENT	(1 << 3)
#define	 ARM64_FLAGS_PHYS_PLACEMENT_DRAM_BASE	(0 << 3)
#define	 ARM64_FLAGS_PHYS_PLACEMENT_ANY		(1 << 3)

/* Magic */
#define	ARM64_MAGIC	0x644d5241

/* Executable code. Program relative branch forward 64 bytes. */
#define	ARM64_CODE0	0x14000010

#endif /* !_HAVE_ARM64_H */
