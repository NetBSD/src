/*	$NetBSD: intr.c,v 1.5 2003/06/16 20:01:00 thorpej Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: intr.c,v 1.5 2003/06/16 20:01:00 thorpej Exp $");

#include <sys/param.h>
#include <sys/malloc.h>

#include <machine/autoconf.h>
#include <machine/cpufunc.h>
#include <machine/intr.h>

#include <hp700/hp700/intr.h>
#include <hp700/hp700/machdep.h>

#include <uvm/uvm_extern.h>

/* The priority level masks. */
int imask[NIPL];

/* The current priority level. */
volatile int cpl;

/* The pending interrupts. */
volatile int ipending;

/* Nonzero iff we are running an interrupt. */
u_int hppa_intr_depth;

/* The list of all interrupt registers. */
struct hp700_int_reg *hp700_int_regs[HP700_INT_BITS];

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
	 * bit.  If the argument is NULL, the handler 
	 * gets the trapframe.
	 */
	int (*int_bit_handler) __P((void *));
	void *int_bit_arg;

} hp700_int_bits[HP700_INT_BITS];

/* The CPU interrupt register. */
struct hp700_int_reg int_reg_cpu;

/* Old-style vmstat -i interrupt counters.  Should be replaced with evcnts. */
const char intrnames[] = "ipl0\0ipl1\0ipl2\0ipl3\0ipl4\0ipl5\0ipl6\0ipl7\0" \
	"ipl8\0ipl9\0ipl10\0ipl11\0ipl12\0ipl13\0ipl14\0ipl15\0" \
	"ipl16\0ipl17\0ipl18\0ipl19\0ipl20\0ipl21\0ipl22\0ipl23\0" \
	"ipl24\0ipl25\0ipl26\0ipl27\0ipl28\0ipl29\0ipl30\0ipl31\0";
const char eintrnames[] = "";
long intrcnt[HP700_INT_BITS];
long eintrcnt[1];

/*
 * This establishes a new interrupt register.
 */
void
hp700_intr_reg_establish(struct hp700_int_reg *int_reg)
{
	int idx;

	/* Initialize the register structure. */
	memset(int_reg, 0, sizeof(*int_reg));
	memset(int_reg->int_reg_bits_map, INT_REG_BIT_UNUSED,
		sizeof(int_reg->int_reg_bits_map));

	/* Add this structure to the list. */
	for (idx = 0; idx < HP700_INT_BITS; idx++)
		if (hp700_int_regs[idx] == NULL) break;
	if (idx == HP700_INT_BITS)
		panic("hp700_intr_reg_establish: too many regs");
	hp700_int_regs[idx] = int_reg;
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
	memset(hp700_int_regs, 0, sizeof(hp700_int_regs));

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
	int idx;
	
	/* Panic on a bad interrupt bit. */
	if (bit_pos < 0 || bit_pos >= HP700_INT_BITS)
		panic("hp700_intr_establish: bad interrupt bit");

	/* Panic if this int bit is already handled. */
	if (int_reg->int_reg_bits_map[31 ^ bit_pos] != INT_REG_BIT_UNUSED)
		panic("hp700_intr_establish: int already handled");

	/*
	 * If this interrupt bit leads us to another interrupt
	 * register, simply note that in the mapping for the bit.
	 */
	if (handler == NULL) {
		for (idx = 0; idx < HP700_INT_BITS; idx++)
			if (hp700_int_regs[idx] == arg) break;
		if (idx == HP700_INT_BITS)
			panic("hp700_intr_establish: unknown int reg");
		int_reg->int_reg_bits_map[31 ^ bit_pos] = 
			(INT_REG_BIT_REG | idx);
		return (NULL);
	}

	/*
	 * Otherwise, allocate a new bit in the spl.
	 */
	idx = _hp700_intr_ipl_next();
	int_reg->int_reg_allocatable_bits &= ~(1 << bit_pos);
	int_reg->int_reg_bits_map[31 ^ bit_pos] = (31 ^ idx);
	int_bit = hp700_int_bits + idx;

	/* Fill this int bit. */
	int_bit->int_bit_reg = int_reg;
	int_bit->int_bit_ipl = ipl;
	int_bit->int_bit_spl = (1 << idx);
	evcnt_attach_dynamic(&int_bit->int_bit_evcnt, EVCNT_TYPE_INTR, NULL,
	    dv->dv_xname, "intr");
	int_bit->int_bit_handler = handler;
	int_bit->int_bit_arg = arg;

	return (int_bit);
}

