/*	$NetBSD: iomdvar.h,v 1.1.4.2 2002/06/20 03:38:10 nathanw Exp $	*/

/*
 * Copyright (c) 1997 Mark Brinicombe.
 * Copyright (c) 1997 Causality Limited
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
 *	This product includes software developed by Mark Brinicombe
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
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
 * iomd_bus.h
 *
 * Created      : 02/02/97
 */

#include <machine/bus.h>

/*
 * Attach args for iomd_clock device
 */

struct clk_attach_args {
	const char		*ca_name;	/* device name*/
	bus_space_tag_t		ca_iot;		/* Bus tag */
	bus_space_handle_t	ca_ioh;		/* Bus handle */
	int			ca_irq;		/* IRQ number */
};


/*
 * Attach args for iomd device
 */

/*
 * Attach args for qms device
 */

struct qms_attach_args {
	const char		*qa_name;	/* device name*/
	bus_space_tag_t		qa_iot;		/* Bus tag */
	bus_space_handle_t	qa_ioh;		/* Bus handle */
	bus_space_handle_t	qa_ioh_but;	/* Bus handle */
	int			qa_irq;		/* IRQ number */
};

/*
 * Attach args for opms device
 */

struct opms_attach_args {
	const char		*pa_name;	/* device name*/
	bus_space_tag_t		pa_iot;		/* Bus tag */
	bus_space_handle_t	pa_ioh;		/* Bus handle */
	int			pa_irq;		/* IRQ number */
};

/*
 * Attach args for kbd device
 */

struct kbd_attach_args {
	const char		*ka_name;	/* device name*/
	bus_space_tag_t		ka_iot;		/* Bus tag */
	bus_space_handle_t	ka_ioh;		/* Bus handle */
	int			ka_rxirq;	/* IRQ number */
	int			ka_txirq;	/* IRQ number */
};

/*
 * Attach args for iic device
 */

struct iic_attach_args {
	const char		*ia_name;	/* device name */
	bus_space_tag_t		ia_iot;		/* Bus tag */
	bus_space_handle_t	ia_ioh;		/* Bus handle */
	int			ia_irq;		/* IRQ number */
};

/*
 * NOTE: All these attach structures and a const char *name as the
 * first field for identification.
 */

union iomd_attach_args {
	struct kbd_attach_args		ia_kbd;
	struct opms_attach_args		ia_opms;
	struct qms_attach_args		ia_qms;
	struct iic_attach_args		ia_iic;
	struct clk_attach_args		ia_clk;
};

/*
 * IOMD_BASE register variable
 */
extern u_int32_t iomd_base;

/* End of iomdvar.h */
