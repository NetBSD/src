/*	$NetBSD: openfirm.h,v 1.5.14.1 2017/12/03 11:36:44 jdolecek Exp $	*/

/*
 * Copyright (C) 1995, 1996 Wolfgang Solfrank.
 * Copyright (C) 1995, 1996 TooLs GmbH.
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
/*
 * Prototypes for Openfirmware Interface Routines
 */

#include <sys/cdefs.h>
#include <sys/types.h>
#include <machine/openfirm.h>

#ifndef PROM_MAX_PATH
#define PROM_MAX_PATH	128
#endif

#if 0
u_int OF_finddevice(char *name);
u_int OF_instance_to_package(u_int ihandle);
u_int OF_getprop(u_int handle, char *prop, void *buf, int buflen);
#ifdef	__notyet__
int OF_setprop(u_int handle, char *prop, void *buf, int len);
#endif
u_int OF_open(char *dname);
void OF_close(u_int handle);
int OF_write(u_int handle, void *addr, int len);
int OF_read(u_int handle, void *addr, int len);
int OF_seek(u_int handle, uint64_t pos);
#endif
void*	OF_claim(void *, u_int, u_int);
void	OF_release(void *, u_int);
int	OF_milliseconds(void);
void	OF_chain(void *, u_int,
	    void (*)(int, int, int (*)(void *), void *, u_int), void *, u_int);
int	OF_peer(int);
int	OF_child(int);
vaddr_t	OF_claim_virt(vaddr_t, int);
vaddr_t	OF_alloc_virt(int, int);
int	OF_free_virt(vaddr_t, int);
int	OF_unmap_virt(vaddr_t, int);
vaddr_t	OF_map_phys(paddr_t, off_t, vaddr_t, int);
paddr_t	OF_alloc_phys(int, int);
paddr_t	OF_claim_phys(paddr_t, int);
int	OF_free_phys(paddr_t, int);
int	OF_interpret(const char *, int, int, ...);
void	OF_initialize(void);
