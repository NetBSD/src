/*	$NetBSD: hat.c,v 1.5 2007/08/03 16:02:17 jmmv Exp $	*/

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

/*
 * hat.c
 *
 * implementation of high-availability timer on SHARK
 *
 * Created      : 19/05/97
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: hat.c,v 1.5 2007/08/03 16:02:17 jmmv Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/device.h>

#include <arm/fiq.h>

#include <machine/cpu.h>
#include <machine/intr.h>
#include <machine/pio.h>
#include <arm/cpufunc.h>

#include <dev/ic/i8253reg.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>

#include "isadma.h"

#include <shark/shark/shark_fiq.h>
#include <shark/shark/sequoia.h>
#include <shark/shark/hat.h>

static int hatOn = 0;

/* interface to high-availability timer */

static void hatClkCount(int);
static void hatEnableSWTCH(void);

static void (*hatWedgeFn)(int);

extern struct fiqregs shark_fiqregs;

int
hatClkOff(void)
{
	u_int16_t    seqReg;

	if (!hatOn) return -1;
	hatOn = 0;
	hatWedgeFn = NULL;

        /* disable the SWTCH pin */

	sequoiaRead(PMC_PMCMCR2_REG, &seqReg);
        sequoiaWrite(PMC_PMCMCR2_REG, seqReg  | (PMCMCR2_M_SWTCHEN));
        
        /* turn off timer 2 */
        outb(ATSR_REG1_REG, 
	     inb(ATSR_REG1_REG) & ~((REG1_M_TMR2EN) | (REG1_M_SPKREN)));

	fiq_getregs(&shark_fiqregs);

	/* get rid of the C routine and stack */
	shark_fiqregs.fr_r9  = 0;
	shark_fiqregs.fr_r13 = 0;

	fiq_setregs(&shark_fiqregs);
#if (NISADMA > 0)
	isa_dmathaw(&isa_chipset_tag);		/* XXX */
#endif

	return 0;
}

int
hatClkOn(int count, void (*hatFn)(int), int arg,
	 unsigned char *stack, void (*wedgeFn)(int))
{
	u_int16_t    seqReg;

	if (hatOn)
		return -1;

	hatWedgeFn = wedgeFn;

#if (NISADMA > 0)
	isa_dmafreeze(&isa_chipset_tag);	/* XXX */
#endif

	fiq_getregs(&shark_fiqregs);

	/* set the C routine and stack */
	shark_fiqregs.fr_r9  = (u_int)hatFn;
	shark_fiqregs.fr_r10 = (u_int)arg;
	shark_fiqregs.fr_r13 = (u_int)stack;

	fiq_setregs(&shark_fiqregs);

	/* no debounce on SWTCH */
	sequoiaRead(PMC_DBCR_REG, &seqReg);
	sequoiaWrite(PMC_DBCR_REG, seqReg | DBCR_M_DBDIS0);

	hatEnableSWTCH(); /* enable the SWTCH -> PMI logic */

        /* turn on timer 2 */
        outb(ATSR_REG1_REG, 
	     inb(ATSR_REG1_REG) | (REG1_M_TMR2EN) | (REG1_M_SPKREN));

	/* start timer 2 running */
	hatClkCount(count);

        /* enable the SWTCH pin */
	sequoiaRead(PMC_PMCMCR2_REG, &seqReg);
        sequoiaWrite(PMC_PMCMCR2_REG, seqReg | (PMCMCR2_M_SWTCHEN));

	hatOn = 1;
	return 0;
}


int
hatClkAdjust(int count)
{
	if (!hatOn)
		return -1;

	hatClkCount(count);
	hatEnableSWTCH();

	return 0;
}

static void 
hatEnableSWTCH(void)
{
	u_int16_t    seqReg;

	/* SWTCH input causes PMI, not automatic switch to standby mode! */
	/* clearing bit 9 is bad news.  seems to enable PMI from secondary
	   activity timeout! */
	/* first setting, then clearing this bit seems to unwedge the edge
	   detect logic in the sequoia */

	sequoiaRead(PMC_PMIMCR_REG, &seqReg);
	sequoiaWrite(PMC_PMIMCR_REG, seqReg |  (PMIMCR_M_IMSKSWSTBY));
	sequoiaWrite(PMC_PMIMCR_REG, seqReg & ~(PMIMCR_M_IMSKSWSTBY));
}

void
hatUnwedge(void)
{
	static int   lastFiqsHappened = -1;
	extern int   fiqs_happened;

	if (!hatOn)
		return;

	if (lastFiqsHappened == fiqs_happened) {
		hatEnableSWTCH();
		if (hatWedgeFn)
			(*hatWedgeFn)(fiqs_happened);
	} else {
		lastFiqsHappened = fiqs_happened;
	}
}

static void
hatClkCount(int count)
{
        u_int savedints;
  
        savedints = disable_interrupts(I32_bit);

	outb(TIMER_MODE, TIMER_SEL2|TIMER_RATEGEN|TIMER_16BIT);
	outb(TIMER_CNTR2, count % 256);
	outb(TIMER_CNTR2, count / 256);

	restore_interrupts(savedints);
}

