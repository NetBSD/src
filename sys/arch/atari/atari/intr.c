/*	$NetBSD: intr.c,v 1.11.30.1 2007/04/10 13:22:52 ad Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Adam Glass, Gordon W. Ross, Jason R. Thorpe, and Leo Weppelman.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: intr.c,v 1.11.30.1 2007/04/10 13:22:52 ad Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/vmmeter.h>
#include <sys/queue.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>

#include <machine/cpu.h>

#include <atari/atari/intr.h>

#define	AVEC_MIN	1
#define	AVEC_MAX	7
#define	AVEC_LOC	25
#define	UVEC_MIN	0
#define	UVEC_MAX	191
#define	UVEC_LOC	64

typedef LIST_HEAD(, intrhand) ih_list_t;
ih_list_t autovec_list[AVEC_MAX - AVEC_MIN + 1];
ih_list_t uservec_list[UVEC_MAX - UVEC_MIN + 1];

void
intr_init()
{
	int i;

	for (i = 0; i < (AVEC_MAX - AVEC_MIN + 1); ++i) {
		LIST_INIT(&autovec_list[i]);
	}
	for (i = 0; i < (UVEC_MAX - UVEC_MIN + 1); ++i) {
		LIST_INIT(&uservec_list[i]);
	}
}

/*
 * Establish an interrupt vector.
 *   - vector
 *	The vector numer the interrupt should be hooked on. It can either
 *	be an auto-vector or a user-vector.
 *   - type
 *	A bit-wise of:
 *		- AUTO_VEC (mutually exclusive with USER_VEC)
 *			Attach to one of the 7 auto vectors
 *		- USER_VEC (mutually exclusive with AUTO_VEC)
 *			Attach to one of the 192 user vectors
 *		- FAST_VEC
 *			The interrupt function 'ih_fun' will be
 *			put into the 'real' interrupt table. This
 *			means:
 *				- This vector can't be shared
 *				- 'ih_fun' must save registers
 *				- 'ih_fun' must do it's own interrupt accounting
 *				- The argument to 'ih_fun' is a standard
 *				  interrupt frame.
 *		- ARG_CLOCKRAME
 *			The 'ih_fun' function will be called with
 *			a standard clock-frame instead of 'ih_arg'.
 *
 *   - pri
 *	When multiple interrupts are established on the same vector,
 *	interrupts with the highest priority will be called first. The
 *	basic ordering is the order of establishment.
 *   - ih_fun
 *	The interrupt function to be called
 *   - ih_arg
 *	The argument given to 'ih_fun' when ARG_CLOCKFRAME is not
 *	specified.
 */

struct intrhand *
intr_establish(vector, type, pri, ih_fun, ih_arg)
	void		*ih_arg;
        int		vector;
        int		type;
        int		pri;
        hw_ifun_t	ih_fun;
{
	struct intrhand	*ih, *cur_vec;
	ih_list_t	*vec_list;
	u_long		*hard_vec;
	int		s;

	/* no point in sleeping unless someone can free memory. */
	ih = malloc(sizeof *ih, M_DEVBUF, cold ? M_NOWAIT : M_WAITOK);
	if (ih == NULL)
		panic("intr_establish: can't malloc handler info");

	/*
	 * Initialize vector info
	 */
	ih->ih_fun    = ih_fun;
	ih->ih_arg    = ih_arg;
	ih->ih_type   = type;
	ih->ih_pri    = pri;
	ih->ih_vector = vector;

	/*
	 * Do some validity checking on the 'vector' argument and determine
	 * vector list this interrupt should be on.
	 */
	switch(type & (AUTO_VEC|USER_VEC)) {
		case AUTO_VEC:
			if (vector < AVEC_MIN || vector > AVEC_MAX)
				return (NULL);
			vec_list = &autovec_list[vector-1];
			hard_vec = &autovects[vector-1];
			ih->ih_intrcnt = &intrcnt_auto[vector-1];
			break;
		case USER_VEC:
			if (vector < UVEC_MIN || vector > UVEC_MAX)
				return (NULL);
			vec_list = &uservec_list[vector];
			hard_vec = &uservects[vector];
			ih->ih_intrcnt = &intrcnt_user[vector];
			break;
		default:
			printf("intr_establish: bogus vector type\n");
			free(ih, M_DEVBUF);
			return(NULL);
	}

	/*
	 * If the vec_list is empty, we insert ourselves at the head of the
	 * list and we re-route the 'hard-vector' to the appropriate handler.
	 */
	if (vec_list->lh_first == NULL) {

		s = splhigh();
		LIST_INSERT_HEAD(vec_list, ih, ih_link);
		if (type & FAST_VEC)
			*hard_vec = (u_long)ih->ih_fun;
		else if(*hard_vec != (u_long)intr_glue) {
			/*
			 * Normally, all settable vectors are already
			 * re-routed to the intr_glue() function. The
			 * marvelous exeption to these are the HBL/VBL
			 * interrupts. They happen *very* often and
			 * can't be turned off on the Falcon. So they
			 * are normally vectored to an 'rte' instruction.
			 */
			*hard_vec = (u_long)intr_glue;
		}
			
		splx(s);

		return ih;
	}

	/*
	 * Check for FAST_VEC botches
	 */
	cur_vec = vec_list->lh_first;
	if (cur_vec->ih_type & FAST_VEC) {
		free(ih, M_DEVBUF);
		printf("intr_establish: vector cannot be shared\n");
		return (NULL);
	}

	/*
	 * We traverse the list and place ourselves after any handlers with
	 * our current (or higher) priority level.
	 */
	for (cur_vec = vec_list->lh_first; cur_vec->ih_link.le_next != NULL;
	    cur_vec = cur_vec->ih_link.le_next) {
		if (ih->ih_pri > cur_vec->ih_pri) {

			s = splhigh();
			LIST_INSERT_BEFORE(cur_vec, ih, ih_link);
			splx(s);

			return (ih);
		}
	}

	/*
	 * We're the least important entry, it seems.  We just go
	 * on the end.
	 */
	s = splhigh();
	LIST_INSERT_AFTER(cur_vec, ih, ih_link);
	splx(s);

	return ih;
}

