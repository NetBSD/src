/*	$NetBSD: isa_shark_machdep.c,v 1.5.10.1 2000/06/22 16:59:31 minoura Exp $	*/

/*
 * Copyright 1997
 * Digital Equipment Corporation. All rights reserved.
 *
 * This software is furnished under license and may be used and
 * copied only in accordance with the following terms and conditions.
 * Subject to these conditions, you may download, copy, install,
 * use, modify and distribute this software in source and/or binary
 * form. No title or ownership is transferred hereby.
 *
 * 1) Any source code used, modified or distributed must reproduce
 *    and retain this copyright notice and list of conditions as
 *    they appear in the source file.
 *
 * 2) No right is granted to use any trade name, trademark, or logo of
 *    Digital Equipment Corporation. Neither the "Digital Equipment
 *    Corporation" name nor any trademark or logo of Digital Equipment
 *    Corporation may be used to endorse or promote products derived
 *    from this software without the prior written permission of
 *    Digital Equipment Corporation.
 *
 * 3) This software is provided "AS-IS" and any express or implied
 *    warranties, including but not limited to, any implied warranties
 *    of merchantability, fitness for a particular purpose, or
 *    non-infringement are disclaimed. In no event shall DIGITAL be
 *    liable for any damages whatsoever, and in particular, DIGITAL
 *    shall not be liable for special, indirect, consequential, or
 *    incidental damages or damages for lost profits, loss of
 *    revenue or loss of use, whether such damages arise in contract,
 *    negligence, tort, under statute, in equity, at law or otherwise,
 *    even if advised of the possibility of such damage.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/syslog.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <machine/irqhandler.h>
#include <machine/pio.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>
#include <dev/isa/isadmavar.h>
#include <arm32/isa/icu.h>

#include <machine/ofw.h>

struct arm32_isa_chipset isa_chipset_tag;

unsigned i8259_mask;

/* Notes on the interaction of StrongARM and ISA.  A lot of the nastiness
   is caused by consciously prostituting shark to a low bill of materials.
   
   It takes on the order of 700ns (about 150 instruction cycles at
   233 MHz) to access the ISA bus, so it is important to minimize the number
   of ISA accesses, in particular to the 8259 interrupt controllers.
   
   To reduce the number of accesses, the 8259's are NOT run in the
   same mode as on a typical Intel (IBM AT) system, which requires
   an interrupt acknowledge sequence (INTA) for every interrupt.
   Instead, the 8259's are used as big OR gates with interrupt masks
   on the front.  The code in irq.S takes particular care to cache
   the state of the interrupt masks and only update them when absolutely
   necessary.
   
   Unfortunately, resetting the 8259 edge detectors without a real
   INTA sequence is problematic at best.  To complicate matters further,
   (unlike EISA components) the 8259s on the Sequoia core logic do
   not allow configuration of edge vs. level on an IRQ-by-IRQ basis.
   Thus, all interrupts must be either edge-triggered or level-triggered.
   To preserve the sanity of the system, this code chooses the 
   level-triggered configuration.
   
   None of the possible operation modes of the 8254 interval timers can 
   be used to generate a periodic, level-triggered, clearable clock 
   interrupt.  This restriction means that TIMER0 -- hardwired to IRQ0 -- 
   may not be used as the heartbeat timer, as it is on Intel-based PCs.  
   Instead, the real-time clock (RTC) interrupt -- connected to 
   IRQ8 -- has the right properties and is used for the heartbeat interrupt.
   TIMER0 may still be used to implement a microsecond timer.
   See clock.c for details.
   
   As on most PC systems, 8254 TIMER1 is used for the ISA refresh signal.
   
   Unlike most PC systems, 8254 TIMER2 is not used for cheap tone
   generation.  Instead, it is used to create a high-availability interrupt
   for bit-bashing functions (e.g. for SmartCard access).  TIMER2 output,
   called "SPKR" on Sequoia 2, is routed back into the SWTCH input on
   Sequoia 1.  This input eventually reemerges from Sequoia 1 on the SMI pin,
   which is then converted into the StrongARM FIQ (fast interrupt request).
   To clear this interrupt, the StrongARM clears the SMI.
   See .../shark/fiq.S for details.
   
   One more complication: ISA devices can be rather nasty with respect
   to ISA bus usage.  For example, the CS8900 ethernet chip will occupy
   the bus for very long DMA streams.  It is possible to configure the
   chip so it relinquishes the ISA bus every 28 usec or so
   (about every 6500 instructions).  This causes problems when trying
   to run the TIMER2/SMI/FIQ at 50 kHz, which is required to detect the
   baud rate of the SmartCard.  A modification to .../dev/isa/isadma.c 
   allows the processor to freeze DMA during critial periods of time.  
   This is a working -- but not very satisfactory -- solution to the problem.
*/

