/* $NetBSD: pxa2x0var.h,v 1.1.2.2 2002/11/11 21:57:03 nathanw Exp $ */

/*
 * Copyright (c) 2002  Genetec Corporation.  All rights reserved.
 * Written by Hiroyuki Bessho for Genetec Corporation.
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
 *	This product includes software developed for the NetBSD Project by
 *	Genetec Corporation.
 * 4. The name of Genetec Corporation may not be used to endorse or 
 *    promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY GENETEC CORPORATION ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GENETEC CORPORATION
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */


#ifndef _ARM_XSCALE_PXA2X0VAR_H_
#define _ARM_XSCALE_PXA2X0VAR_H_

#include <arm/sa11x0/sa11x0_var.h>

/* PXA2X0's integrated peripheral bus. 
 *
 * pxaip_softc `inherits' sa11x0_softc.  This is a hack to make OS
 * timer driver for SA11x0 (arm/sa11x0/sa11x0_ost.c) work on PXA2X0.
 * PXA2X0 has a same OS timer unit as SA11X0 has.
 */

typedef int (* pxa2x0_irq_handler_t)(void *);

struct pxa2x0_softc {
	struct sa11x0_softc	saip;

	bus_space_handle_t	sc_memctl_ioh; /* Memory controller */
	bus_space_handle_t	sc_clkman_ioh; /* Clock manager */
	bus_space_handle_t	sc_rtc_ioh; /* real time clock */
};

extern struct pxa2x0_softc *pxa2x0_softc;

struct pxa2x0_attach_args {
	struct sa11x0_attach_args  pxa_sa;

	int pxa_index;			/* to specify device by index number */

#define pxa_sc   	pxa_sa.sa_sc
#define pxa_iot 	pxa_sa.sa_iot
#define pxa_addr	pxa_sa.sa_addr
#define pxa_size	pxa_sa.sa_size
#define pxa_intr	pxa_sa.sa_intr
#define pxa_gpio	pxa_sa.sa_gpio
};


extern struct bus_space pxa2x0_bs_tag;
extern struct arm32_bus_dma_tag pxa2x0_bus_dma_tag;
extern struct bus_space pxa2x0_a4x_bs_tag;

void pxa2x0_set_intcbase(vaddr_t);
void pxa2x0_intr_init(void);
void *pxa2x0_intr_establish(int irqno, int level,
			    int (*func)(void *), void *cookie);
void pxa2x0_update_intr_masks( int irqno, int level );
extern int current_spl_level;

/* Integrated UART */
enum pxa2x0_uart_id { UART_FFUART, UART_BTUART, UART_STUART };
extern void com_pxaip_setup( struct pxa2x0_softc *, enum pxa2x0_uart_id );


/* misc. */
extern int pxa2x0_measure_cpuclock( struct pxa2x0_softc * );
extern void pxa2x0_fcs_init(void);
extern void pxa2x0_freq_change(int);
extern void pxa2x0_turbo_mode(int);
extern int pxa2x0_i2c_master_tx( int, uint8_t *, int );

void pxa2x0_irq_handler(struct clockframe *);


#endif /* _ARM_XSCALE_PXA2X0VAR_H_ */
