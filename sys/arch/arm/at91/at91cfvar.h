/*	$Id: at91cfvar.h,v 1.2 2008/07/03 01:15:38 matt Exp $	*/
/*	$NetBSD: at91cfvar.h,v 1.2 2008/07/03 01:15:38 matt Exp $	*/

/*
 * Copyright (c) 2007 Embedtronics Oy. All rights reserved.
 *
 * Based on arch/arm/ep93xx/eppcicvar.h,
 * Copyright (c) 2005 HAMAJIMA Katsuomi. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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
 */

#ifndef	_AT91CFVAR_H_
#define	_AT91CFVAR_H_

struct at91cf_chipset_tag {
	int	(*power_ctl)(void *self, int onoff);
#define	POWER_OFF	0
#define	POWER_ON	1
	int	(*card_detect)(void *self);
	int	(*irq_line)(void *self);
	void *	(*intr_establish)(void *self, int which, int ipl, int (*ih_func)(void *), void *arg);
	void	(*intr_disestablish)(void *self, int wich, void *cookie);
#define	CD_IRQ	0	// card detect change interrupt
#define	CF_IRQ	1	// compact flash card interrupt
};

typedef struct at91cf_chipset_tag *at91cf_chipset_tag_t;

struct at91cf_handle;

struct at91cf_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_memory_ioh, sc_common_ioh, sc_io_ioh;
	struct at91cf_handle	*sc_ph;
	at91cf_chipset_tag_t	sc_cscf;
	int			sc_enable;
};

void at91cf_attach_common(device_t, device_t, void *,
			   at91cf_chipset_tag_t);

#endif	/* _AT91CFVAR_H_ */
