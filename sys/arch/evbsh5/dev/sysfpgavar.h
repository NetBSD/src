/*	$NetBSD: sysfpgavar.h,v 1.7 2002/10/22 15:19:09 scw Exp $	*/

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Steve C. Woodford for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _SH5_SYSFPGAVAR_H
#define _SH5_SYSFPGAVAR_H

struct sysfpga_attach_args {
	const char	*sa_name;
	bus_space_tag_t	sa_bust;
	bus_dma_tag_t	sa_dmat;
	bus_addr_t	sa_offset;

	bus_addr_t	_sa_base;
};

/*
 * Interrupt groups managed by the System FPGA
 */
#define	SYSFPGA_IGROUP_IRL1	0	/* Output to CPU's IRL1 pin */
#define	SYSFPGA_IGROUP_IRL0	1	/* Output to CPU's IRL0 pin */
#define	SYSFPGA_IGROUP_IRL2	2	/* Output to CPU's IRL2 pin */
#define	SYSFPGA_IGROUP_IRL3	3	/* Output to CPU's IRL3 pin */
#define	SYSFPGA_NGROUPS		4

/*
 * Super IO generates the following interrupts
 */
#define	SYSFPGA_IRL1_INUM_DCD	0	/* XXX: Not strictly SuperIO! */
#define	SYSFPGA_IRL1_INUM_LAN	1	/* XXX: Not strictly SuperIO! */
#define	SYSFPGA_IRL1_INUM_KBD	2
#define	SYSFPGA_IRL1_INUM_UART2	3
#define	SYSFPGA_IRL1_INUM_UART1	4
#define	SYSFPGA_IRL1_INUM_LPT	5
#define	SYSFPGA_IRL1_INUM_MOUSE	6
#define	SYSFPGA_IRL1_INUM_IDE	7
#define	SYSFPGA_IRL1_NINTR	8

/*
 * PCI1 generates the following interrupts
 */
#define	SYSFPGA_IRL2_INTA	0
#define	SYSFPGA_IRL2_INTB	1
#define	SYSFPGA_IRL2_INTC	2
#define	SYSFPGA_IRL2_INTD	3
#define	SYSFPGA_IRL2_NINTR	4

/*
 * PCI2 generates the following interrupts
 */
#define	SYSFPGA_IRL3_INTA	0
#define	SYSFPGA_IRL3_INTB	1
#define	SYSFPGA_IRL3_INTC	2
#define	SYSFPGA_IRL3_INTD	3
#define	SYSFPGA_IRL3_FAL	4	/* XXX: cPCI form-factor only */
#define	SYSFPGA_IRL3_DEG	5	/* XXX: cPCI form-factor only */
#define	SYSFPGA_IRL3_INTP	6	/* XXX: cPCI form-factor only */
#define	SYSFPGA_IRL3_INTS	7	/* XXX: cPCI form-factor only */
#define	SYSFPGA_IRL3_NINTR	8

struct evcnt;
extern void *sysfpga_intr_establish(int, int, int, int (*)(void *), void *);
extern void sysfpga_intr_disestablish(void *);
extern struct evcnt *sysfpga_intr_evcnt(int, int);
extern void sysfpga_nmi_clear(void);
extern void sysfpga_sreset(void);

#endif /* _SH5_SYSFPGAVAR_H */
