/*	$NetBSD: loongson_intr.h,v 1.1.12.1 2014/08/20 00:02:58 tls Exp $	*/

/*-
 * Copyright (c) 2000, 2001 The NetBSD Foundation, Inc.
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

#ifndef _LOONGSON_INTR_H_
#define	_LOONGSON_INTR_H_

#include <machine/intr.h>
#include <mips/bonito/bonitoreg.h>
#include <sys/evcnt.h>

LIST_HEAD(bonito_intrhand_head, evbmips_intrhand);

struct bonito_intrhead {
	struct evcnt intr_count;
	struct bonito_intrhand_head intrhand_head;
	int refcnt;
};
extern struct bonito_intrhead bonito_intrhead[BONITO_NINTS];

/* interrupt mappings */
struct bonito_irqmap {
	const char *name;
	uint8_t irqidx;
	uint8_t flags;
};

#define IRQ_F_INVERT	0x80	/* invert polarity */
#define IRQ_F_EDGE	0x40	/* edge trigger */
#define IRQ_F_INT0	0x00	/* INT0 */
#define IRQ_F_INT1	0x01	/* INT1 */
#define IRQ_F_INT2	0x02	/* INT2 */
#define IRQ_F_INT3	0x03	/* INT3 */
#define IRQ_F_INTMASK	0x07	/* INT mask */

extern const struct bonito_irqmap loongson2e_irqmap[BONITO_NDIRECT];
extern const struct bonito_irqmap loongson2f_irqmap[BONITO_NDIRECT];

int	loongson_pci_intr_map(const struct pci_attach_args *,
	    pci_intr_handle_t *);
const char *loongson_pci_intr_string(void *, pci_intr_handle_t, char *, size_t);
const struct evcnt *loongson_pci_intr_evcnt(void *, pci_intr_handle_t);
void *	loongson_pci_intr_establish(void *, pci_intr_handle_t, int,
	    int (*)(void *), void *);
void	loongson_pci_intr_disestablish(void *, void *);
void	loongson_pci_conf_interrupt(void *, int, int, int, int, int *);
void *	loongson_pciide_compat_intr_establish(void *,
	    device_t, const struct pci_attach_args *, int,
	    int (*)(void *), void *);

const char *loongson_intr_string(const struct bonito_config *, int, char *, size_t);
#endif /* ! _LOONGSON_INTR_H_ */
