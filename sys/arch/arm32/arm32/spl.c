/* $NetBSD: spl.c,v 1.1 1996/01/31 23:17:02 mark Exp $ */

/*
 * Copyright (c) 1994,1995 Mark Brinicombe.
 * Copyright (c) 1994 Brini.
 * All rights reserved.
 *
 * This code is derived from software written for Brini by Mark Brinicombe
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
 *	This product includes software developed by Brini.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BRINI ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL BRINI OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * RiscBSD kernel project
 *
 * spl.c
 *
 * spl routines
 *
 * Created      : 30/09/94
 * Last updated : 28/08/95
 *
 *    $Id: spl.c,v 1.1 1996/01/31 23:17:02 mark Exp $
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <machine/irqhandler.h>
#include <machine/iomd.h>
#include <machine/cpu.h>

extern u_int spl_mask;

int safepri = 0xffffffff;

int spl_debug = 0;

/*
 * u_int spl(u_int mask)
 *
 * And's bits out of the current spl mask and returns the previous mask.
 * The IOMD mask registers are then updated to reflect the allowable IRQ's
 * Eventually this should be done in assembly as the swap instruction
 * can be used to speed things up.
 */
 
u_int
spl(mask)
	u_int mask;
{
	register u_int oldspl;
	register u_int oldirqstate;

/*
 * The mask indicates which IRQ's are not allowable.
 * Each bit that is zero means that that interrupt is not allowed.
 * The spl_mask is updated so that these interrupts will not be allowed.
 * This routine only disables interrupts, it does not enable them.
 * The routines splx or sple should be used for that.
 */

	oldirqstate = disable_interrupts(I32_bit);

	if (spl_debug) 
		printf("spl: %08x %08x\n", spl_mask, mask);

	oldspl = spl_mask;
	spl_mask &= mask;
	irq_setmasks();

	restore_interrupts(oldirqstate);
	return(oldspl);
}


/*
 * u_int sple(u_int mask)
 *
 * Or's bitsinto the current spl mask and returns the previous mask.
 * The IOMD mask registers are then updated to reflect the allowable IRQ's
 * Eventually this should be done in assembly as the swap instruction
 * can be used to speed things up.
 */
 
u_int
sple(mask)
	u_int mask;
{
	register u_int oldspl;
	register u_int oldirqstate;
    
/*
 * The mask indicates which allowable interrupts should be added to
 * the current spl_mask.
 * This function only enables those interrupts specified. It does
 * not disable any interrupts. 
 */

	oldirqstate = disable_interrupts(I32_bit);

	if (spl_debug) 
		printf("sple: %08x %08x\n", spl_mask, mask);

	oldspl = spl_mask;
	spl_mask |= mask;
	irq_setmasks();

	restore_interrupts(oldirqstate);

	return(oldspl);
}


/*
 * u_int splx(u_int mask)
 *
 * Sets the current spl mask and returns the previous mask.
 * The IOMD mask registers are then updated to reflect the allowable IRQ's
 * Eventually this should be done in assembly as the swap instruction
 * can be used to speed things up.
 */

u_int
splx(mask)
	u_int mask;
{
	register u_int oldspl;
	register u_int oldirqstate;
    
	oldirqstate = disable_interrupts(I32_bit);
    
	if (spl_debug) 
		printf("splx: %08x %08x s=%08x\n", spl_mask, mask, oldirqstate);

	oldspl = spl_mask;
	spl_mask = mask;
	irq_setmasks();

	restore_interrupts(oldirqstate);
	return(oldspl);
}


/*
 * u_int spl0(void)
 *
 * Set the spl mask to enable all IRQ's to be handled
 */
 
u_int
spl0()
{
	if (spl_debug)
		printf("spl0() ");
	return(splx(0xffffffff));
}


/*
 * u_int splhigh(void)
 *
 * Set the spl mask to disable all IRQ's from being handled
 */

u_int
splhigh()
{
	if (spl_debug)
		printf("splhigh() ");
	return(splx(0x00000000));
}

  
u_int
splbio()
{
	if (spl_debug) 
		printf("splbio() ");
	return(spl(irqmasks[IPL_BIO]));
}


u_int
splnet()
{
	if (spl_debug) 
		printf("splnet() ");
	return(spl(irqmasks[IPL_NET]));
/*    return(spl(~IRQMASK_SOFTNET));*/
}


u_int
spltty()
{
	if (spl_debug) 
		printf("spltty() ");
	return(spl(irqmasks[IPL_TTY]));
}


u_int
splimp()
{
	if (spl_debug) 
		printf("splimp() ");
	return(spl(irqmasks[IPL_IMP]));
}


u_int
splclock()
{
	if (spl_debug) 
		printf("splclock() ");
	return(spl(irqmasks[IPL_CLOCK]));
}


u_int
splstatclock()
{
	if (spl_debug) 
		printf("splstatclock() ");
	return(spl(irqmasks[IPL_CLOCK]));
}


u_int
splsoftclock()
{
	if (spl_debug)
		printf("splsoftclock() ");
	return(spl_mask);
}

u_int
splsoftnet()
{
	if (spl_debug)
		printf("splsoftnet() ");
	return(spl(~IRQMASK_SOFTNET));
}

/* End of spl.c */