/*
 * This allocates an interrupt bit within an interrupt register.
 * It returns the bit position, or -1 if no bits were available.
 */
int
hp700_intr_allocate_bit(struct hp700_int_reg *int_reg)
{
	int bit_pos, mask;

	for (bit_pos = 31, mask = (1 << bit_pos);
	     bit_pos >= 0;
	     bit_pos--, mask >>= 1)
		if (int_reg->int_reg_allocatable_bits & mask)
			break;
	if (bit_pos >= 0)
		int_reg->int_reg_allocatable_bits &= ~mask;
	return (bit_pos);
}

/*
 * This returns the next available spl bit.  This is not
 * intended for wide use.
 */
int
_hp700_intr_ipl_next(void)
{
	int idx;

	for (idx = 0; idx < HP700_INT_BITS; idx++)
		if (hp700_int_bits[idx].int_bit_reg == NULL) break;
	if (idx == HP700_INT_BITS)
		panic("_hp700_intr_spl_bit: too many devices");
	return idx;
}

/*
 * This return the single-bit spl mask for an interrupt.  This 
 * can only be called immediately after hp700_intr_establish, and 
 * is not intended for wide use.
 */
int
_hp700_intr_spl_mask(void *_int_bit)
{
	return ((struct hp700_int_bit *) _int_bit)->int_bit_spl;
}

/*
 * This finally initializes interrupts.
 */
void
hp700_intr_init(void)
{
	int idx, bit_pos;
	struct hp700_int_bit *int_bit;
	int mask;
	struct hp700_int_reg *int_reg;
	int eiem;

	/* Initialize soft interrupts. */
	softintr_init();

	/*
	 * Put together the initial imask for each level.
	 */
	memset(imask, 0, sizeof(imask));
	for (bit_pos = 0; bit_pos < HP700_INT_BITS; bit_pos++) {
		int_bit = hp700_int_bits + bit_pos;
		if (int_bit->int_bit_reg == NULL)
			continue;
		imask[int_bit->int_bit_ipl] |= int_bit->int_bit_spl;
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
        imask[IPL_VM] |= imask[IPL_TTY];
        
        imask[IPL_AUDIO] |= imask[IPL_VM];
   
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
	for (idx = 0; idx < HP700_INT_BITS; idx++) {
		int_reg = hp700_int_regs[idx];
		if (int_reg == NULL)
			continue;
		mask = 0;
		for (bit_pos = 0; bit_pos < HP700_INT_BITS; bit_pos++) {
			if (int_reg->int_reg_bits_map[31 ^ bit_pos] !=
			    INT_REG_BIT_UNUSED)
				mask |= (1 << bit_pos);
		}
		if (int_reg == &int_reg_cpu)
			eiem = mask;
		else if (int_reg->int_reg_mask != NULL)
			*int_reg->int_reg_mask = mask;
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
	int hp700_intr_ipending_new __P((struct hp700_int_reg *, int));

	/*
	 * Read the CPU interrupt register and acknowledge
	 * all interrupts.  Starting with this value, get
	 * our set of new pending interrupts.
	 */
	mfctl(CR_EIRR, eirr);
	mtctl(eirr, CR_EIRR);
	ipending_new = hp700_intr_ipending_new(&int_reg_cpu, eirr);

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
