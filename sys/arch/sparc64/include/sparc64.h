/*	$NetBSD: sparc64.h,v 1.8.6.1 2006/04/22 11:38:02 simonb Exp $	*/

/*
 * Copyright (C) 1996 Wolfgang Solfrank.
 * Copyright (C) 1996 TooLs GmbH.
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
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef	_MACHINE_SPARC64_H_
#define	_MACHINE_SPARC64_H_

struct mem_region {
	uint64_t start;
	uint64_t size;
};

int prom_set_trap_table (vaddr_t);
uint64_t prom_vtop (vaddr_t);
vaddr_t prom_claim_virt (vaddr_t, int);
vaddr_t prom_alloc_virt (int, int);
int prom_free_virt (vaddr_t, int);
int prom_unmap_virt (vaddr_t, int);
int prom_map_phys (uint64_t, off_t, vaddr_t, int);
uint64_t prom_alloc_phys (int , int);
uint64_t prom_claim_phys (paddr_t, int);
int prom_free_phys (paddr_t, int);
uint64_t prom_get_msgbuf (int, int);

void prom_stopself(void);
void prom_startcpu(u_int, void *, u_long);

#endif	/* _MACHINE_SPARC64_H_ */
