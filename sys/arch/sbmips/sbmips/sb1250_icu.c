/* $NetBSD: sb1250_icu.c,v 1.13.12.1 2017/12/03 11:36:40 jdolecek Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: sb1250_icu.c,v 1.13.12.1 2017/12/03 11:36:40 jdolecek Exp $");

#define	__INTR_PRIVATE

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/evcnt.h>
#include <sys/kmem.h>

/* XXX for uvmexp */
#include <uvm/uvm_extern.h>

#include <mips/locore.h>

#include <sbmips/cpuvar.h>
#include <sbmips/systemsw.h>

#include <mips/sibyte/include/sb1250_regs.h>
#include <mips/sibyte/include/sb1250_int.h>
#include <mips/sibyte/include/sb1250_scd.h>

static const struct ipl_sr_map sb1250_ipl_sr_map = {
    .sr_bits = {
	[IPL_NONE]	=	MIPS_INT_MASK_5,
	[IPL_SOFTCLOCK]	=	MIPS_SOFT_INT_MASK_0 | MIPS_INT_MASK_5,
	[IPL_SOFTBIO]	=	MIPS_SOFT_INT_MASK_0 | MIPS_INT_MASK_5,
	[IPL_SOFTNET]	=	MIPS_SOFT_INT_MASK | MIPS_INT_MASK_5,
	[IPL_SOFTSERIAL] =	MIPS_SOFT_INT_MASK | MIPS_INT_MASK_5,
	[IPL_VM]	=	MIPS_SOFT_INT_MASK | MIPS_INT_MASK_0
				    | MIPS_INT_MASK_5,
	[IPL_SCHED]	=	MIPS_SOFT_INT_MASK | MIPS_INT_MASK_0
			  	    | MIPS_INT_MASK_1 | MIPS_INT_MASK_5,
	[IPL_DDB]	=	MIPS_SOFT_INT_MASK | MIPS_INT_MASK_0
			  	    | MIPS_INT_MASK_1 | MIPS_INT_MASK_4
			  	    | MIPS_INT_MASK_5,
	[IPL_HIGH]	=	MIPS_INT_MASK,
    },
};

/* imr values corresponding to each pin */
static uint64_t ints_for_ipl[_IPL_N];

struct sb1250_ihand {
	void	(*ih_fun)(void *, uint32_t, vaddr_t);
	void	*ih_arg;
	int	ih_ipl;
};

static struct sb1250_ihand sb1250_ihands[K_INT_SOURCES];

#ifdef MULTIPROCESSOR
static void sb1250_ipi_intr(void *, uint32_t, vaddr_t);
#endif
#define	SB1250_I_MAP(x)		(R_IMR_INTERRUPT_MAP_BASE + (x) * 8)

#define	READ_REG(rp)		mips3_ld((register_t)(rp))
#define	WRITE_REG(rp, val)	mips3_sd((register_t)(rp), (val))

static void	sb1250_cpu_intr(int, vaddr_t, uint32_t);
static void	*sb1250_intr_establish(u_int, u_int,
		    void (*fun)(void *, uint32_t, vaddr_t), void *);

