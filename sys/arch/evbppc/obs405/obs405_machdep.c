/*	$NetBSD: obs405_machdep.c,v 1.1.12.2 2005/09/15 14:28:44 riz Exp $	*/

/*
 * Copyright (c) 2004 Shigeyuki Fukushima.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: obs405_machdep.c,v 1.1.12.2 2005/09/15 14:28:44 riz Exp $");

#include <sys/param.h>

#include <uvm/uvm_extern.h>

#include <net/netisr.h>

#include <machine/cpu.h>

/*
 * Machine-dependent global variables
 *   exec_map:		sys/uvm/uvm_extern.h
 *   mb_map:		sys/uvm/uvm_extern.h
 *   phys_map:		sys/uvm/uvm_extern.h
 *   machine:		sys/sys/systm.h
 *   machine_arch:	sys/sys/systm.h
 */
struct vm_map *exec_map = NULL;
struct vm_map *mb_map = NULL;
struct vm_map *phys_map = NULL;
char machine[] = MACHINE;		/* from <machine/param.h> */
char machine_arch[] = MACHINE_ARCH;	/* from <machine/param.h> */

/*
 * TODO: XXX these functions are machine-dependent ??
 */

/*
 * softnet:
 * Soft networking interrupts.
 */
void
softnet(void)
{
	int isr;

	isr = netisr;
	netisr = 0;

#define DONETISR(bit, fn) do {		\
	if (isr & (1 << bit))		\
		fn();			\
} while (0)

#include <net/netisr_dispatch.h>
#undef DONETISR
}

/*
 * softserial:
 * Soft tty interrupts.
 */
#include "com.h"
void
softserial(void)
{
#if NCOM > 0
	void comsoft(void);	/* XXX from dev/ic/com.c */

	comsoft();
#endif
}
