/*	$NetBSD: imxspivar.h,v 1.1.10.2 2014/08/20 00:02:46 tls Exp $	*/

/*
 * Copyright (c) 2014  Genetec Corporation.  All rights reserved.
 * Written by Hashimoto Kenichi for Genetec Corporation.
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
 * THIS SOFTWARE IS PROVIDED BY GENETEC CORPORATION ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GENETEC CORPORATION
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef	_ARM_IMX_IMXSPIVAR_H_
#define	_ARM_IMX_IMXSPIVAR_H_

#include <dev/spi/spivar.h>

typedef struct spi_chipset_tag {
	void *cookie;

	int (*spi_cs_enable)(void *, int);
	int (*spi_cs_disable)(void *, int);
} *spi_chipset_tag_t;

struct imxspi_attach_args {
	bus_space_tag_t saa_iot;
	bus_addr_t saa_addr;
	bus_size_t saa_size;
	int saa_irq;

	spi_chipset_tag_t saa_tag;
	int saa_nslaves;
	unsigned long saa_freq;

	int saa_enhanced;
};

struct imxspi_softc {
	device_t sc_dev;
	bus_space_tag_t  sc_iot;
	bus_space_handle_t sc_ioh;
	spi_chipset_tag_t sc_tag;

	struct spi_controller sc_spi;
	unsigned long sc_freq;
	struct spi_chunk *sc_wchunk;
	struct spi_chunk *sc_rchunk;
	void *sc_ih;
	struct spi_transfer *sc_transfer;
	bool  sc_running;
	SIMPLEQ_HEAD(,spi_transfer) sc_q;

	int sc_enhanced;
};

int imxspi_attach_common(device_t, struct imxspi_softc *, void *);

/*
 * defined in machine dependent code
 */
int imxspi_match(device_t, cfdata_t, void *);
void imxspi_attach(device_t, device_t, void *);

#endif	/* _ARM_IMX_IMXSPIVAR_H_ */
