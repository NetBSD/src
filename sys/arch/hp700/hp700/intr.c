/*	$NetBSD: intr.c,v 1.1 2002/06/06 19:48:06 fredette Exp $	*/

/*
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matthew Fredette.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

/*
 * Interrupt handling for NetBSD/hp700.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: intr.c,v 1.1 2002/06/06 19:48:06 fredette Exp $");

#include <sys/param.h>
#include <sys/malloc.h>

#include <machine/autoconf.h>
#include <machine/cpufunc.h>
#include <machine/intr.h>

#include <hp700/hp700/intr.h>
#include <hp700/hp700/machdep.h>

#include <uvm/uvm_extern.h>

#define	HP700_INT_BITS	(32)

/* The priority level masks. */
int imask[NIPL];

/* The current priority level. */
volatile int cpl;

/* The pending interrupts. */
volatile int ipending;

/* Nonzero iff we are running an interrupt. */
u_int hppa_intr_depth;

/* The list of all interrupt registers. */
static SLIST_HEAD(hp700_int_reg_list, hp700_int_reg)
	hp700_int_regs;

/*
 * The array of interrupt handler structures, one per bit.
 */
static struct hp700_int_bit {

	/* The interrupt register this bit is in. */
	struct hp700_int_reg *int_bit_reg;

	/*
	 * The priority level associated with this
	 * bit, i.e., something like IPL_BIO,
	 * IPL_NET, etc.  
	 */
	int int_bit_ipl;

	/*
	 * The spl mask for this bit.  This starts out
	 * as the spl bit assigned to this particular
	 * interrupt, and later gets fleshed out by the
	 * mask calculator to be the full mask that we
	 * need to raise spl to when we get this interrupt.
	 */
        int int_bit_spl;

	/* The interrupt event count. */
	struct evcnt int_bit_evcnt;

	/*
	 * The interrupt handler and argument for this
	 * bit.  If the handler is NULL, this interrupt
	 * bit is for a whole I/O subsystem, and the
	 * argument is the struct hp700_int_reg for
	 * that subsystem.  If the argument is NULL,
	 * the handler gets the trapframe.
	 */
	int (*int_bit_handler) __P((void *));
	void *int_bit_arg;

} hp700_int_bits[HP700_INT_BITS];

/* The CPU interrupt register. */
struct hp700_int_reg int_reg_cpu;

/* Old-style vmstat -i interrupt counters.  Should be replaced with evcnts. */
const char intrnames[] = "irq0\0irq1\0irq2\0irq3\0irq4\0irq5\0irq6\0irq7\0" \
	"irq8\0irq9\0irq10\0irq11\0irq12\0irq13\0irq14\0irq15\0" \
	"irq16\0irq17\0irq18\0irq19\0irq20\0irq21\0irq22\0irq23\0" \
	"irq24\0irq25\0irq26\0irq27\0irq28\0irq29\0irq30\0irq31\0";
const char eintrnames[] = "";
long intrcnt[HP700_INT_BITS];
long eintrcnt[1];

/*
 * This establishes a new interrupt register.
 */
void
hp700_intr_reg_establish(struct hp700_int_reg *int_reg)
{

	/* Zero the register structure. */
	memset(int_reg, 0, sizeof(*int_reg));

	/* Add this structure to the list. */
	SLIST_INSERT_HEAD(&hp700_int_regs, int_reg, next);
}

/*
 * This bootstraps interrupts.
 */
void
hp700_intr_bootstrap(void)
{
	int i;

	/* Initialize all prority level masks to mask everything. */
	for (i = 0; i < NIPL; i++)
		imask[i] = -1;

	/* We are now at the highest priority level. */
	cpl = -1;

	/* There are no pending interrupts. */
	ipending = 0;

	/* We are not running an interrupt. */
	hppa_intr_depth = 0;

	/* Zero our counters. */
	memset(intrcnt, 0, sizeof(intrcnt));

	/* There are no interrupt handlers. */
	memset(hp700_int_bits, 0, sizeof(hp700_int_bits));

	/* There are no interrupt registers. */
	SLIST_INIT(&hp700_int_regs);

	/* Initialize the CPU interrupt register description. */
	hp700_intr_reg_establish(&int_reg_cpu);
	int_reg_cpu.int_reg_dev = "cpu0";	/* XXX */

	/* Bootstrap soft interrupts. */
	softintr_bootstrap();
}

/*
 * This establishes a new interrupt handler.
 */
void *
hp700_intr_establish(struct device *dv, int ipl,
		     int (*handler) __P((void *)), void *arg,
		     struct hp700_int_reg *int_reg, int bit_pos)
{
	struct hp700_int_bit *int_bit;
	
