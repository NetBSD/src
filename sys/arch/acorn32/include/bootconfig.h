/*	$NetBSD: bootconfig.h,v 1.1.6.3 2002/03/16 15:55:25 jdolecek Exp $	*/

/*
 * Copyright (c) 2002 Reinoud Zandijk.
 * Copyright (c) 1994 Mark Brinicombe.
 * Copyright (c) 1994 Brini.
 * All rights reserved.
 *
 * This code is derived from software written for Brini by Mark Brinicombe
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
 *	This product includes software developed by Mark Brinicombe
 *	for the NetBSD Project.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * boot configuration structures
 *
 */

#if defined(_KERNEL)


/* get some spare blocks ;) */
#define DRAM_BLOCKS	32
#define VRAM_BLOCKS	16


typedef struct {
	u_int address;
	u_int pages;
	u_int flags;
} phys_mem;


typedef struct _BootConfig {
	u_int magic;
	u_int version;			/* version 2+ */

	u_char machine_id[4];
	char kernelname[80];
	char args[512];			/* 512 bytes is better than 4096 */

	u_int kernvirtualbase;		/* not used now */
	u_int kernphysicalbase;		/* not used now */
	u_int kernsize;
	u_int scratchvirtualbase;
	u_int scratchphysicalbase;
	u_int scratchsize;

	u_int ksym_start;
	u_int ksym_end;

	u_int MDFvirtualbase;		/* not used yet */
	u_int MDFphysicalbase;		/* not used yet */
	u_int MDFsize;			/* not used yet */

	u_int display_phys;
	u_int display_start;
	u_int display_size;
	u_int width;
	u_int height;
	u_int log2_bpp;
	u_int framerate;

	char reserved[512];		/* future expansion */

	u_int pagesize;
	u_int drampages;
	u_int vrampages;
	u_int dramblocks;
	u_int vramblocks;

	phys_mem dram[DRAM_BLOCKS];
	phys_mem vram[VRAM_BLOCKS];

} BootConfig;


/************ compat stuff ************/

typedef struct {
	u_int address;
	u_int pages;
} phys_mem_v1;


typedef struct {
	u_int kernvirtualbase;
	u_int kernphysicalbase;
	u_int kernsize;
	u_int argvirtualbase;
	u_int argphysicalbase;
	u_int argsize;
	u_int scratchvirtualbase;
	u_int scratchphysicalbase;
	u_int scratchsize;

	u_int display_start;
	u_int display_size;
	u_int width;
	u_int height;
	u_int log2_bpp;

	phys_mem_v1 dram[4];
	phys_mem_v1 vram[1];

	u_int dramblocks;
	u_int vramblocks;
	u_int pagesize;
	u_int drampages;
	u_int vrampages;

	char kernelname[80];

	u_int framerate;
	u_char machine_id[4];
	u_int magic;
	u_int display_phys;
} BootConfig_v1;

/************ end compat stuff ***********/

#define BOOTCONFIG_MAGIC     0x43112233
#define BOOTCONFIG_VERSION   	    0x2

extern BootConfig bootconfig;
#endif	/* _KERNEL */


#ifdef _KERNEL
#define BOOTOPT_TYPE_BOOLEAN		0
#define BOOTOPT_TYPE_STRING		1
#define BOOTOPT_TYPE_INT		2
#define BOOTOPT_TYPE_BININT		3
#define BOOTOPT_TYPE_HEXINT		4
#define BOOTOPT_TYPE_MASK		7

int get_bootconf_option __P((char *string, char *option, int type, void *result));

extern char *boot_args;
extern char *boot_file;
#endif	/* _KERNEL */


/* End of bootconfig.h */
