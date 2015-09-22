/* $Id: arbusvar.h,v 1.4.30.1 2015/09/22 12:05:46 skrll Exp $ */
/*
 * Copyright (c) 2006 Urbana-Champaign Independent Media Center.
 * Copyright (c) 2006 Garrett D'Amore.
 * All rights reserved.
 *
 * This code was written by Garrett D'Amore for the Champaign-Urbana
 * Community Wireless Network Project.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgements:
 *      This product includes software developed by the Urbana-Champaign
 *      Independent Media Center.
 *	This product includes software developed by Garrett D'Amore.
 * 4. Urbana-Champaign Independent Media Center's name and Garrett
 *    D'Amore's name may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE URBANA-CHAMPAIGN INDEPENDENT
 * MEDIA CENTER AND GARRETT D'AMORE ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE URBANA-CHAMPAIGN INDEPENDENT
 * MEDIA CENTER OR GARRETT D'AMORE BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef	_MIPS_ATHEROS_ARBUSVAR_H_
#define	_MIPS_ATHEROS_ARBUSVAR_H_

#include <sys/bus.h>
#include <sys/cpu.h>

#include <mips/cpuregs.h>

struct arbus_attach_args {
	const char	*aa_name;
	bus_space_tag_t	aa_bst;
	bus_space_tag_t	aa_bst_le;
	bus_dma_tag_t	aa_dmat;

	bus_addr_t	aa_addr;
	bus_size_t	aa_size;
	int		aa_cirq;	/* cpu irq */
	int		aa_mirq;	/* misc irq */
};

void arbus_init(void);
void arbusle_bus_mem_init(bus_space_tag_t, void *);
bus_space_tag_t arbus_get_bus_space_tag(void);
bus_dma_tag_t arbus_get_bus_dma_tag(void);

void *arbus_intr_establish(int, int, int (*)(void *), void *);
void arbus_intr_disestablish(void *);

void com_arbus_cnattach(bus_addr_t, uint32_t);

/*
 * These definitions are "funny".  To distinquish between
 * miscellaneous IRQs and global processor interrupts, we encode the
 * IRQ as (cpuirq) | ((miscirq) << 4).
 *
 * Note that these IRQs seem to be general on all AR531X based parts.
 */

#define	ARBUS_IRQ_CPU_M		0xf
#define	ARBUS_IRQ_CPU_S		0
#define	ARBUS_IRQ_MISC_M	0xf0
#define	ARBUS_IRQ_MISC_S	4
#define	ARBUS_IRQ_CPU(x)	(((x) & ARBUS_IRQ_CPU_M) >> ARBUS_IRQ_CPU_S)
#define	ARBUS_IRQ_MISC(x)	(((x) & ARBUS_IRQ_MISC_M) >> ARBUS_IRQ_MISC_S)
#define	ARBUS_MAKE_IRQ(c,m)	\
	(((c) << ARBUS_IRQ_CPU_S) | ((m) << ARBUS_IRQ_MISC_S))

#define	ARBUS_IRQ_WLAN0		ARBUS_MAKE_IRQ(0,0)
#define	ARBUS_IRQ_ENET0		ARBUS_MAKE_IRQ(1,0)
#define	ARBUS_IRQ_ENET1		ARBUS_MAKE_IRQ(2,0)
#define	ARBUS_IRQ_WLAN1		ARBUS_MAKE_IRQ(3,0)
#define	ARBUS_IRQ_TIMER		ARBUS_MAKE_IRQ(4,0)
#define	ARBUS_IRQ_AHBPERR	ARBUS_MAKE_IRQ(4,1)
#define	ARBUS_IRQ_AHBDMAE	ARBUS_MAKE_IRQ(4,2)
#define	ARBUS_IRQ_GPIO		ARBUS_MAKE_IRQ(4,3)
#define	ARBUS_IRQ_UART0		ARBUS_MAKE_IRQ(4,4)
#define	ARBUS_IRQ_UART0_DMA	ARBUS_MAKE_IRQ(4,5)
#define	ARBUS_IRQ_WDOG		ARBUS_MAKE_IRQ(4,6)

#endif /* _MIPS_ATHEROS_ARBUSVAR_H_ */
