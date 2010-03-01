/* $NetBSD: sb1250_icu.c,v 1.9.36.6 2010/03/01 23:55:49 matt Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: sb1250_icu.c,v 1.9.36.6 2010/03/01 23:55:49 matt Exp $");

#define	__INTR_PRIVATE

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kmem.h>

/* XXX for uvmexp */
#include <uvm/uvm_extern.h>

#include <machine/systemsw.h>
#include <mips/locore.h>

#include <mips/sibyte/include/sb1250_regs.h>
#include <mips/sibyte/include/sb1250_int.h>
#include <mips/sibyte/include/sb1250_scd.h>

static const struct ipl_sr_map sb1250_ipl_sr_map = {
    .sr_bits = {
	[IPL_NONE]	=	0,
	[IPL_SOFTCLOCK]	=	MIPS_SOFT_INT_MASK_0,
	[IPL_SOFTBIO]	=	MIPS_SOFT_INT_MASK_0,
	[IPL_SOFTNET]	=	MIPS_SOFT_INT_MASK,
	[IPL_SOFTSERIAL] =	MIPS_SOFT_INT_MASK,
	[IPL_VM]	=	MIPS_SOFT_INT_MASK | MIPS_INT_MASK_0,
#if IPL_SCHED == IPL_HIGH
	[IPL_SCHED]	=	MIPS_INT_MASK,
#else
	[IPL_SCHED]	=	MIPS_SOFT_INT_MASK | MIPS_INT_MASK_0
			  	    | MIPS_INT_MASK_1 | MIPS_INT_MASK_5,
#endif
	[IPL_HIGH]	=	MIPS_INT_MASK,
    },
};

/* imr values corresponding to each pin */
static uint64_t ints_for_ipl[_IPL_N];
static uint64_t imr_all;

struct sb1250_ihand {
	void	(*ih_fun)(void *, uint32_t, vaddr_t);
	void	*ih_arg;
	int	ih_ipl;
};

static struct sb1250_ihand sb1250_ihands[K_INT_SOURCES];

#ifdef MULTIPROCESSOR
static void sb1250_ipi_intr(void *, uint32_t, vaddr_t);
struct evcnt *sb1250_evcnts;
#define	INTR_EVCNTS(ci)		(&sb1250_evcnts[(ci)->ci_cpuid])

#define	SB1250_I_IMR_BASE(ci)	MIPS_PHYS_TO_KSEG1(A_IMR_CPU0_BASE \
				    + ((ci)->ci_cpuid * IMR_REGISTER_SPACING))
#else
struct evcnt sb1250_evcnts[K_INT_SOURCES];
#define	INTR_EVCNTS(ci)		((void)ci, sb1250_evcnts)

#define	SB1250_I_IMR_BASE(ci)	((void)ci, MIPS_PHYS_TO_KSEG1(A_IMR_CPU0_BASE))
#endif
#define	SB1250_I_IMR_ADDR(base)	((base) + R_IMR_INTERRUPT_MASK)
#define	SB1250_I_IMR_SSTATUS(base) ((base) + R_IMR_INTERRUPT_SOURCE_STATUS)
#define	SB1250_I_MAP(base, x)	((base) + R_IMR_INTERRUPT_MAP_BASE + (x) * 8)
#define	SB1250_I_IMR_MAILBOX(base)	((base) + R_IMR_MAILBOX_CPU)
#define	SB1250_I_IMR_MAILBOX_SET(base)	((base) + R_IMR_MAILBOX_SET_CPU)
#define	SB1250_I_IMR_MAILBOX_CLR(base)	((base) + R_IMR_MAILBOX_CLR_CPU)

#define	READ_REG(rp)		(mips3_ld((volatile uint64_t *)(rp)))
#define	WRITE_REG(rp, val)	(mips3_sd((volatile uint64_t *)(rp), (val)))

static void	sb1250_cpu_intr(int, vaddr_t, uint32_t);
static void	*sb1250_intr_establish(u_int, u_int,
		    void (*fun)(void *, uint32_t, vaddr_t), void *);

