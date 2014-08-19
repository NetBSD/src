/*	$NetBSD: bcm2835_gpio_subr.c,v 1.2.8.3 2014/08/20 00:02:45 tls Exp $	*/

/*
 * Copyright (c) 2013 Jonathan A. Kollasch
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: bcm2835_gpio_subr.c,v 1.2.8.3 2014/08/20 00:02:45 tls Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/bus.h>
#include <sys/intr.h>

#include <arm/broadcom/bcm_amba.h>
#include <arm/broadcom/bcm2835reg.h>
#include <arm/broadcom/bcm2835_gpioreg.h>
#include <arm/broadcom/bcm2835_gpio_subr.h>

void
bcm2835gpio_function_select(u_int pin, u_int func)
{
	const bus_space_tag_t iot = &bcm2835_bs_tag;
	const bus_space_handle_t ioh = BCM2835_IOPHYSTOVIRT(BCM2835_GPIO_BASE);
	const u_int mask = (1 << BCM2835_GPIO_GPFSEL_BITS_PER_PIN) - 1;
	const u_int regid = (pin / BCM2835_GPIO_GPFSEL_PINS_PER_REGISTER);
	const u_int shift = (pin % BCM2835_GPIO_GPFSEL_PINS_PER_REGISTER) *
	    BCM2835_GPIO_GPFSEL_BITS_PER_PIN;
	uint32_t v;

	KASSERT(func <= mask);

	v = bus_space_read_4(iot, ioh, BCM2835_GPIO_GPFSEL(regid));

	if (((v >> shift) & mask) == func) {
		return;
	}

	aprint_debug("bcm2835: changing FSEL%u to %#o\n", pin, func);

	v &= ~(mask << shift);
	v |=  (func << shift);

	bus_space_write_4(iot, ioh, BCM2835_GPIO_GPFSEL(regid), v);
}

u_int
bcm2835gpio_function_read(u_int pin)
{
	const bus_space_tag_t iot = &bcm2835_bs_tag;
	const bus_space_handle_t ioh = BCM2835_IOPHYSTOVIRT(BCM2835_GPIO_BASE);
	const u_int mask = (1 << BCM2835_GPIO_GPFSEL_BITS_PER_PIN) - 1;
	const u_int regid = (pin / BCM2835_GPIO_GPFSEL_PINS_PER_REGISTER);
	const u_int shift = (pin % BCM2835_GPIO_GPFSEL_PINS_PER_REGISTER) *
	    BCM2835_GPIO_GPFSEL_BITS_PER_PIN;
	uint32_t v;

	v = bus_space_read_4(iot, ioh, BCM2835_GPIO_GPFSEL(regid));

	return ((v >> shift) & mask);
}
