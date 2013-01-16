/*	$NetBSD: eppcicvar.h,v 1.1.120.2 2013/01/16 05:32:46 yamt Exp $	*/

/*
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

#ifndef	_EPPCICVAR_H_
#define	_EPPCICVAR_H_

/* socket_type */
#define	SLOT_DISABLE	0
#define	CF_MODE		1
#define	PCMCIA_MODE	2
/* power_capability */
#define	VCC_5V		(1<<0)
#define	VCC_3V		(1<<1)
/* power_ctl */
#define	POWER_ON	1
#define	POWER_OFF	0

struct eppcic_chipset_tag {
	int	(*socket_type)(void *, int socket);
	int	(*power_capability)(void *, int socket);
	int	(*power_ctl)(void *, int socket, int onoff);
};
typedef struct eppcic_chipset_tag *eppcic_chipset_tag_t;

#define	EP93XX_PCMCIA_NSOCKET	1

struct eppcic_handle;

struct eppcic_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	struct eppcic_handle	*sc_ph[EP93XX_PCMCIA_NSOCKET];
	eppcic_chipset_tag_t	sc_pcic;
	uint32_t		sc_hclk;
	int			sc_enable;
	struct epgpio_softc	*sc_gpio;
};

void eppcic_attach_common(device_t, device_t, void *,
			  eppcic_chipset_tag_t);

#endif	/* _EPPCICVAR_H_ */
