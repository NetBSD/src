/*	$NetBSD: vripvar.h,v 1.5.4.2 2002/02/28 04:10:07 nathanw Exp $	*/

/*-
 * Copyright (c) 1999, 2002
 *         Shin Takemura and PocketBSD Project. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#ifndef _VRIPVAR_H_
#define _VRIPVAR_H_

#include <hpcmips/vr/vripif.h>

struct vrip_unit {
	char	*vu_name;
	int	vu_intr[2];
	int	vu_clkmask;
	bus_addr_t	vu_lreg;
	bus_addr_t	vu_mlreg;
	bus_addr_t	vu_hreg;
	bus_addr_t	vu_mhreg;
};

struct vrip_softc {
	struct	device sc_dv;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
	hpcio_chip_t sc_gpio_chips[VRIP_NIOCHIPS];
	vrcmu_chipset_tag_t sc_cc;
	int sc_pri; /* attaching device priority */
	u_int32_t sc_intrmask;
	struct vrip_chipset_tag sc_chipset;
	const struct vrip_unit *sc_units;
	int sc_nunits;
	bus_addr_t sc_icu_addr;
	int sc_sysint2;
	int sc_msysint2;
	struct intrhand {
		int	(*ih_fun)(void *);
		void	*ih_arg;
		const struct vrip_unit *ih_unit;
	} sc_intrhands[32];
};

void vrip_intr_suspend(void);
void vrip_intr_resume(void);
int vripmatch(struct device *, struct cfdata *, void *);
void vripattach_common(struct device *, struct device *, void *);

#endif /* !_VRIPVAR_H_ */