static const char * const intr_names[K_INT_SOURCES] = {
	[K_INT_WATCHDOG_TIMER_0]	= "wdog0",
	[K_INT_WATCHDOG_TIMER_1]	= "wdog1",
	[K_INT_TIMER_0]			= "timer0",
	[K_INT_TIMER_1]			= "timer1",
	[K_INT_TIMER_2]			= "timer2",
	[K_INT_TIMER_3]			= "timer3",
	[K_INT_SMB_0]			= "smb0",
	[K_INT_SMB_1]			= "smb1",
	[K_INT_UART_0]			= "uart0",
	[K_INT_UART_1]			= "uart1",
	[K_INT_SER_0]			= "syncser0",
	[K_INT_SER_1]			= "syncser1",
	[K_INT_PCMCIA]			= "pcmcia",
	[K_INT_ADDR_TRAP]		= "addrtrap",
	[K_INT_PERF_CNT]		= "perfcnt",
	[K_INT_TRACE_FREEZE]		= "tracefreeze",
	[K_INT_BAD_ECC]			= "bad ECC",
	[K_INT_COR_ECC]			= "corrected ECC",
	[K_INT_IO_BUS]			= "iobus",
	[K_INT_MAC_0]			= "mac0",
	[K_INT_MAC_1]			= "mac1",
	[K_INT_MAC_2]			= "mac2",
	[K_INT_DM_CH_0]			= "dmover0",
	[K_INT_DM_CH_1]			= "dmover1",
	[K_INT_DM_CH_2]			= "dmover2",
	[K_INT_DM_CH_3]			= "dmover3",
	[K_INT_MBOX_0]			= "mbox0",
	[K_INT_MBOX_1]			= "mbox1",
	[K_INT_MBOX_2]			= "mbox2",
	[K_INT_MBOX_3]			= "mbox3",
	[K_INT_CYCLE_CP0_INT]		= "zbccp0",
	[K_INT_CYCLE_CP1_INT]		= "zbccp1",
	[K_INT_GPIO_0]			= "gpio0",
	[K_INT_GPIO_1]			= "gpio1",
	[K_INT_GPIO_2]			= "gpio2",
	[K_INT_GPIO_3]			= "gpio3",
	[K_INT_GPIO_4]			= "gpio4",
	[K_INT_GPIO_5]			= "gpio5",
	[K_INT_GPIO_6]			= "gpio6",
	[K_INT_GPIO_7]			= "gpio7",
	[K_INT_GPIO_8]			= "gpio8",
	[K_INT_GPIO_9]			= "gpio9",
	[K_INT_GPIO_10]			= "gpio10",
	[K_INT_GPIO_11]			= "gpio11",
	[K_INT_GPIO_12]			= "gpio12",
	[K_INT_GPIO_13]			= "gpio13",
	[K_INT_GPIO_14]			= "gpio14",
	[K_INT_GPIO_15]			= "gpio15",
	[K_INT_LDT_FATAL]		= "ldt fatal",
	[K_INT_LDT_NONFATAL]		= "ldt nonfatal",
	[K_INT_LDT_SMI]			= "ldt smi",
	[K_INT_LDT_NMI]			= "ldt nmi",
	[K_INT_LDT_INIT]		= "ldt init",
	[K_INT_LDT_STARTUP]		= "ldt startup",
	[K_INT_LDT_EXT]			= "ldt ext",
	[K_INT_PCI_ERROR]		= "pci error",
	[K_INT_PCI_INTA]		= "pci inta",
	[K_INT_PCI_INTB]		= "pci intb",
	[K_INT_PCI_INTC]		= "pci intc",
	[K_INT_PCI_INTD]		= "pci intd",
	[K_INT_SPARE_2]			= "spare2",
	[K_INT_MAC_0_CH1]		= "mac0 ch1",
	[K_INT_MAC_1_CH1]		= "mac1 ch1",
	[K_INT_MAC_2_CH1]		= "mac2 ch1",
};

#ifdef MULTIPROCESSOR
static int
sb1250_send_ipi(struct cpu_info *ci, int tag)
{
	const vaddr_t imr_base = SB1250_I_IMR_BASE(ci);
	const uint64_t mbox_mask = 1LLU << tag;

	WRITE_REG(SB1250_I_IMR_MAILBOX_SET(imr_base), mbox_mask);

	return 0;
}

static void
sb1250_ipi_intr(void *arg, uint32_t status, vaddr_t pc)
{
	struct cpu_info * const ci = curcpu();
	const vaddr_t imr_base = SB1250_I_IMR_BASE(ci);
	uint64_t mbox_mask;

	mbox_mask = READ_REG(SB1250_I_IMR_MAILBOX(imr_base));
	WRITE_REG(SB1250_I_IMR_MAILBOX_CLR(imr_base), mbox_mask);

	ipi_process(ci, mbox_mask);
}
#endif

void
sb1250_cpu_init(struct cpu_info *ci)
{
	const vaddr_t imr_base = SB1250_I_IMR_BASE(ci);
	const char * const xname = ci->ci_dev ? device_xname(ci->ci_dev) : "cpu0";
	struct evcnt * evcnts = INTR_EVCNTS(ci);
	const char * const * names = intr_names;

	WRITE_REG(SB1250_I_IMR_ADDR(imr_base), imr_all);

	for (u_int i = 0; i < K_INT_SOURCES; i++, evcnts++, names++) {
		WRITE_REG(SB1250_I_MAP(imr_base, i), K_INT_MAP_I0);
		evcnt_attach_dynamic(evcnts, EVCNT_TYPE_INTR, NULL,
		    xname, names[i]);
	}
}

