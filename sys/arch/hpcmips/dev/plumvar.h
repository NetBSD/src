/*	$NetBSD: plumvar.h,v 1.1 1999/11/21 06:50:26 uch Exp $ */

/*
 * Copyright (c) 1999, by UCHIYAMA Yasushi
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the developer may NOT be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

typedef u_int32_t plumreg_t;

plumreg_t	plum_conf_read __P((bus_space_tag_t, bus_space_handle_t, int));
void		plum_conf_write __P((bus_space_tag_t, bus_space_handle_t, int, plumreg_t));

struct plum_chipset_tag {
	tx_chipset_tag_t pc_tc; /* TX39 layer tag */
	void *pc_intrt;  /* Plum2 interrupt tag */
	void *pc_powert; /* Plum2 power/clock tag */
};

typedef struct plum_chipset_tag *plum_chipset_tag_t;

struct plum_attach_args {
	plum_chipset_tag_t	pa_pc;
	bus_space_tag_t		pa_regt;
	bus_space_tag_t		pa_iot;
	bus_space_tag_t		pa_memt;
	int			pa_irq;
};

void	plum_conf_register_intr __P((plum_chipset_tag_t, void*));
void	plum_conf_register_power __P((plum_chipset_tag_t, void*));
