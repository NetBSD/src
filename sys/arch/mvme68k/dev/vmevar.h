/*	$NetBSD: vmevar.h,v 1.2 1997/10/09 08:42:47 jtc Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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

/*
 * Autoconfiguration and glue definitions for VME support on the
 * Motorola MVME series of computers.
 */

struct vmechip_softc {
	struct	device sc_dev;		/* generic device info */
	caddr_t	sc_reg;			/* chip registers */
	struct	vme_chip *sc_chip;	/* controller vector */
	u_long	sc_irqref[8];		/* ipl reference count */
};

/*
 * Structure used to describe VME controller chips.
 */
struct vme_chip {
	int	(*vme_translate_addr) __P((u_long, size_t, int, int, u_long *));
	void	(*vme_intrline_enable) __P((int));
	void	(*vme_intrline_disable) __P((int));
};

/*
 * Structure used to attach childres to the VME busses and controller.
 */
struct vme_attach_args {
	int	va_bustype;		/* VME_D16 or VME_D32 */
	int	va_atype;		/* VME_A16, VME_A24, or VME_A32 */
	u_long	va_addr;		/* address of card in bus space */
	int	va_ipl;			/* card interrupt level */
	int	va_vec;			/* card interrupt vector */
};

#define VME_D16		0		/* D16 */
#define VME_D32		1		/* D32 */

#define VME_A16		16		/* A16 */
#define VME_A24		24		/* A24 */
#define VME_A32		32		/* A32 */

/* Shorthand for locators. */
#define vmecf_atype	cf_loc[0]
#define vmecf_addr	cf_loc[1]
#define vmecf_ipl	cf_loc[2]
#define vmecf_vec	cf_loc[3]

extern	struct cfdriver vmechip_cd;
extern	struct cfdriver vmes_cd;
extern	struct cfdriver vmel_cd;

void	vme_config __P((struct vmechip_softc *));
void	*vmemap __P((u_long, size_t, int, int));
void	vmeunmap __P((void *, size_t));
void	vmeintr_establish __P((int (*)(void *), void *, int, int));
void	vmeintr_disestablish __P((int, int));