/*
 * Initialize the interrupt controllers.
 */
void
isa_init8259s()
{
  /* initialize 8259's */
  outb(IO_ICU1, 0x19);		   /* reset; four bytes, level triggered */
  outb(IO_ICU1+1, ICU_OFFSET);	   /* int base: not used */
  outb(IO_ICU1+1, 1 << IRQ_SLAVE); /* slave on line 2 */
  outb(IO_ICU1+1, 2 | 1);	   /* auto EOI, 8086 mode */
  outb(IO_ICU1+1, 0xff);	   /* disable all interrupts */
  outb(IO_ICU1, 0x68);		   /* special mask mode (if available) */
  outb(IO_ICU1, 0x0a);		   /* Read IRR, not ISR */

  outb(IO_ICU2, 0x19);		   /* reset; four bytes, level triggered */
  outb(IO_ICU2+1, ICU_OFFSET+8);   /* int base + offset for master: not used */
  outb(IO_ICU2+1, IRQ_SLAVE);      /* who ami i? */
  outb(IO_ICU2+1, 2 | 1);	   /* auto EOI, 8086 mode */
  outb(IO_ICU2+1, 0xff);	   /* disable all interrupts */
  outb(IO_ICU2, 0x68);		   /* special mask mode (if available) */
  outb(IO_ICU2, 0x0a);		   /* Read IRR by default. */

  i8259_mask = 0x0000ffff;         /* everything disabled */
}

#define	LEGAL_IRQ(x)	((x) >= 0 && (x) < ICU_LEN && (x) != 2)

const struct evcnt *
isa_intr_evcnt(isa_chipset_tag_t ic, int irq)
{

	/* XXX for now, no evcnt parent reported */
	return NULL;
}

/*
 * Set up an interrupt handler to start being called.
 */
void *
isa_intr_establish(ic, irq, type, level, ih_fun, ih_arg)
	isa_chipset_tag_t ic;
	int irq;
	int type;
	int level;
	int (*ih_fun) __P((void *));
	void *ih_arg;
{
	    irqhandler_t *ih;

	    /* no point in sleeping unless someone can free memory. */
	    ih = malloc(sizeof *ih, M_DEVBUF, cold ? M_NOWAIT : M_WAITOK);
	    if (ih == NULL)
		panic("isa_intr_establish: can't malloc handler info");

	    if (!LEGAL_IRQ(irq) || type == IST_NONE)
		panic("intr_establish: bogus irq or type");

	    /* Note: sequoia doesn't allow configuration of edge vs. level
	       on an IRQ-by-IRQ basis.  */
	    if (type != IST_LEVEL)
	      printf("WARNING: irq %d not level triggered\n", irq);

	    memset(ih, 0, sizeof *ih);
	    ih->ih_func = ih_fun;
	    ih->ih_arg = ih_arg;
	    ih->ih_level = level;
	    ih->ih_name = "isa intr";

	    if (irq_claim(irq, ih) == -1)
		panic("isa_intr_establish: can't install handler");

	    return (ih);
}


/*
 * Deregister an interrupt handler.
 */
void
isa_intr_disestablish(ic, arg)
	isa_chipset_tag_t ic;
	void *arg;
{
    panic("isa_intr_disestablish");
}

/* isa_init() might eventually become the ISA attach routine */
void
isa_init(vm_offset_t isa_io_addr, vm_offset_t isa_mem_addr)
{
  /* initialize the bus space functions */
  isa_io_init(isa_io_addr, isa_mem_addr);

  /* Clear the IRQ/FIQ masks */
  isa_init8259s();

  /* Initialize the ISA interrupt handling code */
  irq_init();
}

void
isa_attach_hook(parent, self, iba)
        struct device *parent, *self;
        struct isabus_attach_args *iba;
{

	/*
	 * Since we can only have one ISA bus, we just use a single
	 * statically allocated ISA chipset structure.  Pass it up
	 * now.
	 */
	iba->iba_ic = &isa_chipset_tag;
}
