/*	$NetBSD: fpuvar.h,v 1.2 2002/03/24 23:37:42 bjh21 Exp $	*/

/*-
 * Copyright (c) 2001 Ben Harris
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

#ifndef _FPUVAR_H
#define _FPUVAR_H

#include <sys/device.h>
#include <machine/frame.h>

struct proc;

struct fpu_softc {
	struct device	sc_dev;
	register_t	sc_fputype;
	void		(*sc_ctxload)(struct fpframe *);
	void		(*sc_ctxsave)(struct fpframe *);
	void		(*sc_enable)(void);
	void		(*sc_disable)(void);
	struct proc	*sc_owner;
};

#ifdef _KERNEL
extern struct fpu_softc *the_fpu;

extern void fpu_swapout(struct proc *);

void fpctx_save_fpa(struct fpframe *);
void fpctx_load_fpa(struct fpframe *);
void fpu_enable_fpa(void);
void fpu_disable_fpa(void);
void fpctx_save_fppc(struct fpframe *);
void fpctx_load_fppc(struct fpframe *);
void fpu_enable_fppc(void);
void fpu_disable_fppc(void);
#endif

#endif
