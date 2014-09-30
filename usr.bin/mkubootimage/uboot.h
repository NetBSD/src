/* $NetBSD: uboot.h,v 1.6 2014/09/30 10:21:50 msaitoh Exp $ */

/*-
 * Copyright (c) 2010 Jared D. McNeill <jmcneill@invisible.ca>
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

#ifndef _HAVE_UBOOT_H
#define _HAVE_UBOOT_H

enum uboot_image_os {
	IH_OS_UNKNOWN = 0,
	IH_OS_OPENBSD = 1,
	IH_OS_NETBSD = 2,
	IH_OS_FREEBSD = 3,
	IH_OS_LINUX = 5
};

enum uboot_image_arch {
	IH_ARCH_UNKNOWN = 0,
	IH_ARCH_ARM = 2,
	IH_ARCH_I386 = 3,
	IH_ARCH_MIPS = 5,
	IH_ARCH_MIPS64 = 6,
	IH_ARCH_PPC = 7,
	IH_ARCH_OPENRISC = 21,
	IH_ARCH_ARM64 = 22
};

enum uboot_image_type {
	IH_TYPE_UNKNOWN = 0,
	IH_TYPE_STANDALONE = 1,
	IH_TYPE_KERNEL = 2,
	IH_TYPE_RAMDISK = 3,
	IH_TYPE_SCRIPT = 6,
	IH_TYPE_FILESYSTEM = 7,
};

enum uboot_image_comp {
	IH_COMP_NONE = 0,
	IH_COMP_GZIP = 1,
	IH_COMP_BZIP2 = 2,
	IH_COMP_LZMA = 3,
	IH_COMP_LZO = 4,
};

#define IH_MAGIC	0x27051956
#define IH_NMLEN	32

struct uboot_image_header {
	uint32_t	ih_magic;
	uint32_t	ih_hcrc;
	uint32_t	ih_time;
	uint32_t	ih_size;
	uint32_t	ih_load;
	uint32_t	ih_ep;
	uint32_t	ih_dcrc;
	uint8_t		ih_os;
	uint8_t		ih_arch;
	uint8_t		ih_type;
	uint8_t		ih_comp;
	uint8_t		ih_name[IH_NMLEN];
};

#endif /* !_HAVE_UBOOT_H */
