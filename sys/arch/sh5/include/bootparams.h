/*	$NetBSD: bootparams.h,v 1.1 2003/03/13 13:44:17 scw Exp $	*/

/*
 * Copyright 2003 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Steve C. Woodford for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _SH5_BOOTPARAMS_H
#define _SH5_BOOTPARAMS_H

#define	BP_N_MEMBLOCKS		4		/* Handle 4 chunks of RAM */
#define	BP_N_CPUS		1		/* Handle 1 cpu (no SMP yet) */
#define	BP_PATH_STRING_LEN	128		/* Length of path/dev strings */

/*
 * This is the structure passed in from the boot loader.
 */
struct boot_params {
	/* Magic number */
	u_int		bp_magic;
#define	BP_MAGIC		0xcafebabe	/* bp_magic number */

	/* Structure version number */
	u_int		bp_version;
#define	BP_VERSION		0x00000001	/* structure version number */

	/* Boot Flags (RB_SINGLE, RB_KDB, etc) */
	u_int		bp_flags;

	/* Physical address of start of KSEG0 */
	int64_t		bp_kseg0_phys;

	/* Machine identifier string */
	char		bp_machine[64];

	/* CPU parameters */
	struct {
		u_int		cpuid;
#define	BP_CPUID_UNUSED		0xffffffff
		u_int		version;
		u_int		pport;
		u_int64_t	speed;
	} bp_cpu[BP_N_CPUS];

	/* Memory parameters */
	struct {
		u_int		type;
#define	BP_MEM_TYPE_UNUSED	0
#define	BP_MEM_TYPE_SDRAM	1
		u_int		pport;
		int64_t		physstart;
		int64_t		physsize;
	} bp_mem[BP_N_MEMBLOCKS];

	/* Locator for the device we booted from */
	struct {
		char		dev[BP_PATH_STRING_LEN];
		char		path[BP_PATH_STRING_LEN];
		u_int		partition;
	} bp_bootdev;

	/* Locator for the console device */
	struct {
		char		dev[BP_PATH_STRING_LEN];
		u_int		speed;
	} bp_consdev;
};

#if defined(_KERNEL) || defined(_STANDALONE)
extern struct boot_params bootparams;
#endif

#endif /* _SH5_BOOTPARAMS_H */
