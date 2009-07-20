/*	$NetBSD: sapic.c,v 1.1 2009/07/20 04:41:36 kiyohara Exp $	*/
/*-
 * Copyright (c) 2001 Doug Rabson
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD: src/sys/ia64/ia64/sapic.c,v 1.15 2008/04/14 20:34:45 marcel Exp $
 */

#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/bus.h>
#include <sys/lock.h>
#include <sys/mutex.h>
#include <sys/sysctl.h>

#include <machine/intr.h>
#include <machine/pal.h>
#include <machine/sapicreg.h>
#include <machine/sapicvar.h>


static MALLOC_DEFINE(M_SAPIC, "sapic", "I/O SAPIC devices");

struct sapic *ia64_sapics[16]; /* XXX make this resizable */
static int ia64_sapic_count;

uint64_t ia64_lapic_address = PAL_PIB_DEFAULT_ADDR;

struct sapic_rte {
	uint64_t	rte_vector		:8;
	uint64_t	rte_delivery_mode	:3;
	uint64_t	rte_destination_mode	:1;
	uint64_t	rte_delivery_status	:1;
	uint64_t	rte_polarity		:1;
	uint64_t	rte_rirr		:1;
	uint64_t	rte_trigger_mode	:1;
	uint64_t	rte_mask		:1;
	uint64_t	rte_flushen		:1;
	uint64_t	rte_reserved		:30;
	uint64_t	rte_destination_eid	:8;
	uint64_t	rte_destination_id	:8;
};

struct sapic *
sapic_lookup(uint irq)
{
	struct sapic *sa;
	int i;

	for (i = 0; i < ia64_sapic_count; i++) {
		sa = ia64_sapics[i];
		if (irq >= sa->sa_base && irq <= sa->sa_limit)
			return sa;
	}

	return NULL;
}

static __inline uint32_t
sapic_read(struct sapic *sa, int which)
{
	vaddr_t reg = sa->sa_registers;

	*(volatile uint32_t *) (reg + SAPIC_IO_SELECT) = which;
	ia64_mf();
	return *((volatile uint32_t *)(reg + SAPIC_IO_WINDOW));
}

static __inline void
sapic_write(struct sapic *sa, int which, uint32_t value)
{
	vaddr_t reg = sa->sa_registers;

	*(volatile uint32_t *) (reg + SAPIC_IO_SELECT) = which;
	ia64_mf();
	*(volatile uint32_t *) (reg + SAPIC_IO_WINDOW) = value;
	ia64_mf();
}

static __inline void
sapic_read_rte(struct sapic *sa, int which, struct sapic_rte *rte)
{
	uint32_t *p = (uint32_t *) rte;

	p[0] = sapic_read(sa, SAPIC_RTE_BASE + 2 * which);
	p[1] = sapic_read(sa, SAPIC_RTE_BASE + 2 * which + 1);
}

static __inline void
sapic_write_rte(struct sapic *sa, int which, struct sapic_rte *rte)
{
	uint32_t *p = (uint32_t *) rte;

	sapic_write(sa, SAPIC_RTE_BASE + 2 * which, p[0]);
	sapic_write(sa, SAPIC_RTE_BASE + 2 * which + 1, p[1]);
}

int
sapic_config_intr(u_int irq, int trigger)
{
	struct sapic_rte rte;
	struct sapic *sa;

	sa = sapic_lookup(irq);
	if (sa == NULL)
		return EINVAL;

	mutex_enter(&sa->sa_mtx);
	sapic_read_rte(sa, irq - sa->sa_base, &rte);
	rte.rte_trigger_mode = (trigger == IST_EDGE) ?
	    SAPIC_TRIGGER_EDGE : SAPIC_TRIGGER_LEVEL;
	sapic_write_rte(sa, irq - sa->sa_base, &rte);
	mutex_exit(&sa->sa_mtx);
	return 0;
}

