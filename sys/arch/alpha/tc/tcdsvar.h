/* $NetBSD: tcdsvar.h,v 1.9 1998/05/24 23:41:43 thorpej Exp $ */

/*
 * Copyright (c) 1995, 1996 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 * 
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS" 
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND 
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

struct tcds_slotconfig {
	/*
	 * Bookkeeping information
	 */
	int	sc_slot;

	bus_space_tag_t sc_bst;			/* to frob TCDS regs */
	bus_space_handle_t sc_bsh;

	struct asc_softc *sc_asc;		/* to frob child's regs */

	int	(*sc_intrhand) __P((void *));	/* intr. handler */
	void	*sc_intrarg;			/* intr. handler arg. */

	/*
	 * Sets of bits in TCDS CIR and IMER that enable/check
	 * various things.
	 */
	u_int32_t sc_resetbits;
	u_int32_t sc_intrmaskbits;
	u_int32_t sc_intrbits;
	u_int32_t sc_dmabits;
	u_int32_t sc_errorbits;

	/*
	 * Offsets to slot-specific DMA resources.
	 */
	bus_size_t sc_sda;
	bus_size_t sc_dic;
	bus_size_t sc_dud0;
	bus_size_t sc_dud1;

	/*
	 * DMA bookkeeping information.
	 */
	int	sc_active;                      /* DMA active ? */
	int	sc_iswrite;			/* DMA into main memory? */
	size_t	sc_dmasize;
	caddr_t	*sc_dmaaddr;
	size_t	*sc_dmalen;
};

struct tcdsdev_attach_args {
	bus_space_tag_t tcdsda_bst;		/* bus space tag */
	bus_space_handle_t tcdsda_bsh;		/* bus space handle */
	struct tcds_slotconfig *tcdsda_sc;	/* slot configuration */
	int	tcdsda_chip;			/* chip number */
	int	tcdsda_id;			/* SCSI ID */
	u_int	tcdsda_freq;			/* chip frequency */
	int	tcdsda_variant;			/* NCR chip variant */
};

/*
 * TCDS functions.
 */
void	tcds_intr_establish __P((struct device *, int,
	    int (*)(void *), void *));
void	tcds_intr_disestablish __P((struct device *, int));
void	tcds_dma_enable __P((struct tcds_slotconfig *, int));
void	tcds_scsi_enable __P((struct tcds_slotconfig *, int));
int	tcds_scsi_iserr __P((struct tcds_slotconfig *));
int	tcds_scsi_isintr __P((struct tcds_slotconfig *, int));
void	tcds_scsi_reset __P((struct tcds_slotconfig *));

/*
 * TCDS DMA functions (used the the 53c94 driver)
 */
int	tcds_dma_isintr	__P((struct tcds_slotconfig *));
void	tcds_dma_reset __P((struct tcds_slotconfig *));
int	tcds_dma_intr __P((struct tcds_slotconfig *));
int	tcds_dma_setup __P((struct tcds_slotconfig *, caddr_t *, size_t *,
	    int, size_t *));
void	tcds_dma_go __P((struct tcds_slotconfig *));
int	tcds_dma_isactive __P((struct tcds_slotconfig *));