static const char sb1250_intr_names[K_INT_SOURCES][16] = {
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
static void
sb1250_lsw_cpu_init(struct cpu_info *ci)
{
	struct cpu_softc * const cpu = ci->ci_softc;

	WRITE_REG(cpu->sb1cpu_imr_base + R_IMR_INTERRUPT_MASK, cpu->sb1cpu_imr_all);
}

static int
sb1250_lsw_send_ipi(struct cpu_info *ci, int tag)
{
	struct cpu_softc * const cpu = ci->ci_softc;
	const uint64_t mbox_mask = 1LLU << tag;

	if (cpus_running & (1 << cpu_index(ci)))
		WRITE_REG(cpu->sb1cpu_imr_base + R_IMR_MAILBOX_SET_CPU, mbox_mask);

	return 0;
}

static void
sb1250_ipi_intr(void *arg, uint32_t status, vaddr_t pc)
{
	struct cpu_info * const ci = curcpu();
	struct cpu_softc * const cpu = ci->ci_softc;
	uint64_t mbox_mask;

	ci->ci_data.cpu_nintr++;

	mbox_mask = READ_REG(cpu->sb1cpu_imr_base + R_IMR_MAILBOX_CPU);
	WRITE_REG(cpu->sb1cpu_imr_base + R_IMR_MAILBOX_CLR_CPU, mbox_mask);

	ipi_process(ci, mbox_mask);
}
#endif /* MULTIPROCESSOR */

void
sb1250_cpu_init(struct cpu_softc *cpu)
{
	const char * const xname = device_xname(cpu->sb1cpu_dev);
	struct evcnt * evcnts = cpu->sb1cpu_intr_evcnts;

	cpu->sb1cpu_imr_base =
	    MIPS_PHYS_TO_KSEG1(A_IMR_MAPPER(cpu->sb1cpu_ci->ci_cpuid));
#ifdef MULTIPROCESSOR
	cpu->sb1cpu_imr_all =
	    ~(M_INT_MBOX_0|M_INT_MBOX_1|M_INT_MBOX_2|M_INT_MBOX_3
	      |M_INT_WATCHDOG_TIMER_0|M_INT_WATCHDOG_TIMER_1);
#else
	cpu->sb1cpu_imr_all = ~(M_INT_WATCHDOG_TIMER_0|M_INT_WATCHDOG_TIMER_1);
#endif

	for (u_int i = 0; i < K_INT_SOURCES; i++, evcnts++) {
		WRITE_REG(cpu->sb1cpu_imr_base + SB1250_I_MAP(i), K_INT_MAP_I0);
		evcnt_attach_dynamic(evcnts, EVCNT_TYPE_INTR, NULL,
		    xname, sb1250_intr_names[i]);
	}
#if 0
	WRITE_REG(cpu->sb1cpu_imr_base + SB1250_I_MAP(K_INT_WATCHDOG_TIMER_0), K_INT_MAP_NMI);
	WRITE_REG(cpu->sb1cpu_imr_base + SB1250_I_MAP(K_INT_WATCHDOG_TIMER_1), K_INT_MAP_NMI);
#endif

	WRITE_REG(cpu->sb1cpu_imr_base + R_IMR_INTERRUPT_MASK, cpu->sb1cpu_imr_all);
#ifdef MULTIPROCESSOR
	if (sb1250_ihands[K_INT_MBOX_0].ih_fun == NULL) {
		/*
		 * For now, deliver all IPIs at IPL_SCHED.  Eventually
		 * some will be at IPL_VM.
		 */
		for (int irq = K_INT_MBOX_0; irq <= K_INT_MBOX_3; irq++)
			sb1250_intr_establish(irq, IPL_SCHED,
			    sb1250_ipi_intr, NULL);
	}
#endif /* MULTIPROCESSOR */
}

void
sb1250_ipl_map_init(void)
{
	ipl_sr_map = sb1250_ipl_sr_map;
}

void
sb1250_icu_init(void)
{
	const uint64_t imr_all = 0xffffffffffffffffULL;

	KASSERT(memcmp((const void *)&ipl_sr_map, (const void *)&sb1250_ipl_sr_map, sizeof(ipl_sr_map)) == 0);

	/* zero out the list of used interrupts/lines */
	memset(ints_for_ipl, 0, sizeof ints_for_ipl);
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
	vaddr_t imr = MIPS_PHYS_TO_KSEG1(A_IMR_CPU0_BASE + R_IMR_INTERRUPT_MASK);
	for (u_int i = 1; imr += IMR_REGISTER_SPACING, i < cpus; i++) {
		WRITE_REG(imr, imr_all);
	}
#endif /* MULTIPROCESSOR */
	WRITE_REG(MIPS_PHYS_TO_KSEG1(A_IMR_CPU0_BASE + R_IMR_INTERRUPT_MASK),
	    imr_all);

#ifdef MULTIPROCESSOR
	mips_locoresw.lsw_send_ipi = sb1250_lsw_send_ipi;
	mips_locoresw.lsw_cpu_init = sb1250_lsw_cpu_init;
#endif /* MULTIPROCESSOR */
}

static void
sb1250_cpu_intr(int ppl, vaddr_t pc, uint32_t status)
{
	struct cpu_info * const ci = curcpu();
	struct cpu_softc * const cpu = ci->ci_softc;
	const vaddr_t imr_base = cpu->sb1cpu_imr_base;
	struct evcnt * const evcnts = cpu->sb1cpu_intr_evcnts;
	uint32_t pending;
	int ipl;

	ci->ci_data.cpu_nintr++;

	while (ppl < (ipl = splintr(&pending))) {
		splx(ipl);

		/* XXX do something if 5? */
		if (pending & MIPS_INT_MASK_5) {
			uint32_t cycles = mips3_cp0_count_read();
			mips3_cp0_compare_write(cycles - 1);
			/* just leave the bugger disabled */
		}

		uint64_t sstatus = ints_for_ipl[ipl];
		sstatus &= READ_REG(imr_base + R_IMR_INTERRUPT_SOURCE_STATUS);
		while (sstatus != 0) {
#ifndef __mips_o32
			u_int n;
			__asm("dclz %0,%1" : "=r"(n) : "r"(sstatus));
#else
			u_int n = (sstatus >> 32)
			    ?  0 + __builtin_clz(sstatus >> 32)
			    : 32 + __builtin_clz((uint32_t)sstatus);
#endif
			u_int j = 63 - n;
			KASSERT(sstatus & (1ULL << j));
			sstatus ^= (1ULL << j);
			struct sb1250_ihand *ihp = &sb1250_ihands[j];
			KASSERT(ihp->ih_fun);
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
	struct cpu_softc * const cpu = curcpu()->ci_softc;
	struct sb1250_ihand * const ih = &sb1250_ihands[num];
	const int s = splhigh();

	if (num >= K_INT_SOURCES)
		panic("%s: invalid interrupt number (0x%x)", __func__, num);
	if (ipl >= _IPL_N || ipl < IPL_VM)
		panic("%s: invalid ipl %d", __func__, ipl);
	if (ih->ih_fun != NULL)
		panic("%s: cannot share sb1250 interrupts", __func__);

	ints_for_ipl[ipl] |= (1ULL << num);
	cpu->sb1cpu_imr_all &= ~(1ULL << num);

	ih->ih_fun = fun;
	ih->ih_arg = arg;
	ih->ih_ipl = ipl;

	if (num <= K_INT_WATCHDOG_TIMER_1)
		WRITE_REG(cpu->sb1cpu_imr_base + SB1250_I_MAP(num), K_INT_MAP_I4);
	else if (ipl > IPL_VM)
		WRITE_REG(cpu->sb1cpu_imr_base + SB1250_I_MAP(num), K_INT_MAP_I1);

	WRITE_REG(cpu->sb1cpu_imr_base + R_IMR_INTERRUPT_MASK, cpu->sb1cpu_imr_all);

	splx(s);

	return ih;
}
