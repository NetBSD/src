/* $NetBSD: jenseniovar.h,v 1.1.2.2 2000/07/12 20:59:14 thorpej Exp $ */

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _ALPHA_JENSENIO_JENSENIOVAR_H_
#define _ALPHA_JENSENIO_JENSENIOVAR_H_

/*
 * Arguments used to attach devices to the Jensen I/O bus.
 */
struct jensenio_attach_args {
	union {
		struct eisabus_attach_args jaun_eisa;
		struct isabus_attach_args jaun_isa;
	} ja_un;

	bus_addr_t ja_ioaddr;		/* I/O space address */
	int ja_irq[2];			/* Jensen IRQs */

/* For attaching busses. */
#define	ja_eisa		ja_un.jaun_eisa
#define	ja_isa		ja_un.jaun_isa

/* For attaching built-in devices. */
#define	ja_name		ja_eisa.eba_busname
#define	ja_iot		ja_eisa.eba_iot
#define	ja_ec		ja_eisa.eba_ec
};

/*
 * The Jensen I/O bus configuration.
 *
 * All of the information that the chipset-specific functions need to
 * do their dirty work (and more!)
 */
struct jensenio_config {
	int	jc_initted;

	struct alpha_bus_space jc_internal_iot, jc_eisa_iot, jc_eisa_memt;
	struct alpha_eisa_chipset jc_ec;
	struct alpha_isa_chipset jc_ic;

	struct alpha_bus_dma_tag jc_dmat_eisa, jc_dmat_isa;

	struct extent *jc_io_ex, *jc_s_mem_ex;

	int	jc_mallocsafe;
};

void	jensenio_init(struct jensenio_config *, int);
void	jensenio_bus_io_init(bus_space_tag_t, void *);
void	jensenio_bus_intio_init(bus_space_tag_t, void *);
void	jensenio_bus_mem_init(bus_space_tag_t, void *);
void	jensenio_intr_init(struct jensenio_config *);
void	jensenio_dma_init(struct jensenio_config *);
#endif /* _ALPHA_JENSENIO_JENSENIOVAR_H_ */
