/*
 * Copyright (c) 1992 Berkeley Software Design, Inc.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Berkeley Software
 *	Design, Inc.
 * 4. The name of Berkeley Software Design must not be used to endorse
 *    or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BERKELEY SOFTWARE DESIGN, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL BERKELEY SOFTWARE DESIGN, INC. BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	from: BSDI Id: isavar.h,v 1.5 1992/12/01 18:06:00 karels Exp 
 *	$Id: isavar.h,v 1.9.2.1 1994/07/27 08:46:07 cgd Exp $
 */

/*
 * ISA driver attach arguments
 */
struct isa_attach_args {
	u_short	ia_iobase;		/* base i/o address */
	u_short	ia_iosize;		/* span of ports used */
	u_short	ia_irq;			/* interrupt request */
	u_short	ia_drq;			/* DMA request */
	caddr_t ia_maddr;		/* physical i/o mem addr */
	u_int	ia_msize;		/* size of i/o memory */
	void	*ia_aux;		/* driver specific */
};

#define	IOBASEUNK	0xffff		/* i/o address is unknown */
#define	IRQUNK		0xffff		/* interrupt request line is unknown */
#define	DRQUNK		0xffff		/* DMA request line is unknown */
#define	MADDRUNK	(caddr_t)-1	/* shared memory address is unknown */

/*
 * per-device ISA variables
 */
struct isadev {
	struct  device *id_dev;		/* back pointer to generic */
	struct	isadev *id_bchain;	/* forward link in bus chain */	
};

/*
 * ISA masterbus 
 */
struct isa_softc {
	struct	device sc_dev;		/* base device */
	struct	isadev *sc_isadev;	/* list of all children */
};

#define		cf_iobase		cf_loc[0]
#define		cf_iosize		cf_loc[1]
#define		cf_maddr		cf_loc[2]
#define		cf_msize		cf_loc[3]
#define		cf_irq			cf_loc[4]
#define		cf_drq			cf_loc[5]

/*
 * Interrupt handler chains.  Interrupt handlers should return 0 for
 * `not I', 1 (`I took care of it'), or -1 (`I guess it was mine, but
 * I wasn't expecting it').  intr_establish() inserts a handler into
 * the list.  The handler is called with its (single) argument.
 */
struct intrhand {
	int	(*ih_fun)();
	void	*ih_arg;
	u_long	ih_count;
	struct	intrhand *ih_next;
	int	ih_level;
};

void intr_establish __P((int intr, struct intrhand *));
void intr_disestablish __P((int intr, struct intrhand *));
void isa_establish __P((struct isadev *, struct device *));

/*
 * software conventions
 */
typedef enum { BUS_ISA, BUS_EISA, BUS_MCA } isa_type;

extern int atdevbase;		/* kernel virtual address of "hole" */
extern isa_type isa_bustype;	/* type of bus */

/*
 * Given a kernel virtual address for some location
 * in the "hole" I/O space, return a physical address.
 */
#define	ISA_PHYSADDR(v)	((caddr_t) ((u_long)(v) - atdevbase + IOM_BEGIN))
/*
 * Given a physical address in the "hole",
 * return a kernel virtual address.
 */
#define	ISA_HOLE_VADDR(p)  ((caddr_t) ((u_long)(p) - IOM_BEGIN + atdevbase))
