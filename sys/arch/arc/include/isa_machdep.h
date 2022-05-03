/*	$NetBSD: isa_machdep.h,v 1.18 2022/05/03 20:52:31 andvar Exp $	*/
/*      $OpenBSD: isa_machdep.h,v 1.5 1997/04/19 17:20:00 pefo Exp $  */

/*
 * Copyright (c) 1996 Per Fogelstrom
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
 *      This product includes software developed by Per Fogelstrom
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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
#ifndef _ISA_MACHDEP_H_
#define _ISA_MACHDEP_H_

#include <dev/isa/isadmavar.h>

typedef struct arc_isa_bus *isa_chipset_tag_t;

/*
 *      I/O macros to access isa bus ports/memory.
 *      At the first glance these macros may seem inefficient.
 *      However, the CPU executes an instruction every 7.5ns
 *      so the bus is much slower so it doesn't matter, really.
 */
#define isa_outb(x,y)   outb(arc_bus_io.bs_vbase + (x)- arc_bus_io.bs_start, y)
#define isa_inb(x)      inb(arc_bus_io.bs_vbase + (x) - arc_bus_io.bs_start)

struct arc_isa_bus {
        void    *ic_data;

	struct isa_dma_state ic_dmastate;

        void    (*ic_attach_hook)(device_t, device_t,
                    struct isabus_attach_args *);
	const struct evcnt *(*ic_intr_evcnt)(isa_chipset_tag_t, int);
        void    *(*ic_intr_establish)(isa_chipset_tag_t, int, int, int,
                    int (*)(void *), void *);
        void    (*ic_intr_disestablish)(isa_chipset_tag_t, void *);
	void	(*ic_detach_hook)(isa_chipset_tag_t, device_t);
};


/*
 * Functions provided to machine-independent ISA code.
 */
#define isa_attach_hook(p, s, a)                             /*           \
    (*(a)->iba_ic->ic_attach_hook)((p), (s), (a)) */
#define	isa_detach_hook(c, s)						\
    (*(c)->ic_detach_hook)((c), (s))
#define	isa_intr_evcnt(c, i)					\
    (*(c)->ic_intr_evcnt)((c)->ic_data, (i))
#define isa_intr_establish(c, i, t, l, f, a)                         \
    (*(c)->ic_intr_establish)((c)->ic_data, (i), (t), (l), (f), (a))
#define isa_intr_establish_xname(c, i, t, l, f, a, x)			\
    (*(c)->ic_intr_establish)((c)->ic_data, (i), (t), (l), (f), (a))
#define isa_intr_disestablish(c, h)                                     \
    (*(c)->ic_intr_disestablish)((c)->ic_data, (h))

#define	isa_dmainit(ic, bst, dmat, d)					\
	_isa_dmainit(&(ic)->ic_dmastate, (bst), (dmat), (d))
#define	isa_dmadestroy(ic)						\
	_isa_dmadestroy(&(ic)->ic_dmastate)
#define	isa_dmacascade(ic, c)						\
	_isa_dmacascade(&(ic)->ic_dmastate, (c))
#define	isa_dmamaxsize(ic, c)						\
	_isa_dmamaxsize(&(ic)->ic_dmastate, (c))
#define	isa_dmamap_create(ic, c, s, f)					\
	_isa_dmamap_create(&(ic)->ic_dmastate, (c), (s), (f))
#define	isa_dmamap_destroy(ic, c)					\
	_isa_dmamap_destroy(&(ic)->ic_dmastate, (c))
#define	isa_dmastart(ic, c, a, n, p, f, bf)				\
	_isa_dmastart(&(ic)->ic_dmastate, (c), (a), (n), (p), (f), (bf))
#define	isa_dmaabort(ic, c)						\
	_isa_dmaabort(&(ic)->ic_dmastate, (c))
#define	isa_dmacount(ic, c)						\
	_isa_dmacount(&(ic)->ic_dmastate, (c))
#define	isa_dmafinished(ic, c)						\
	_isa_dmafinished(&(ic)->ic_dmastate, (c))
#define	isa_dmadone(ic, c)						\
	_isa_dmadone(&(ic)->ic_dmastate, (c))
#define	isa_dmafreeze(ic)						\
	_isa_dmafreeze(&(ic)->ic_dmastate)
#define	isa_dmathaw(ic)							\
	_isa_dmathaw(&(ic)->ic_dmastate)
#define	isa_dmamem_alloc(ic, c, s, ap, f)				\
	_isa_dmamem_alloc(&(ic)->ic_dmastate, (c), (s), (ap), (f))
#define	isa_dmamem_free(ic, c, a, s)					\
	_isa_dmamem_free(&(ic)->ic_dmastate, (c), (a), (s))
#define	isa_dmamem_map(ic, c, a, s, kp, f)				\
	_isa_dmamem_map(&(ic)->ic_dmastate, (c), (a), (s), (kp), (f))
#define	isa_dmamem_unmap(ic, c, k, s)					\
	_isa_dmamem_unmap(&(ic)->ic_dmastate, (c), (k), (s))
#define	isa_dmamem_mmap(ic, c, a, s, o, p, f)				\
	_isa_dmamem_mmap(&(ic)->ic_dmastate, (c), (a), (s), (o), (p), (f))
#define isa_drq_alloc(ic, c)						\
	_isa_drq_alloc(&(ic)->ic_dmastate, c)
#define isa_drq_free(ic, c)						\
	_isa_drq_free(&(ic)->ic_dmastate, c)
#define	isa_drq_isfree(ic, c)						\
	_isa_drq_isfree(&(ic)->ic_dmastate, (c))
#define	isa_malloc(ic, c, s, p, f)					\
	_isa_malloc(&(ic)->ic_dmastate, (c), (s), (p), (f))
#define	isa_free(a, p)							\
	_isa_free((a), (p))
#define	isa_mappage(m, o, p)						\
	_isa_mappage((m), (o), (p))

int isa_intr_alloc(isa_chipset_tag_t, int, int, int *);

void sysbeepstop(void *);
void sysbeep(int, int);


/*
 *	Interrupt control struct used to control the ICU setup.
 */

struct isa_intrhand {
	struct	isa_intrhand *ih_next;
	int	(*ih_fun)(void *);
	void    *ih_arg;
	u_long  ih_count;
	int     ih_level;
	int     ih_irq;
	struct evcnt ih_evcnt;
	char	ih_evname[32];
};

#endif /* _ISA_MACHDEP_H_ */
