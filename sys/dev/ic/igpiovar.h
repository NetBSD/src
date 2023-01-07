/* $NetBSD: igpiovar.h,v 1.2 2023/01/07 03:27:01 msaitoh Exp $ */

/*
 * Copyright (c) 2021 Emmanuel Dreyfus
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

#ifndef _IGPIOVAR_H
#define _IGPIOVAR_H

struct igpio_softc {
	device_t		sc_dev;
	const char		*sc_acpi_hid;
	bus_space_tag_t		sc_bst;
	int			sc_nbar;
	bus_addr_t		*sc_base;
	bus_size_t		*sc_length;
	bus_space_handle_t	*sc_bsh;

	gpio_pin_t		*sc_pins;
	int			sc_npins;
	struct gpio_chipset_tag	sc_gc;

	uint32_t		sc_reserved_mask;

	struct igpio_bank	*sc_banks;
};

void igpio_attach(struct igpio_softc *);
void igpio_detach(struct igpio_softc *);

int	igpio_pin_read(void *, int);
void	igpio_pin_write(void *, int, int);
void	igpio_pin_ctl(void *, int, int);
void	*igpio_intr_establish(void *, int, int, int, int (*)(void *), void *);
void	igpio_intr_disestablish(void *, void *);
bool	igpio_intr_str(void *, int, int, char *, size_t);
int	igpio_intr(void *);



#endif /* !_IGPIOVAR_H */