	/* Panic if this int bit is already handled. */
	int_bit = hp700_int_bits + bit_pos;
	if (int_bit->int_bit_handler != NULL ||
	    int_bit->int_bit_arg != NULL)
		panic("hp700_intr_establish: int already handled\n");

	/* Fill this int bit. */
	int_bit->int_bit_reg = int_reg;
	int_bit->int_bit_ipl = ipl;
	int_bit->int_bit_spl = (1 << bit_pos);
	evcnt_attach_dynamic(&int_bit->int_bit_evcnt, EVCNT_TYPE_INTR, NULL,
	    dv->dv_xname, "intr");
	int_bit->int_bit_handler = handler;
	int_bit->int_bit_arg = arg;

	return (int_bit);
}

/*
 * This finally initializes interrupts.
 */
void
hp700_intr_init(void)
{
	int bit_pos;
	struct hp700_int_bit *int_bit;
	int spl_free, spl_mask;
	struct hp700_int_reg *int_reg;
	int eiem;

	/* Calculate the remaining free spl bits. */
	spl_free = 0;
	for (bit_pos = 0; bit_pos < HP700_INT_BITS; bit_pos++)
		spl_free |= hp700_int_bits[bit_pos].int_bit_spl;
	spl_free = ~spl_free;

	/* Initialize soft interrupts. */
	spl_free = softintr_init(spl_free);

	/*
	 * Put together the initial imask for each level and
	 * mark which bits in each interrupt register are
	 * frobbable.
	 */
	memset(imask, 0, sizeof(imask));
	for (bit_pos = 0; bit_pos < HP700_INT_BITS; bit_pos++) {
		int_bit = hp700_int_bits + bit_pos;
		if (int_bit->int_bit_reg == NULL)
			continue;
		spl_mask = int_bit->int_bit_spl;
		imask[int_bit->int_bit_ipl] |= spl_mask;
		int_bit->int_bit_reg->int_reg_frobbable |= spl_mask;
	}
	
	/* The following bits cribbed from i386/isa/isa_machdep.c: */

        /* 
         * IPL_NONE is used for hardware interrupts that are never blocked,
         * and do not block anything else.
         */
        imask[IPL_NONE] = 0;

        /*
         * Enforce a hierarchy that gives slow devices a better chance at not
         * dropping data.
         */
        imask[IPL_SOFTCLOCK] |= imask[IPL_NONE]; 
        imask[IPL_SOFTNET] |= imask[IPL_SOFTCLOCK];
        imask[IPL_BIO] |= imask[IPL_SOFTNET];
        imask[IPL_NET] |= imask[IPL_BIO];
        imask[IPL_SOFTSERIAL] |= imask[IPL_NET];
        imask[IPL_TTY] |= imask[IPL_SOFTSERIAL];

        /*
         * There are tty, network and disk drivers that use free() at interrupt
         * time, so imp > (tty | net | bio).
         */
        imask[IPL_IMP] |= imask[IPL_TTY];
        
        imask[IPL_AUDIO] |= imask[IPL_IMP];
   
        /*
         * Since run queues may be manipulated by both the statclock and tty,
         * network, and disk drivers, clock > imp.
         */             
        imask[IPL_CLOCK] |= imask[IPL_AUDIO];
        
        /* 
         * IPL_HIGH must block everything that can manipulate a run queue.
         */     
        imask[IPL_HIGH] |= imask[IPL_CLOCK]; 

	/* Now go back and flesh out the spl levels on each bit. */
	for(bit_pos = 0; bit_pos < HP700_INT_BITS; bit_pos++) {
		int_bit = hp700_int_bits + bit_pos;
		if (int_bit->int_bit_reg == NULL)
			continue;
		int_bit->int_bit_spl = imask[int_bit->int_bit_ipl];
	}

	/* Print out the levels. */
	printf("biomask %08x netmask %08x ttymask %08x\n",
	    imask[IPL_BIO], imask[IPL_NET], imask[IPL_TTY]);
#if 0
	for(bit_pos = 0; bit_pos < NIPL; bit_pos++)
		printf("imask[%d] == %08x\n", bit_pos, imask[bit_pos]);
#endif

	/*
	 * Load all mask registers, loading %eiem last.
	 * This will finally enable interrupts, but since
	 * cpl and ipending should be -1 and 0, respectively,
	 * no interrupts will get dispatched until the 
	 * priority level is lowered.
	 *
	 * Because we're paranoid, we force these values
	 * for cpl and ipending, even though they should
	 * be unchanged since hp700_intr_bootstrap().
	 */
	cpl = -1;
	ipending = 0;
	SLIST_FOREACH(int_reg, &hp700_int_regs, next) {
		spl_mask = int_reg->int_reg_frobbable;
		if (int_reg == &int_reg_cpu)
			eiem = spl_mask;
		else if (int_reg->int_reg_mask != NULL)
			*int_reg->int_reg_mask = spl_mask;
	}
	mtctl(eiem, CR_EIEM);
}

