/* $NetBSD: s3c24x0_spi.h,v 1.2.118.1 2012/02/18 07:31:31 mrg Exp $ */

/*
 * Copyright (c) 2004  Genetec corporation.  All rights reserved.
 * Written by Hiroyuki Bessho for Genetec corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of Genetec corporation may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY GENETEC CORP. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GENETEC CORP.
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _S3C2410_SPI_H_
#define _S3C2410_SPI_H_

#include <arm/s3c2xx0/s3c24x0var.h>

struct ssspi_softc;

/*
 * attach arguments for sub-devices hooked to SPI ports.
 */
struct ssspi_attach_args {
	s3c2xx0_chipset_tag_t	spia_sc;
	bus_space_tag_t  	spia_iot;
	bus_space_handle_t	spia_ioh;   /* SPI controller registers */
	bus_space_handle_t	spia_gpioh; /* GPIO registers. SPI devices often
					       needs additional pins */
	bus_dma_tag_t    	spia_dmat;
	short			spia_intr;  /* interrupt from SPI */
	short			spia_index; /* index number of SPI unit (0|1) */
	short			spia_aux_intr; /* additional interrupt */
};


int s3c24x0_spi_setup(struct ssspi_softc *, uint32_t, int, int);
int s3c24x0_spi_master_send(struct ssspi_softc *, uint8_t);
int s3c24x0_spi_wait(struct ssspi_softc *, uint8_t*);
void s3c24x0_spi_spin_wait(struct ssspi_softc *sc);
int s3c24x0_spi_bps(struct ssspi_softc *sc, int bps);


#endif /* _S3C2410_SPI_H_ */
