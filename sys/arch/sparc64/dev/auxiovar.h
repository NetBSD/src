/*	$OpenBSD: auxiovar.h,v 1.7 2005/03/09 18:41:48 miod Exp $	*/
/*	$NetBSD: auxiovar.h,v 1.7.62.1 2015/12/27 12:09:43 skrll Exp $	*/

/*
 * Copyright (c) 2000 Matthew R. Green
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _LOCORE

/*
 * on sun4u, auxio exists with one register (LED) on the sbus, and 5
 * registers on the ebus2 (pci) (LED, PCIMODE, FREQUENCY, SCSI
 * OSCILLATOR, and TEMP SENSE.
 */

struct auxio_softc {
	device_t		sc_dev;

	kmutex_t		sc_lock;

	/* parent's tag */
	bus_space_tag_t		sc_tag;

	/* handles to the various auxio register sets */
	bus_space_handle_t	sc_led;
	bus_space_handle_t	sc_pci;
	bus_space_handle_t	sc_freq;
	bus_space_handle_t	sc_scsi;
	bus_space_handle_t	sc_temp;

	int			sc_flags;
#define	AUXIO_LEDONLY		0x1	// only sc_led is valid
#define	AUXIO_EBUS		0x2
};

#define	AUXIO_ROM_NAME		"auxio"

void auxio_attach_common(struct auxio_softc *);
int auxio_fd_control(u_int32_t);

#endif