/*
 * Service interrupts.  This doesn't necessarily dispatch them.
 * This is called with %eiem loaded with zero.  It's named
 * hppa_intr instead of hp700_intr because trap.c calls it.
 */
void
hppa_intr(struct trapframe *frame)
{
	int eirr;
	int ipending_new;
	int bit_pos, bit_mask;
	struct hp700_int_bit *int_bit;
	struct hp700_int_reg *int_reg;

	/*
	 * Read the CPU interrupt register and acknowledge
	 * all interrupts.  Take this value as our initial
	 * set of new pending interrupts.
	 */
	mfctl(CR_EIRR, eirr);
	mtctl(eirr, CR_EIRR);
	ipending_new = eirr;

	/* Loop while there are CPU interrupt bits. */
	while (eirr) {

		/* Get the next bit. */
		bit_pos = ffs(eirr) - 1;
		bit_mask = ~(1 << bit_pos);
		eirr &= bit_mask;
		int_bit = hp700_int_bits + bit_pos;

		/*
		 * If this bit has a real interrupt handler,
		 * it's not for an I/O subsystem.  Continue.
		 */
		if (int_bit->int_bit_handler != NULL)
			continue;

		/*
		 * Now this bit is either for an I/O subsystem 
		 * or it's a stray.
		 */

		/* Clear the bit from our ipending_new value. */
		ipending_new &= bit_mask;

		/*
		 * If this is for an I/O subsystem, read its 
		 * request register, mask off any bits that 
		 * aren't frobbable, and or the result into 
		 * ipending_new.
		 *
		 * The read of the request register also serves 
		 * to acknowledge the interrupt to the I/O subsystem.
		 */
		int_reg = int_bit->int_bit_arg;
		if (int_reg != NULL)
			ipending_new |= (*int_reg->int_reg_req &
					 int_reg->int_reg_frobbable);
		else
			printf("cpu0: stray int %d\n", bit_pos);
	}

	/* Add these new bits to ipending. */
	ipending |= ipending_new;

	/* If we have interrupts to dispatch, do so. */
	if (ipending & ~cpl)
		hp700_intr_dispatch(cpl, frame->tf_eiem, frame);
#if 0
	else if (ipending != 0x80000000)
		printf("ipending %x cpl %x\n", ipending, cpl);
#endif
}
		
/*
 * Dispatch interrupts.  This dispatches at least one interrupt.
 * This is called with %eiem loaded with zero.
 */
void
hp700_intr_dispatch(int ncpl, int eiem, struct trapframe *frame)
{
	int ipending_run;
	u_int old_hppa_intr_depth;
	int bit_pos;
	struct hp700_int_bit *int_bit;
	void *arg;
	struct clockframe clkframe;
	int handled;

	/* Increment our depth, grabbing the previous value. */
	old_hppa_intr_depth = hppa_intr_depth++;

	/* Loop while we have interrupts to dispatch. */
	for (;;) {

		/* Read ipending and mask it with ncpl. */
		ipending_run = (ipending & ~ncpl);
		if (ipending_run == 0)
			break;

		/* Choose one of the resulting bits to dispatch. */
		bit_pos = ffs(ipending_run) - 1;

		/* Increment the counter for this interrupt. */
		intrcnt[bit_pos]++;

		/*
		 * If this interrupt handler takes the clockframe
		 * as an argument, conjure one up.
		 */
		int_bit = hp700_int_bits + bit_pos;
		arg = int_bit->int_bit_arg;
		if (arg == NULL) {
			clkframe.cf_flags = (old_hppa_intr_depth ? 
						TFF_INTR : 0);
			clkframe.cf_spl = ncpl;
			if (frame != NULL) {
				clkframe.cf_flags |= frame->tf_flags;
				clkframe.cf_pc = frame->tf_iioq_head;
			}
			arg = &clkframe;
		}
	
		/*
		 * Remove this bit from ipending, raise spl to 
		 * the level required to run this interrupt, 
		 * and reenable interrupts.
		 */
		ipending &= ~(1 << bit_pos);
		cpl = ncpl | int_bit->int_bit_spl;
		mtctl(eiem, CR_EIEM);

		/* Dispatch the interrupt. */
		handled = (*int_bit->int_bit_handler)(arg);
#if 0
		if (!handled)
			printf("%s: can't handle interrupt\n",
				int_bit->int_bit_evcnt.ev_name);
#endif

		/* Disable interrupts and loop. */
		mtctl(0, CR_EIEM);
	}

	/* Interrupts are disabled again, restore cpl and the depth. */
	cpl = ncpl;
	hppa_intr_depth = old_hppa_intr_depth;
}
