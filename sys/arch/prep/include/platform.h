/*	$NetBSD: platform.h,v 1.1.2.1 2002/03/16 15:59:22 jdolecek Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by NONAKA Kimihiro.
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

#ifndef	_PREP_PLATFORM_H_
#define	_PREP_PLATFORM_H_

#include <sys/param.h>
#include <sys/device.h>

#include <dev/pci/pcivar.h>

struct platform {
	const char	*model;
	int		(*match)(struct platform *);
	void		(*pci_get_chipset_tag)(pci_chipset_tag_t);
	void		(*pci_intr_fixup)(int, int, int *);
	void		(*ext_intr)(void);
	void		(*cpu_setup)(struct device *);
	void		(*reset)(void);
};

struct plattab {
	struct platform **platform;
	int num;
};

extern struct platform *platform;

int ident_platform(void);
int platform_generic_match(struct platform *);
void pci_intr_nofixup(int, int, int *);
void cpu_setup_unknown(struct device *);
void reset_unknown(void);

/* IBM */
extern struct plattab plattab_ibm;
extern struct platform platform_ibm_6050;
extern struct platform platform_ibm_7248;

void cpu_setup_ibm_generic(struct device *);
void reset_ibm_generic(void);

void pci_intr_fixup_ibm_6050(int, int, int *);
void pci_intr_fixup_ibm_7248(int, int, int *);

/* Motorola */
extern struct plattab plattab_mot;

extern struct platform platform_mot_ulmb60xa;

void pci_intr_fixup_mot_ulmb60xa(int, int, int *);

#endif /* !_PREP_PLATFORM_H_ */
