/* $NetBSD: sb1250_icu.c,v 1.9.36.2 2009/11/23 18:46:50 matt Exp $ */

/*
 * Copyright 2000, 2001
 * Broadcom Corporation. All rights reserved.
 *
 * This software is furnished under license and may be used and copied only
 * in accordance with the following terms and conditions.  Subject to these
 * conditions, you may download, copy, install, use, modify and distribute
 * modified or unmodified copies of this software in source and/or binary
 * form. No title or ownership is transferred hereby.
 *
 * 1) Any source code used, modified or distributed must reproduce and
 *    retain this copyright notice and list of conditions as they appear in
 *    the source file.
 *
 * 2) No right is granted to use any trade name, trademark, or logo of
 *    Broadcom Corporation.  The "Broadcom Corporation" name may not be
 *    used to endorse or promote products derived from this software
 *    without the prior written permission of Broadcom Corporation.
 *
 * 3) THIS SOFTWARE IS PROVIDED "AS-IS" AND ANY EXPRESS OR IMPLIED
 *    WARRANTIES, INCLUDING BUT NOT LIMITED TO, ANY IMPLIED WARRANTIES OF
 *    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, OR
 *    NON-INFRINGEMENT ARE DISCLAIMED. IN NO EVENT SHALL BROADCOM BE LIABLE
 *    FOR ANY DAMAGES WHATSOEVER, AND IN PARTICULAR, BROADCOM SHALL NOT BE
 *    LIABLE FOR DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *    BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *    WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *    OR OTHERWISE), EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sb1250_icu.c,v 1.9.36.2 2009/11/23 18:46:50 matt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>

/* XXX for uvmexp */
#include <uvm/uvm_extern.h>

#include <machine/systemsw.h>
#include <mips/locore.h>

/* XXX for now, this copes with one cpu only, and assumes it's CPU 0 */

/* imr values corresponding to each pin */
uint64_t ints_for_line[6];
uint64_t imr_all;

struct sb1250_ihand {
	void	(*fun)(void *, uint32_t, vaddr_t);
	void	*arg;
	int	level;
	struct evcnt count;
};
static struct sb1250_ihand sb1250_ihands[64];		/* XXX */

#define	SB1250_I_IMR_ADDR	(MIPS_PHYS_TO_KSEG1(0x10020000 + 0x0028))
#define	SB1250_I_IMR_SSTATUS	(MIPS_PHYS_TO_KSEG1(0x10020000 + 0x0040))
#define	SB1250_I_MAP(x)							\
    (MIPS_PHYS_TO_KSEG1(0x10020000 + 0x0200 + (x) * 8))
#define	SB1250_I_MAP_I0		0x00
#define	SB1250_I_MAP_I1		0x01
#define	SB1250_I_MAP_I2		0x02
/* XXX */

#define	READ_REG(rp)		(mips3_ld((volatile uint64_t *)(rp)))
#define	WRITE_REG(rp, val)	(mips3_sd((volatile uint64_t *)(rp), (val)))

static void	sb1250_cpu_intr(uint32_t, uint32_t, vaddr_t, uint32_t);
static void	*sb1250_intr_establish(u_int, u_int,
		    void (*fun)(void *, uint32_t, vaddr_t), void *);

void
sb1250_icu_init(void)
{
	int i;
	char *name;

	/* zero out the list of used interrupts/lines */
	memset(ints_for_line, 0, sizeof ints_for_line);
	imr_all = 0xffffffffffffffffULL;
	memset(sb1250_ihands, 0, sizeof sb1250_ihands);

	systemsw.s_cpu_intr = sb1250_cpu_intr;
	systemsw.s_intr_establish = sb1250_intr_establish;

	WRITE_REG(SB1250_I_IMR_ADDR, imr_all);

	for (i = 0; i < 64; i++) {
		WRITE_REG(SB1250_I_MAP(i), SB1250_I_MAP_I0);
		/* XXX add irq name arrays for various CPU models? */
		name = malloc(8, M_DEVBUF, M_NOWAIT);
		snprintf(name, 8, "irq %d", i);
		evcnt_attach_dynamic(&sb1250_ihands[i].count, EVCNT_TYPE_INTR,
		    NULL, "sb1250", name);	/* XXX "sb1250"? */
	}
}

