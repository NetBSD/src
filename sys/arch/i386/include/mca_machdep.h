/*	$NetBSD: mca_machdep.h,v 1.1.2.1 2000/06/22 17:00:33 minoura Exp $	*/

/*
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * Copyright (c) 1999 Scott D. Telford.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _I386_MCA_MACHDEP_H_
#define _I386_MCA_MACHDEP_H_

/*
 * i386-specific definitions for MCA autoconfiguration.
 */

extern struct i386_bus_dma_tag mca_bus_dma_tag;

/* set to 1 if MCA bus is detected */
extern int MCA_system;

int	mca_nmi(void);

/*
 * Types provided to machine-independent MCA code.
 */
struct i386_mca_chipset {
        void * /*struct mca_dma_state*/ ic_dmastate;
};

typedef struct i386_mca_chipse *mca_chipset_tag_t;
typedef int mca_intr_handle_t;

/*
 * Functions provided to machine-independent MCA code.
 */
struct mcabus_attach_args;

void	mca_attach_hook(struct device *, struct device *,
		struct mcabus_attach_args *);
const struct evcnt *mca_intr_evcnt(mca_chipset_tag_t, mca_intr_handle_t);
void	*mca_intr_establish(mca_chipset_tag_t, mca_intr_handle_t,
		int, int (*)(void *), void *);
void	mca_intr_disestablish(mca_chipset_tag_t, void *);
int	mca_conf_read(mca_chipset_tag_t, int, int);
void	mca_conf_write(mca_chipset_tag_t, int, int, int);
void	mca_busprobe(void);

/* MCA register addresses for IBM PS/2 */

#define PS2_SYS_CTL_A		0x92	/* PS/2 System Control Port A */
#define MCA_MB_SETUP_REG	0x94	/* Motherboard setup register */
#define MCA_ADAP_SETUP_REG	0x96	/* Adapter setup register */
#define MCA_POS_REG_BASE	0x100	/* POS registers base address */
#define MCA_POS_REG_SIZE	8	/* POS registers window size */

#define MCA_POS_REG(n)		(0x100+(n))	/* POS registers 0-7 */

/* Adapter setup register bits */

#define MCA_ADAP_SET		0x08	/* Adapter setup mode */
#define MCA_ADAP_CHR		0x80	/* Adapter channel reset */

#endif /* _I386_MCA_MACHDEP_H_ */
