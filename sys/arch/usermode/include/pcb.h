/* $NetBSD: pcb.h,v 1.12.2.1 2012/04/17 00:06:59 yamt Exp $ */

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
#include <sys/queue.h>


#define TRAPSTACKSIZE (USPACE -2*sizeof(ucontext_t) - 3*sizeof(register_t))
struct pcb {
	ucontext_t pcb_ucp;		/* switchframe */
	ucontext_t pcb_userret_ucp;

	uint8_t *sys_stack_top;		/* points at free point in sys_stack */
	uint8_t	 sys_stack[TRAPSTACKSIZE];

	void	*pcb_onfault;		/* on fault handler */

	int	 pcb_errno;		/* save/restore place */
};

#endif /* !_ARCH_USERMODE_INCLUDE_PCB_H */
