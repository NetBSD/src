/*	$NetBSD: initarmvar.h,v 1.1 2003/04/18 12:01:32 scw Exp $	*/

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

#ifndef __EVBARM_INITARMVAR_H
#define __EVBARM_INITARMVAR_H

struct initarm_iospace {
	vaddr_t ii_kva;			/* KVA at which to map this io space */
	paddr_t ii_pa;			/* PA of start of region to map */
	psize_t ii_size;		/* Size of region, in bytes */
	vm_prot_t ii_prot;		/* VM_PROT_READ/VM_PROT_WRITE */
	int ii_cache;			/* PTE_{NO,}CACHE */
};

struct initarm_config {
	const BootConfig *ic_bootconf;	/* Boot configuration */
	paddr_t ic_kernel_base_pa;	/* Physical address of KERNEL_BASE */
	vaddr_t ic_vecbase;		/* ARM_VECTORS_{LOW,HIGH} */
	vaddr_t ic_iobase;		/* KVA of start of fixed io space */
	vsize_t ic_iosize;		/* Size of fixed io space (bytes) */
	int ic_nio;			/* # of fixed io mappings in ic_io */
	const struct initarm_iospace *ic_io;	/* List of fixed io mappings */
};

extern vaddr_t initarm_common(const struct initarm_config *);

#endif /* __EVBARM_INITARMVAR_H */
