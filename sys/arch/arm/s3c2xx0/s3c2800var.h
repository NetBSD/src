/* $NetBSD: s3c2800var.h,v 1.3 2003/05/12 07:49:10 bsh Exp $ */

/*
 * Copyright (c) 2002 Fujitsu Component Limited
 * Copyright (c) 2002 Genetec Corporation
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
 * 3. Neither the name of The Fujitsu Component Limited nor the name of
 *    Genetec corporation may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY FUJITSU COMPONENT LIMITED AND GENETEC
 * CORPORATION ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL FUJITSU COMPONENT LIMITED OR GENETEC
 * CORPORATION BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _ARM_S3C2800VAR_H_
#define _ARM_S3C2800VAR_H_

#include <arm/s3c2xx0/s3c2xx0var.h>
#include <sys/termios.h>
#include <dev/pci/pcivar.h>

struct s3c2800_softc {
	struct s3c2xx0_softc  sc_sx;

	bus_space_handle_t sc_tmr0_ioh;
	bus_space_handle_t sc_tmr1_ioh;
};

void	s3c2800_softreset(void);
void	*s3c2800_intr_establish(int, int, int, s3c2xx0_irq_handler_t, void *);
void	s3c2800_intr_init(struct s3c2800_softc *);
int	s3c2800_sscom_cnattach(bus_space_tag_t, int, int, int, tcflag_t);
int	s3c2800_sscom_kgdb_attach(bus_space_tag_t, int, int, int, tcflag_t);
void	s3c2800_pci_init(pci_chipset_tag_t, void *);

/* Platform provides this */
bus_dma_tag_t s3c2800_pci_dma_init(void);

#endif /* _ARM_S3C2800VAR_H_ */
