/*	$NetBSD: hypervisor_machdep.c,v 1.2 2004/04/24 17:41:49 cl Exp $	*/

/*
 *
 * Copyright (c) 2004 Christian Limpach.
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
 *      This product includes software developed by Christian Limpach.
 * 4. The name of the author may not be used to endorse or promote products
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

/******************************************************************************
 * hypervisor.c
 * 
 * Communication to/from hypervisor.
 * 
 * Copyright (c) 2002-2003, K A Fraser
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
 */


#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: hypervisor_machdep.c,v 1.2 2004/04/24 17:41:49 cl Exp $");

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/systm.h>

#include <machine/xen.h>
#include <machine/hypervisor.h>

/* static */ unsigned long event_mask = 0;
static unsigned long ev_err_count;

int stipending(void);
int
stipending()
{
	unsigned long events;
	struct cpu_info *ci;
	int num, ret;

	ret = 0;
	ci = curcpu();

#if 0
	if (HYPERVISOR_shared_info->events)
		printf("stipending events %08lx mask %08lx ilevel %d\n",
		    HYPERVISOR_shared_info->events,
		    HYPERVISOR_shared_info->events_mask, ci->ci_ilevel);
#endif

	do {
		/*
		 * we're only called after STIC, so we know that we'll
		 * have to STI at the end
		 */
		__cli();

		events = xchg(&HYPERVISOR_shared_info->events, 0);
		events &= event_mask;

		while (events) {
			__asm__ __volatile__ (
				"   bsfl %1,%0		;"
				"   btrl %0,%1		;"
				: "=r" (num) : "r" (events));
			ci->ci_ipending |= (1 << num);
			if (ret == 0 &&
			    ci->ci_ilevel <
			    ci->ci_isources[num]->is_handlers->ih_level)
				ret = 1;
		}

		__sti();
	} while (HYPERVISOR_shared_info->events);

#if 0
	if (ci->ci_ipending & 0x1)
		printf("stipending events %08lx mask %08lx ilevel %d ipending %08x\n",
		    HYPERVISOR_shared_info->events,
		    HYPERVISOR_shared_info->events_mask, ci->ci_ilevel,
		    ci->ci_ipending);
#endif

	return (ret);
}

void do_hypervisor_callback(struct trapframe *regs)
{
	unsigned long events, flags;
	shared_info_t *shared = HYPERVISOR_shared_info;
	struct cpu_info *ci;
	int level;
	extern int once;

	ci = curcpu();
	level = ci->ci_ilevel;
	if (0 && once == 2)
		printf("hypervisor\n");

	do {
		/* Specialised local_irq_save(). */
		flags = test_and_clear_bit(EVENTS_MASTER_ENABLE_BIT, 
		    &shared->events_mask);
		barrier();

		events = xchg(&shared->events, 0);
		events &= event_mask;

		/* 'events' now contains some pending events to handle. */
		__asm__ __volatile__ (
			"   push %1                    ;"
			"   sub  $4,%%esp              ;"
			"   jmp  2f                    ;"
			"1: btrl %%eax,%0              ;" /* clear bit     */
			"   mov  %%eax,(%%esp)         ;"
			"   call do_event              ;" /* do_event(event) */
			"2: bsfl %0,%%eax              ;" /* %eax == bit # */
			"   jnz  1b                    ;"
			"   add  $8,%%esp              ;"
			/* we use %ebx because it is callee-saved */
			: : "b" (events), "r" (regs)
			/* clobbered by callback function calls */
			: "eax", "ecx", "edx", "memory" ); 

		/* Specialised local_irq_restore(). */
		if (flags)
			set_bit(EVENTS_MASTER_ENABLE_BIT, &shared->events_mask);
		barrier();
	}
	while ( shared->events );

	if (level != ci->ci_ilevel)
		printf("hypervisor done %08lx level %d/%d ipending %08x\n",
		    HYPERVISOR_shared_info->events_mask, level, ci->ci_ilevel,
		    ci->ci_ipending);
	if (0 && once == 2)
		printf("hypervisor done\n");
}

void hypervisor_enable_event(unsigned int ev)
{
	set_bit(ev, &event_mask);
	set_bit(ev, &HYPERVISOR_shared_info->events_mask);
#if 0
	if ( test_bit(EVENTS_MASTER_ENABLE_BIT, 
		 &HYPERVISOR_shared_info->events_mask) )
		do_hypervisor_callback(NULL);
#endif
}

void hypervisor_disable_event(unsigned int ev)
{
	clear_bit(ev, &event_mask);
	clear_bit(ev, &HYPERVISOR_shared_info->events_mask);
}

void hypervisor_acknowledge_event(unsigned int ev)
{
	if ( !(event_mask & (1<<ev)) )
		atomic_inc((atomic_t *)(void *)&ev_err_count);
	set_bit(ev, &HYPERVISOR_shared_info->events_mask);
}