static void
sb1250_cpu_intr(uint32_t status, uint32_t cause, vaddr_t pc, uint32_t ipending)
{
	int i, j;
	uint64_t sstatus;
	uint32_t cycles;
	struct cpu_info *ci;

	ci = curcpu();
	ci->ci_idepth++;
	uvmexp.intrs++;

	/* XXX do something if 5? */
	if (ipending & (MIPS_INT_MASK_0 << 5)) {
		cycles = mips3_cp0_count_read();
		mips3_cp0_compare_write(cycles - 1);
		/* just leave the bugger disabled */
	}

	for (i = 4; i >= 0; i--) {
		if (ipending & (MIPS_INT_MASK_0 << i)) {

			sstatus = READ_REG(SB1250_I_IMR_SSTATUS);
			sstatus &= ints_for_line[i];
			for (j = 0; sstatus != 0 && j < 64; j++) {
				if (sstatus & ((uint64_t)1 << j)) {
					struct sb1250_ihand *ihp =
					    &sb1250_ihands[j];
					(*ihp->fun)(ihp->arg, status, pc);
					sstatus &= ~((uint64_t)1 << j);
					ihp->count.ev_count++;
				}
			}
		}
		cause &= ~(MIPS_INT_MASK_0 << i);
	}
	ci->ci_idepth--;

	/* Re-enable anything that we have processed. */
	_splset(MIPS_SR_INT_IE | ((status & ~cause) & MIPS_HARD_INT_MASK));

#ifdef __HAVE_FAST_SOFTINTS
	ipending &= (MIPS_SOFT_INT_MASK_1|MIPS_SOFT_INT_MASK_0);
	if (ipending == 0)
		return;
	_clrsoftintr(ipending);
	softintr_dispatch(ipending);
#endif
}

static void *
sb1250_intr_establish(u_int num, u_int ipl,
    void (*fun)(void *, uint32_t, vaddr_t), void *arg)
{
	int s, line;

	s = splhigh();

	if (num >= 64)					/* XXX */
	    panic("invalid interrupt number (0x%x)", num);
	if (ipl >= _NIPL)
	    panic("invalid ipl (0x%x)", ipl);

	if (sb1250_ihands[num].fun != NULL)
	    panic("cannot share sb1250 interrupts");

	/* XXX for now, everything on I0 */
	switch (ipl) {
#if 0
	case IPL_NMI:
		sr_mask = XXX;
		break;
	case IPL_STATCLOCK:
		sr_mask = XXX;
		break;
	case IPL_CLOCK:
		sr_mask = XXX;
		break;
#endif
	default:
		line = 0;
		break;
	}

	ints_for_line[line] |= (1ULL << num);
	imr_all &= ~(1ULL << num);

	/* XXX map ! */

	sb1250_ihands[num].fun = fun;
	sb1250_ihands[num].arg = arg;
	sb1250_ihands[num].level = ipl;

	WRITE_REG(SB1250_I_IMR_ADDR, imr_all);

	splx(s);

	return (&sb1250_ihands[num]);
}

static const int ipl2spl_table[] = {
	[IPL_NONE] = 0,
	[IPL_SOFTCLOCK] = MIPS_SOFT_INT_MASK_0,
	[IPL_SOFTNET] = MIPS_SOFT_INT_MASK_1,
	[IPL_VM] = _IMR_VM,
	[IPL_SCHED] = _IMR_SCHED,
	[IPL_HIGH] = _IMR_HIGH,
};

ipl_cookie_t
makeiplcookie(ipl_t ipl)
{

	return (ipl_cookie_t){._spl = ipl2spl_table[ipl]};
}
