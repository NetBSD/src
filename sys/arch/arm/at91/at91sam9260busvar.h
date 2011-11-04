/*	$NetBSD: at91sam9260busvar.h,v 1.1 2011/11/04 17:20:54 aymeric Exp $ */

/*
 * Copyright (c) 2007 Embedtronics Oy
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
 *
 * THIS SOFTWARE IS PROVIDED BY ICHIRO FUKUHARA ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL ICHIRO FUKUHARA OR THE VOICES IN HIS HEAD BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/* Adaptation to AT91SAM9260 by Aymeric Vincent is in the public domain */

#ifndef _AT91SAM9260BUSVAR_H_
#define _AT91SAM9260BUSVAR_H_

#include <arm/at91/at91busvar.h>
#include <arm/at91/at91pmcvar.h>
#include <arm/at91/at91aicvar.h>
#include <arm/at91/at91dbguvar.h>
#include <arm/at91/at91sam9260reg.h>

#include "at91dbgu.h"

void at91sam9260bus_init(struct at91bus_clocks *);
#if NAT91DBGU > 0
#define	at91sam9260bus_attach_cn		at91dbgu_attach_cn
#else
void at91sam9260bus_attach_cn(bus_space_tag_t iot, int ospeed, int cflag);
#endif
const struct pmap_devmap *at91sam9260bus_devmap(void);
void at91sam9260bus_peripheral_clock(int pid, int enable);
at91pio_port at91sam9260bus_pio_port(int pid);
uint32_t at91sam9260bus_gpio_mask(int pid);
#define	at91sam9260bus_intr_init		at91aic_init
#define	at91sam9260bus_intr_establish	at91aic_intr_establish
#define	at91sam9260bus_intr_disestablish	at91aic_intr_disestablish
#define	at91sam9260bus_intr_poll		at91aic_intr_poll
#define	at91sam9260bus_intr_dispatch	at91aic_intr_dispatch
const char *at91sam9260bus_peripheral_name(int pid);
void at91sam9260bus_search_peripherals(device_t self,
				   device_t (*found_func)(device_t, bus_addr_t, int));

extern const struct at91bus_machdep at91sam9260bus;

#endif	// _AT91SAM9260BUSVAR_H_
