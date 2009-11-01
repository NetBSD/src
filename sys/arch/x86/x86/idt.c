/*	$NetBSD: idt.c,v 1.2.14.2 2009/11/01 13:58:18 jym Exp $	*/

/*-
 * Copyright (c) 1996, 1997, 1998, 2000, 2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum, by Jason R. Thorpe of the Numerical Aerospace
 * Simulation Facility NASA Ames Research Center, and by Andrew Doran.
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

/*-
 * Copyright (c) 1982, 1987, 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)machdep.c	7.4 (Berkeley) 6/3/91
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: idt.c,v 1.2.14.2 2009/11/01 13:58:18 jym Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mutex.h>
#include <sys/cpu.h>
#include <sys/atomic.h>

#include <machine/segments.h>

#if !defined(XEN)

struct gate_descriptor *idt;
static char idt_allocmap[NIDT];

/*
 * Allocate an IDT vector slot within the given range.
 * cpu_lock will be held unless single threaded during early boot.
 */
int
idt_vec_alloc(int low, int high)
{
	int vec;

	KASSERT(mutex_owned(&cpu_lock) || !mp_online);

	for (vec = low; vec <= high; vec++) {
		if (idt_allocmap[vec] == 0) {
			/* idt_vec_free() can be unlocked, so membar. */
			membar_sync();
			idt_allocmap[vec] = 1;
			return vec;
		}
	}
	return 0;
}

void
idt_vec_reserve(int vec)
{
	int result;

	KASSERT(mutex_owned(&cpu_lock) || !mp_online);

	result = idt_vec_alloc(vec, vec);
	if (result != vec) {
		panic("%s: failed to reserve vec %d", __func__, vec);
	}
}

void
idt_vec_set(int vec, void (*function)(void))
{

	KASSERT(mutex_owned(&cpu_lock) || !mp_online);
	KASSERT(idt_allocmap[vec] == 1);
	setgate(&idt[vec], function, 0, SDT_SYS386IGT, SEL_KPL,
	    GSEL(GCODE_SEL, SEL_KPL));
}

/*
 * Free IDT vector.  No locking required as release is atomic.
 */
void
idt_vec_free(int vec)
{

	unsetgate(&idt[vec]);
	idt_allocmap[vec] = 0;
}

void
idt_init(void)
{

}

#endif /* !defined(XEN) */
