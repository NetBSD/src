/* $NetBSD: pcb.h,v 1.9 2011/09/04 20:54:52 reinoud Exp $ */

/*-
 * Copyright (c) 2007 Jared D. McNeill <jmcneill@invisible.ca>
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

#ifndef _ARCH_USERMODE_INCLUDE_PCB_H
#define _ARCH_USERMODE_INCLUDE_PCB_H

#include <sys/cdefs.h>
#include <sys/ucontext.h>

/*
 * Trap frame.  Pushed onto the kernel stack on a trap (synchronous exception).
 * XXX move to frame.h?
 */

/* XXX unused XXX */
typedef struct trapframe {
	int		(*tf_syscall)(lwp_t *, struct trapframe *);
	int		 tf_reason;		/* XXX unused */
	uintptr_t	 tf_io[8];		/* to transport info */
} trapframe_t;


struct pcb {
	ucontext_t	 pcb_ucp;		/* lwp switchframe */
	ucontext_t	 pcb_userland_ucp;	/* userland switchframe */
	bool		 pcb_needfree;
	struct trapframe pcb_tf;
	void *		 pcb_onfault;		/* on fault handler */

	int		 errno;			/* save/restore place */
};

#endif /* !_ARCH_USERMODE_INCLUDE_PCB_H */