void
sb1250_icu_init(void)
{
	ipl_sr_map = sb1250_ipl_sr_map;

	/* zero out the list of used interrupts/lines */
	memset(ints_for_ipl, 0, sizeof ints_for_ipl);
	imr_all = 0xffffffffffffffffULL;
	memset(sb1250_ihands, 0, sizeof sb1250_ihands);

	systemsw.s_cpu_intr = sb1250_cpu_intr;
	systemsw.s_intr_establish = sb1250_intr_establish;

#ifdef MULTIPROCESSOR
	/*
	 * Bits 27:24 (11:8 of G_SYS_PART) encode the number of CPUs present.
	 */
	u_int sys_part = G_SYS_PART(READ_REG(MIPS_PHYS_TO_KSEG1(A_SCD_SYSTEM_REVISION)));
	const u_int cpus = (sys_part >> 8) & 0xf;

	/*
	 * Allocate an evcnt structure for every possible interrupt on
	 * every possible CPU.
	 */
	sb1250_evcnts = kmem_alloc(sizeof(struct evcnt [K_INT_SOURCES*cpus]),
	    KM_SLEEP);
#endif /* MULTIPROCESSOR */

	sb1250_cpu_init(curcpu());

#ifdef MULTIPROCESSOR
	sb1250_intr_establish(K_INT_MBOX_0, IPL_SCHED, sb1250_ipi_intr, NULL);
	sb1250_intr_establish(K_INT_MBOX_1, IPL_SCHED, sb1250_ipi_intr, NULL);
	sb1250_intr_establish(K_INT_MBOX_2, IPL_SCHED, sb1250_ipi_intr, NULL);
	sb1250_intr_establish(K_INT_MBOX_3, IPL_SCHED, sb1250_ipi_intr, NULL);

	mips_locoresw.lsw_send_ipi = sb1250_send_ipi;
#endif /* MULTIPROCESSOR */
}

static void
sb1250_cpu_intr(int ppl, vaddr_t pc, uint32_t status)
{
	struct cpu_info * const ci = curcpu();
	const vaddr_t imr_base = SB1250_I_IMR_BASE(ci);
	struct evcnt * const evcnts = INTR_EVCNTS(ci);
	uint32_t pending;
	int ipl;

	uvmexp.intrs++;

	while (ppl < (ipl = splintr(&pending))) {
		splx(ipl);

		/* XXX do something if 5? */
		if (pending & MIPS_INT_MASK_5) {
			uint32_t cycles = mips3_cp0_count_read();
			mips3_cp0_compare_write(cycles - 1);
			/* just leave the bugger disabled */
		}

		uint64_t sstatus = ints_for_ipl[ipl];
		if (sstatus == 0)
			continue;

		sstatus &= READ_REG(SB1250_I_IMR_SSTATUS(imr_base));
		for (int j = 63; sstatus != 0; j--) {
			u_int n = __builtin_clz(sstatus);
			KASSERT((sstatus >> (63-n)) & 1);
			sstatus <<= n + 1;
			j -= n;
			struct sb1250_ihand *ihp = &sb1250_ihands[j];
			(*ihp->ih_fun)(ihp->ih_arg, status, pc);
			evcnts[j].ev_count++;
		}
		(void) splhigh();
	}
}

static void *
sb1250_intr_establish(u_int num, u_int ipl,
    void (*fun)(void *, uint32_t, vaddr_t), void *arg)
{
	const vaddr_t imr_base = SB1250_I_IMR_BASE(curcpu());
	struct sb1250_ihand * const ih = &sb1250_ihands[num];
	const int s = splhigh();

	if (num >= K_INT_SOURCES)
		panic("%s: invalid interrupt number (0x%x)", __func__, num);
	if (ipl >= _IPL_N || ipl < IPL_VM)
		panic("%s: invalid ipl %d", __func__, ipl);
	if (ih->ih_fun != NULL)
		panic("%s: cannot share sb1250 interrupts", __func__);

	ints_for_ipl[ipl] |= (1ULL << num);
	imr_all &= ~(1ULL << num);

	ih->ih_fun = fun;
	ih->ih_arg = arg;
	ih->ih_ipl = ipl;

	if (ipl > IPL_VM)
		WRITE_REG(SB1250_I_MAP(imr_base, num), K_INT_MAP_I1);

	WRITE_REG(SB1250_I_IMR_ADDR(imr_base), imr_all);

	splx(s);

	return ih;
}
