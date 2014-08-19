/*	$NetBSD: omap_intr.h,v 1.8.2.1 2014/08/20 00:02:47 tls Exp $ */

/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain this list of conditions
 *    and the following disclaimer.
 * 2. Redistributions in binary form must reproduce this list of conditions
 *    and the following disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL ANY
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * This file used pxa2x0_intr.h as a template, but now bears little semblance
 * to it.  In its new form, it should not be included directly.  Rather, it
 * should only be included from an include file that is specific to a
 * particular OMAP (e.g. omap5912_intr.h).  That file will define the
 * information that varies from OMAP to OMAP and then includes this file to
 * provide the OMAP generic information.  As more OMAP ports are done, more
 * information may need to move from this file out into the particular OMAP
 * header files.
 */

#ifndef _ARM_OMAP_OMAP_INTR_H_
#define _ARM_OMAP_OMAP_INTR_H_

#define	ARM_IRQ_HANDLER	_C_LABEL(omap_irq_handler)

#ifndef _LOCORE

#include <sys/device_if.h>

#include <arm/cpu.h>
#include <arm/armreg.h>
#include <arm/cpufunc.h>

#define OMAP_IRQ_MIN			0
#define OMAP_NIRQ			(OMAP_INT_L1_NIRQ + OMAP_INT_L2_NIRQ)
#define OMAP_BANK_WIDTH			(32)
#if (OMAP_NIRQ & (OMAP_BANK_WIDTH - 1))
#error OMAP_NIRQ must be an even multiple of OMAP_BANK_WIDTH
#endif
#define OMAP_NBANKS			(OMAP_NIRQ / OMAP_BANK_WIDTH)

/* Offsets within the OMAP interrupt controllers */
#define OMAP_INTC_SIR_IRQ	0x10	/* Interrupt source register (IRQ) */
#define OMAP_INTC_SIR_FIQ	0x14	/* Interrupt source register (FIQ) */
/* These registers differ in meaning between L1 and L2 controllers */
#define OMAP_INTL1_CONTROL	0x18	/* Interrupt control register */
#define  OMAP_INTL1_CONTROL_NEW_FIQ_AGR		(1<<1)
#define  OMAP_INTL1_CONTROL_NEW_IRQ_AGR		(1<<0)
#define OMAP_INTL1_GMR		0xA0	/* Global mask interrupt register */
#define  OMAP_INTL1_GMR_GLOBAL_MASK			(1<<0)
#define OMAP_INTL2_CONTROL	0x18	/* Interrupt control register */
#define  OMAP_INTL2_CONTROL_GLOBAL_MASK		(1<<2)
#define  OMAP_INTL2_CONTROL_NEW_FIQ_AGR		(1<<1)
#define  OMAP_INTL2_CONTROL_NEW_IRQ_AGR		(1<<0)
#define OMAP_INTL2_STATUS	0xA0	/* Status register */
#define  OMAP_INTL2_STATUS_RESET_DONE		(1<<0)
#define OMAP_INTL2_OCP_CFG	0xA4	/* OCP configuration register */
#define  OMAP_INTL2_OCP_CFG_FORCE_WAKEUP	(0<<3)
#define  OMAP_INTL2_OCP_CFG_SMART_IDLE		(2<<3)
#define  OMAP_INTL2_OCP_CFG_SOFTRESET		(1<<1)
#define  OMAP_INTL2_OCP_CFG_AUTOIDLE		(1<<0)
#define OMAP_INTL2_INTH_REV	0xA8	/* Interrupt controller revision ID */
#define  OMAP_INTL2_INTH_REV_MAJOR(r)		(((r)&0xF0)>>4)
#define  OMAP_INTL2_INTH_REV_MINOR(r)		((r)&0x0F)
#define OMAP_INTL2_BANK_OFF	0x100	/* Offset between each bank. */