struct sapic *
sapic_create(u_int id, u_int base, uint64_t address)
{
	struct sapic_rte rte;
	struct sapic *sa;
	u_int i;

	sa = malloc(sizeof(struct sapic), M_SAPIC, M_ZERO | M_NOWAIT);
	if (sa == NULL)
		return NULL;

	sa->sa_id = id;
	sa->sa_base = base;
	sa->sa_registers = IA64_PHYS_TO_RR6(address);

	mutex_init(&sa->sa_mtx, MUTEX_SPIN, IPL_HIGH);

	sa->sa_limit = base + ((sapic_read(sa, SAPIC_VERSION) >> 16) & 0xff);

	ia64_sapics[ia64_sapic_count++] = sa;

	/*
	 * Initialize all RTEs with a default trigger mode and polarity.
	 * This may be changed later by calling sapic_config_intr(). We
	 * mask all interrupts by default.
	 */
	memset(&rte, 0, sizeof(rte));
	rte.rte_mask = 1;
	for (i = base; i <= sa->sa_limit; i++) {
		rte.rte_trigger_mode = (i < 16) ? SAPIC_TRIGGER_EDGE :
		    SAPIC_TRIGGER_LEVEL;
		rte.rte_polarity = (i < 16) ? SAPIC_POLARITY_HIGH :
		    SAPIC_POLARITY_LOW;
		sapic_write_rte(sa, i - base, &rte);
	}

	return sa;
}

int
sapic_enable(struct sapic *sa, u_int irq, u_int vector)
{
	struct sapic_rte rte;
	uint64_t lid = ia64_get_lid();

	mutex_enter(&sa->sa_mtx);
	sapic_read_rte(sa, irq - sa->sa_base, &rte);
	rte.rte_destination_id = (lid >> 24) & 255;
	rte.rte_destination_eid = (lid >> 16) & 255;
	rte.rte_delivery_mode = SAPIC_DELMODE_FIXED;
	rte.rte_vector = vector;
	rte.rte_mask = 0;
	sapic_write_rte(sa, irq - sa->sa_base, &rte);
	mutex_exit(&sa->sa_mtx);
	return 0;
}

void
sapic_eoi(struct sapic *sa, u_int vector)
{
	vaddr_t reg = sa->sa_registers;

	*(volatile uint32_t *)(reg + SAPIC_APIC_EOI) = vector;
	ia64_mf();
}

/* Expected to be called with interrupts disabled. */
void
sapic_mask(struct sapic *sa, u_int irq)
{
	struct sapic_rte rte;

	mutex_enter(&sa->sa_mtx);
	sapic_read_rte(sa, irq - sa->sa_base, &rte);
	rte.rte_mask = 1;
	sapic_write_rte(sa, irq - sa->sa_base, &rte);
	mutex_exit(&sa->sa_mtx);
}

/* Expected to be called with interrupts disabled. */
void
sapic_unmask(struct sapic *sa, u_int irq)
{
	struct sapic_rte rte;

	mutex_enter(&sa->sa_mtx);
	sapic_read_rte(sa, irq - sa->sa_base, &rte);
	rte.rte_mask = 0;
	sapic_write_rte(sa, irq - sa->sa_base, &rte);
	mutex_exit(&sa->sa_mtx);
}


#ifdef DDB

#include <ddb/ddb.h>

void
sapic_print(struct sapic *sa, u_int irq)
{
	struct sapic_rte rte;

	db_printf("sapic=%u, irq=%u: ", sa->sa_id, irq);
	sapic_read_rte(sa, irq - sa->sa_base, &rte);
	db_printf("%3d %x->%x:%x %d %s %s %s %s %s %s\n", rte.rte_vector,
	    rte.rte_delivery_mode,
	    rte.rte_destination_id, rte.rte_destination_eid,
	    rte.rte_destination_mode,
	    rte.rte_delivery_status ? "DS" : "  ",
	    rte.rte_polarity ? "low-active " : "high-active",
	    rte.rte_rirr ? "RIRR" : "    ",
	    rte.rte_trigger_mode ? "level" : "edge ",
	    rte.rte_flushen ? "F" : " ",
	    rte.rte_mask ? "(masked)" : "");
}

#endif
