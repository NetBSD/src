/*	$NetBSD: gumstixvar.h,v 1.2 2006/10/17 17:06:22 kiyohara Exp $ */
/*
 * Copyright (C) 2005, 2006 WIDE Project and SOUM Corporation.
 * All rights reserved.
 *
 * Written by Takashi Kiyohara and Susumu Miki for WIDE Project and SOUM
 * Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the name of SOUM Corporation
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT and SOUM CORPORATION ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT AND SOUM CORPORATION
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef _EVBARM_GUMSTIXVAR_H_
#define _EVBARM_GUMSTIXVAR_H_

#include <sys/conf.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <arm/xscale/pxa2x0_gpio.h>
#include <evbarm/gumstix/gumstixreg.h>


extern uint32_t system_serial_high;
extern uint32_t system_serial_low;


struct gxio_softc {
	struct device sc_dev;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
};

typedef void *gxio_chipset_tag_t;

struct gxio_attach_args {
	gxio_chipset_tag_t gxa_sc;
	bus_space_tag_t gxa_iot; 	/* bus tag */
	bus_addr_t gxa_addr;		/* I/O address */
	int gxa_gpirq;			/* IRQ on GPIO */
};

/*
 * IRQ handler
 */
#define gxio_intr_establish(sc, gpirq, level, spl, func, arg) \
    pxa2x0_gpio_intr_establish((gpirq), (level), (spl), (func), (arg))
#define gxio_intr_disestablish(sc, cookie) \
    pxa2x0_gpio_intr_disestablish((cookie))

#endif /* _EVBARM_GUMSTIXVAR_H_ */