int
intr_disestablish(ih)
struct intrhand	*ih;
{
	ih_list_t	*vec_list;
	u_long		*hard_vec;
	int		vector, s;
	struct intrhand	*cur_vec;

	vector = ih->ih_vector;
	switch(ih->ih_type & (AUTO_VEC|USER_VEC)) {
		case AUTO_VEC:
			if (vector < AVEC_MIN || vector > AVEC_MAX)
				return 0;
			vec_list = &autovec_list[vector-1];
			hard_vec = &autovects[vector-1];
			break;
		case USER_VEC:
			if (vector < UVEC_MIN || vector > UVEC_MAX)
				return 0;
			vec_list = &uservec_list[vector];
			hard_vec = &uservects[vector];
			break;
		default:
			printf("intr_disestablish: bogus vector type\n");
			return  0;
	}

	/*
	 * Check if the vector is really in the list we think it's in....
	 */
	for (cur_vec = vec_list->lh_first; cur_vec->ih_link.le_next != NULL;
	    cur_vec = cur_vec->ih_link.le_next) {
		if (ih == cur_vec)
			break;
	}
	if (ih != cur_vec) {
		printf("intr_disestablish: 'ih' has inconsistent data\n");
		return 0;
	}

	s = splhigh();
	LIST_REMOVE(ih, ih_link);
	if ((vec_list->lh_first == NULL) && (ih->ih_type & FAST_VEC))
		*hard_vec = (u_long)intr_glue;
	splx(s);

	free(ih, M_DEVBUF);
	return 1;
}

/*
 * This is the dispatcher called by the low-level
 * assembly language interrupt-glue routine.
 */
void
intr_dispatch(frame)
struct clockframe	frame;
{
	static int	unexpected, straycount;
	int		vector;
	int		handled = 0;
	ih_list_t	*vec_list;
	struct intrhand	*ih;

	uvmexp.intrs++;
	vector = (frame.cf_vo & 0xfff) >> 2;
	if (vector < (AVEC_LOC+AVEC_MAX) && vector >= AVEC_LOC)
		vec_list = &autovec_list[vector - AVEC_LOC];
	else if (vector <= (UVEC_LOC+UVEC_MAX) && vector >= UVEC_LOC)
		vec_list = &uservec_list[vector - UVEC_LOC];
	else panic("intr_dispatch: Bogus vector %d", vector);

	if ((ih = vec_list->lh_first) == NULL) {
		printf("intr_dispatch: vector %d unexpected\n", vector);
		if (++unexpected > 10)
		  panic("intr_dispatch: too many unexpected interrupts");
		return;
	}
	ih->ih_intrcnt[0]++;

	/* Give all the handlers a chance. */
	for ( ; ih != NULL; ih = ih->ih_link.le_next)
		handled |= (*ih->ih_fun)((ih->ih_type & ARG_CLOCKFRAME)
					? &frame : ih->ih_arg, frame.cf_sr);

	if (handled)
	    straycount = 0;
	else if (++straycount > 50)
	    panic("intr_dispatch: too many stray interrupts");
	else
	    printf("intr_dispatch: stray level %d interrupt\n", vector);
}

static const int ipl2psl_table[] = {
	[IPL_NONE]       = PSL_IPL0,
	[IPL_SOFT]       = PSL_IPL1,
	[IPL_SOFTCLOCK]  = PSL_IPL1,
	[IPL_SOFTNET]    = PSL_IPL1,
	[IPL_SOFTSERIAL] = PSL_IPL1,
	[IPL_BIO]        = PSL_IPL3,
	[IPL_NET]        = PSL_IPL3,
	[IPL_TTY]        = PSL_IPL4,
	/* IPL_LPT == IPL_TTY */
	[IPL_VM]         = PSL_IPL4,
	[IPL_SERIAL]     = PSL_IPL5,
	[IPL_CLOCK]      = PSL_IPL6,
	[IPL_HIGH]       = PSL_IPL7,
};

ipl_cookie_t
makeiplcookie(ipl_t ipl)
{

	return (ipl_cookie_t){._psl = ipl2psl_table[ipl] | PSL_S};
}