/* Offsets within the banks of the OMAP interrupt controllers */
#define OMAP_INTB_ITR		0x00	/* Interrupt register */
#define OMAP_INTB_MIR		0x04	/* Interrupt mask register */
#define OMAP_INTB_ILR_BASE	0x1C	/* Interrupt priority level register */
#define  OMAP_INTB_ILR_PRIO_SHIFT		2
#define  OMAP_INTB_ILR_PRIO_LOWEST		31	/* L2's really 127 */
#define  OMAP_INTB_ILR_PRIO_HIGHEST		0
#define  OMAP_INTB_ILR_EDGE			(0<<1)
#define  OMAP_INTB_ILR_LEVEL			(1<<1)
#define  OMAP_INTB_ILR_IRQ			(0<<0)
#define  OMAP_INTB_ILR_FIQ			(1<<0)
#define OMAP_INTB_SISR		0x9C	/* Software interrupt set register */

/* Array of structures that allow us to get per interrupt constants. */
typedef enum {
    /* 0 in this field marks a reserved interrupt that should not be used. */
    INVALID = 0,
    TRIG_LEVEL,
    TRIG_EDGE,
    TRIG_LEVEL_OR_EDGE
} omap_intr_trig_t;
typedef struct {
	omap_intr_trig_t trig;
	vaddr_t bank_base;
	int bank_num;
	vaddr_t ILR;
	uint32_t mask;
} omap_intr_info_t;
extern const omap_intr_info_t omap_intr_info[OMAP_NIRQ];

/* Array of pointers to each bank's base. */
extern vaddr_t omap_intr_bank_bases[OMAP_NBANKS];

/* Array of arrays to give us the per bank masks for each level. */
extern uint32_t omap_spl_masks[NIPL][OMAP_NBANKS];
/* Array of globally-off masks while interrupts processed. */
extern uint32_t omap_global_masks[OMAP_NBANKS];

/*
 * Direct access is being done because 1) it's faster and 2) the interrupt
 * controller code is still very tied to the OMAP so we don't really have
 * the concept of going through a bus at an address specified in the config.
 */
#define read_icu(base,offset) (*(volatile uint32_t *)((base)+(offset)))
#define write_icu(base,offset,value) \
 (*(volatile uint32_t *)((base)+(offset))=(value))

static inline void
omap_splx(int new)
{
	vaddr_t *bases = &omap_intr_bank_bases[0];
	uint32_t *masks = &omap_spl_masks[new][0];
	int psw = disable_interrupts(I32_bit);

	set_curcpl(new);

#if OMAP_NBANKS != 5
#error Revisit loop unrolling in omap_splx()
#endif
	write_icu(bases[0], OMAP_INTB_MIR, masks[0] | omap_global_masks[0]);
	write_icu(bases[1], OMAP_INTB_MIR, masks[1] | omap_global_masks[1]);
	write_icu(bases[2], OMAP_INTB_MIR, masks[2] | omap_global_masks[2]);
	write_icu(bases[3], OMAP_INTB_MIR, masks[3] | omap_global_masks[3]);
	write_icu(bases[4], OMAP_INTB_MIR, masks[4] | omap_global_masks[4]);
	cpu_dosoftints();
	restore_interrupts(psw);
}

static inline int
omap_splraise(int ipl)
{
	const int old = curcpl();
	if (ipl > old)
		omap_splx(ipl);
	return (old);
}

static inline int
omap_spllower(int ipl)
{
	const int old = curcpl();
	omap_splx(ipl);
	return(old);
}

int	_splraise(int);
int	_spllower(int);
void	splx(int);

#if !defined(EVBARM_SPL_NOINLINE)
#define splx(new)		omap_splx(new)
#define	_spllower(ipl)		omap_spllower(ipl)
#define	_splraise(ipl)		omap_splraise(ipl)
#ifdef __HAVE_FAST_SOFTINTS
#define	_setsoftintr(si)	omap_setsoftintr(si)
#endif
#endif	/* !EVBARM_SPL_NOINTR */

void omap_irq_handler(void *);
void *omap_intr_establish(int, int, const char *, int (*)(void *), void *);
void omap_intr_disestablish(void *);
/* XXX MARTY -- This is a hack to work around the circular dependency
 * between sys/device.h and omap_intr.h  It should be removed when
 * the circular dependency is fixed and the declaration repaired.
 */
struct cfdata;
int omapintc_match(device_t, struct cfdata *, void *);
void omapintc_attach(device_t, device_t, void *);

#endif /* ! _LOCORE */

#endif /* _ARM_OMAP_OMAP_INTR_H_ */
