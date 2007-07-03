/*	$NetBSD: machdep.c,v 1.84 2007/07/03 10:31:57 tsutsui Exp $	*/

/*
 * Copyright (c) 2006 Izumi Tsutsui.
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

/*
 * Copyright (c) 2000 Soren S. Jorvang.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer.
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
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.84 2007/07/03 10:31:57 tsutsui Exp $");

#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_execfmt.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/reboot.h>
#include <sys/user.h>
#include <sys/mount.h>
#include <sys/kcore.h>
#include <sys/boot_flag.h>
#include <sys/ksyms.h>

#include <uvm/uvm_extern.h>

#include <mips/mips3_clock.h>
#include <machine/bootinfo.h>
#include <machine/bus.h>
#include <machine/cpu.h>
#include <machine/intr.h>
#include <machine/leds.h>
#include <machine/psl.h>

#include <mips/locore.h>

#include <dev/cons.h>

#include <dev/ic/i8259reg.h>
#include <dev/isa/isareg.h>

#include <cobalt/dev/gtreg.h>
#define GT_BASE		0x14000000	/* XXX */
#define PCIB_BASE	0x10000000	/* XXX */

#ifdef KGDB
#include <sys/kgdb.h>
#endif

#include "ksyms.h"

#if NKSYMS || defined(DDB) || defined(LKM)
#include <machine/db_machdep.h>
#include <ddb/db_extern.h>
#define ELFSIZE		DB_ELFSIZE
#include <sys/exec_elf.h>
#endif

/* Our exported CPU info; we can have only one. */
struct cpu_info cpu_info_store;

/* Maps for VM objects. */
struct vm_map *exec_map = NULL;
struct vm_map *mb_map = NULL;
struct vm_map *phys_map = NULL;

int	physmem;		/* Total physical memory */
char	*bootinfo = NULL;	/* pointer to bootinfo structure */

char	bootstring[512];	/* Boot command */
int	netboot;		/* Are we netbooting? */

char *	nfsroot_bstr = NULL;
char *	root_bstr = NULL;
int	bootunit = -1;
int	bootpart = -1;

int cpuspeed;

u_int cobalt_id;
static const char * const cobalt_model[] =
{
	NULL,
	NULL,
	NULL,
	"Cobalt Qube 2700",
	"Cobalt RaQ",
	"Cobalt Qube 2",
	"Cobalt RaQ 2"
};
#define COBALT_MODELS	__arraycount(cobalt_model)

phys_ram_seg_t mem_clusters[VM_PHYSSEG_MAX];
int mem_cluster_cnt;

void	mach_init(unsigned int, u_int, char*);
void	decode_bootstring(void);
static char *	strtok_light(char *, const char);
static u_int read_board_id(void);

/*
 * safepri is a safe priority for sleep to set for a spin-wait during
 * autoconfiguration or after a panic.  Used as an argument to splx().
 */
int	safepri = MIPS1_PSL_LOWIPL;

extern char *esym;
extern struct user *proc0paddr;



/*
 * Do all the stuff that locore normally does before calling main().
 */
void
mach_init(unsigned int memsize, u_int bim, char *bip)
{
	char *kernend, *v;
	u_long first, last;
	extern char edata[], end[];
	const char *bi_msg;
#if NKSYMS || defined(DDB) || defined(LKM)
	int nsym = 0;
	char *ssym = 0;
	struct btinfo_symtab *bi_syms;
#endif

	/*
	 * Clear the BSS segment (if needed).
	 */
	if (memcmp(((Elf_Ehdr *)end)->e_ident, ELFMAG, SELFMAG) == 0 &&
	    ((Elf_Ehdr *)end)->e_ident[EI_CLASS] == ELFCLASS) {
		esym = end;
#if NKSYMS || defined(DDB) || defined(LKM)
		esym += ((Elf_Ehdr *)end)->e_entry;
#endif
		kernend = (char *)mips_round_page(esym);
		/*
		 * We don't have to clear BSS here
		 * since our bootloader already does it.
		 */
#if 0
		memset(edata, 0, end - edata);
#endif
	} else {
		kernend = (void *)mips_round_page(end);
		/*
		 * No symbol table, so assume we are loaded by
		 * the firmware directly with "bfd" command.
		 * The firmware loader doesn't clear BSS of
		 * a loaded kernel, so do it here.
		 */
		memset(edata, 0, kernend - edata);

		/*
		 * XXX
		 * lwp0 and cpu_info_store are allocated in BSS
		 * and initialized before mach_init() is called,
		 * so restore them again.
		 */
		lwp0.l_cpu = &cpu_info_store;
		cpu_info_store.ci_curlwp = &lwp0;
	}

	/* Check for valid bootinfo passed from bootstrap */
	if (bim == BOOTINFO_MAGIC) {
		struct btinfo_magic *bi_magic;

		bootinfo = bip;
		bi_magic = lookup_bootinfo(BTINFO_MAGIC);
		if (bi_magic == NULL || bi_magic->magic != BOOTINFO_MAGIC)
			bi_msg = "invalid bootinfo structure.\n";
		else
			bi_msg = NULL;
	} else
		bi_msg = "invalid bootinfo (standalone boot?)\n";

#if NKSYMS || defined(DDB) || defined(LKM)
	bi_syms = lookup_bootinfo(BTINFO_SYMTAB);

	/* Load symbol table if present */
	if (bi_syms != NULL) {
		nsym = bi_syms->nsym;
		ssym = (void *)bi_syms->ssym;
		esym = (void *)bi_syms->esym;
		kernend = (void *)mips_round_page(esym);
	}
#endif

	cobalt_id = read_board_id();
	if (cobalt_id >= COBALT_MODELS || cobalt_model[cobalt_id] == NULL)
		sprintf(cpu_model, "Cobalt unknown model (board ID %u)",
		    cobalt_id);
	else
		strcpy(cpu_model, cobalt_model[cobalt_id]);

	switch (cobalt_id) {
	case COBALT_ID_QUBE2700:
	case COBALT_ID_RAQ:
		cpuspeed = 150; /* MHz */
		break;
	case COBALT_ID_QUBE2:
	case COBALT_ID_RAQ2:
		cpuspeed = 250; /* MHz */
		break;
	default:
		/* assume the fastest, so that delay(9) works */
		cpuspeed = 250;
		break;
	}
	curcpu()->ci_cpu_freq = cpuspeed * 1000 * 1000;
	curcpu()->ci_cycles_per_hz = (curcpu()->ci_cpu_freq + hz / 2) / hz;
	curcpu()->ci_divisor_delay =
	    ((curcpu()->ci_cpu_freq + (1000000 / 2)) / 1000000);
	/* all models have Rm5200, which is CPU_MIPS_DOUBLE_COUNT */
	curcpu()->ci_cycles_per_hz /= 2;
	curcpu()->ci_divisor_delay /= 2;
	MIPS_SET_CI_RECIPRICAL(curcpu());

	physmem = btoc(memsize - MIPS_KSEG0_START);

	consinit();

	if (bi_msg != NULL)
		printf(bi_msg);

	uvm_setpagesize();

	/*
	 * Copy exception-dispatch code down to exception vector.
	 * Initialize locore-function vector.
	 * Clear out the I and D caches.
	 */
	mips_vector_init();

	/*
	 * The boot command is passed in the top 512 bytes,
	 * so don't clobber that.
	 */
	mem_clusters[0].start = 0;
	mem_clusters[0].size = ctob(physmem) - 512;
	mem_cluster_cnt = 1;

	memcpy(bootstring, (char *)(memsize - 512), 512);
	memset((char *)(memsize - 512), 0, 512);
	bootstring[511] = '\0';

	decode_bootstring();

#if NKSYMS || defined(DDB) || defined(LKM)
	/* init symbols if present */
	if ((bi_syms != NULL) && (esym != NULL))
		ksyms_init(esym - ssym, ssym, esym);
	else
		ksyms_init(0, NULL, NULL);
#endif
#ifdef DDB
	if (boothowto & RB_KDB)
		Debugger();
#endif
#ifdef KGDB
	if (boothowto & RB_KDB)
		kgdb_connect(0);
#endif

	/*
	 * Load the rest of the available pages into the VM system.
	 */
	first = round_page(MIPS_KSEG0_TO_PHYS(kernend));
	last = mem_clusters[0].start + mem_clusters[0].size;
	uvm_page_physload(atop(first), atop(last), atop(first), atop(last),
	    VM_FREELIST_DEFAULT);

	/*
	 * Initialize error message buffer (at end of core).
	 */
	mips_init_msgbuf();

	pmap_bootstrap();

	/*
	 * Allocate space for proc0's USPACE.
	 */
	v = (char *)uvm_pageboot_alloc(USPACE);
	lwp0.l_addr = proc0paddr = (struct user *)v;
	lwp0.l_md.md_regs = (struct frame *)(v + USPACE) - 1;
	proc0paddr->u_pcb.pcb_context[11] =
	   MIPS_INT_MASK | MIPS_SR_INT_IE; /* SR */
}

/*
 * Allocate memory for variable-sized tables,
 */
void
cpu_startup(void)
{
	vaddr_t minaddr, maxaddr;
	char pbuf[9];

	/*
	 * Good {morning,afternoon,evening,night}.
	 */
	printf("%s%s", copyright, version);
	printf("%s\n", cpu_model);
	format_bytes(pbuf, sizeof(pbuf), ctob(physmem));
	printf("total memory = %s\n", pbuf);

	minaddr = 0;
	/*
	 * Allocate a submap for exec arguments.  This map effectively
	 * limits the number of processes exec'ing at any time.
	 */
	exec_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
	    16 * NCARGS, VM_MAP_PAGEABLE, false, NULL);
	/*
	 * Allocate a submap for physio.
	 */
	phys_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
	    VM_PHYS_SIZE, 0, false, NULL);

	/*
	 * (No need to allocate an mbuf cluster submap.  Mbuf clusters
	 * are allocated via the pool allocator, and we use KSEG to
	 * map those pages.)
	 */

	format_bytes(pbuf, sizeof(pbuf), ptoa(uvmexp.free));
	printf("avail memory = %s\n", pbuf);
}

static int waittime = -1;

void
cpu_reboot(int howto, char *bootstr)
{

	/* Take a snapshot before clobbering any registers. */
	if (curlwp)
		savectx((struct user *)curpcb);

	if (cold) {
		howto |= RB_HALT;
		goto haltsys;
	}

	/* If "always halt" was specified as a boot flag, obey. */
	if (boothowto & RB_HALT)
		howto |= RB_HALT;

	boothowto = howto;
	if ((howto & RB_NOSYNC) == 0 && (waittime < 0)) {
		waittime = 0;
		vfs_shutdown();

		/*
		 * If we've been adjusting the clock, the todr
		 * will be out of synch; adjust it now.
		 */
		resettodr();
	}

	splhigh();

	if (howto & RB_DUMP)
		dumpsys();

 haltsys:
	doshutdownhooks();

	if (howto & RB_HALT) {
		printf("\n");
		printf("The operating system has halted.\n");
		printf("Please press any key to reboot.\n\n");
		cnpollc(1);	/* For proper keyboard command handling */
		cngetc();
		cnpollc(0);
	}

	printf("rebooting...\n\n");
	delay(500000);

	*(volatile char *)MIPS_PHYS_TO_KSEG1(LED_ADDR) = LED_RESET;
	printf("WARNING: reboot failed!\n");

	for (;;)
		;
}


#define NCPU_INT	6
#define NICU_INT	16
#define IRQ_SLAVE	2

#define IO_ELCR		0x4d0
#define IO_ELCRSIZE	2
#define ELCR0		0
#define ELCR1		1

#define ICU1_READ(reg)		\
    bus_space_read_1(icu_bst, icu1_bsh, (reg))
#define ICU1_WRITE(reg, val)	\
    bus_space_write_1(icu_bst, icu1_bsh, (reg), (val))
#define ICU2_READ(reg)		\
    bus_space_read_1(icu_bst, icu2_bsh, (reg))
#define ICU2_WRITE(reg, val)	\
    bus_space_write_1(icu_bst, icu2_bsh, (reg), (val))
#define ELCR_READ(reg)		\
    bus_space_read_1(icu_bst, elcr_bsh, (reg))
#define ELCR_WRITE(reg, val)	\
    bus_space_write_1(icu_bst, elcr_bsh, (reg), (val))

const uint32_t mips_ipl_si_to_sr[SI_NQUEUES] = {
	[SI_SOFT] = MIPS_SOFT_INT_MASK_0,
	[SI_SOFTCLOCK] = MIPS_SOFT_INT_MASK_0,
	[SI_SOFTNET] = MIPS_SOFT_INT_MASK_1,
	[SI_SOFTSERIAL] = MIPS_SOFT_INT_MASK_1,
};

u_int icu_imen;

static bus_space_tag_t icu_bst;
static bus_space_handle_t icu1_bsh, icu2_bsh, elcr_bsh;
static struct cobalt_intrhand cpu_intrtab[NCPU_INT];
static struct cobalt_intrhand icu_intrtab[NICU_INT];

static int	icu_intr(void *);
static void	icu_reinit_irqs(void);
static u_int	icu_setmask(u_int);


void
icu_init(void)
{

	icu_bst = 0;	/* XXX unused on cobalt */
	bus_space_map(icu_bst, PCIB_BASE + IO_ICU1, IO_ICUSIZE, 0, &icu1_bsh);
	bus_space_map(icu_bst, PCIB_BASE + IO_ICU2, IO_ICUSIZE, 0, &icu2_bsh);
	bus_space_map(icu_bst, PCIB_BASE + IO_ELCR, IO_ELCRSIZE, 0, &elcr_bsh);

	/* Initialize master PIC */

	/* reset; program device, four bytes */
	ICU1_WRITE(PIC_ICW1, ICW1_SELECT | ICW1_IC4);
	/* starting at this vector index */
	ICU1_WRITE(PIC_ICW2, 0);			/* XXX */
	/* slave on line 2 */
	ICU1_WRITE(PIC_ICW3, ICW3_CASCADE(IRQ_SLAVE));
	/* special fully nested mode, 8086 mode */
	ICU1_WRITE(PIC_ICW4, ICW4_SFNM | ICW4_8086);
	/* special mask mode */
	ICU1_WRITE(PIC_OCW3, OCW3_SELECT | OCW3_SSMM | OCW3_SMM);
	/* read IRR by default */
	ICU1_WRITE(PIC_OCW3, OCW3_SELECT | OCW3_RR);

	/* Initialize slave PIC */

	/* reset; program device, four bytes */
	ICU2_WRITE(PIC_ICW1, ICW1_SELECT | ICW1_IC4);
	/* starting at this vector index */
	ICU2_WRITE(PIC_ICW2, 8);			/* XXX */
	/* slave connected to line 2 of master */
	ICU2_WRITE(PIC_ICW3, ICW3_SIC(IRQ_SLAVE));
	/* special fully nested mode, 8086 mode */
	ICU2_WRITE(PIC_ICW4, ICW4_SFNM | ICW4_8086);
	/* special mask mode */
	ICU2_WRITE(PIC_OCW3, OCW3_SELECT | OCW3_SSMM | OCW3_SMM);
	/* read IRR by default */
	ICU2_WRITE(PIC_OCW3, OCW3_SELECT | OCW3_RR);

	icu_setmask(0xffff);	/* mask all interrupts */

	/* default to edge-triggered */
	ELCR_WRITE(ELCR0, 0);
	ELCR_WRITE(ELCR1, 0);

	wbflush();

	cpu_intr_establish(4, IPL_NONE, icu_intr, NULL);
}

void *
icu_intr_establish(int irq, int type, int ipl, int (*func)(void *),
    void *arg)
{
	struct cobalt_intrhand *ih;

	ih = &icu_intrtab[irq];
	if (ih->ih_func != NULL)
		panic("icu_intr_establish(): irq %d is already in use", irq);

	ih->ih_cookie_type = COBALT_COOKIE_TYPE_ICU;
	ih->ih_func = func;
	ih->ih_arg = arg;
	ih->ih_type = type;
	snprintf(ih->ih_evname, sizeof(ih->ih_evname), "irq %d", irq);
	evcnt_attach_dynamic(&ih->ih_evcnt, EVCNT_TYPE_INTR, NULL, "icu",
	    ih->ih_evname);

	icu_reinit_irqs();

	return ih;
}

void
icu_intr_disestablish(void *cookie)
{
	struct cobalt_intrhand *ih = cookie;

	if (ih->ih_cookie_type == COBALT_COOKIE_TYPE_ICU) {
		ih->ih_func = NULL;
		ih->ih_arg = NULL;
		ih->ih_cookie_type = 0;
		ih->ih_type = IST_NONE;
		evcnt_detach(&ih->ih_evcnt);

		icu_reinit_irqs();
	}
}

void
icu_reinit_irqs(void)
{
	u_int i, irqs, elcr;

	/* unmask interrupts */
	irqs = 0;
	elcr = 0;
	for (i = 0; i < NICU_INT; i++) {
		if (icu_intrtab[i].ih_func) {
			irqs |= 1 << i;
			if (icu_intrtab[i].ih_type == IST_LEVEL)
				elcr |= 1 << i;
		}
	}
	if (irqs & 0xff00) /* any slave IRQs in use */
		irqs |= 1 << IRQ_SLAVE;
	icu_imen = ~irqs;

	ICU1_WRITE(PIC_OCW1, icu_imen);
	ICU2_WRITE(PIC_OCW1, icu_imen >> 8);

	ELCR_WRITE(ELCR0, elcr);
	ELCR_WRITE(ELCR1, elcr >> 8);
}

u_int
icu_setmask(u_int mask)
{
	u_int old;

	old = icu_imen;
	icu_imen = mask;
	ICU1_WRITE(PIC_OCW1, icu_imen);
	ICU2_WRITE(PIC_OCW1, icu_imen >> 8);

	return old;
}

int
icu_intr(void *arg)
{
	struct cobalt_intrhand *ih;
	int irq, handled;

	handled = 0;

	/* check requested irq */
	ICU1_WRITE(PIC_OCW3, OCW3_SELECT | OCW3_POLL);
	irq = ICU1_READ(PIC_OCW3);
	if ((irq & OCW3_POLL_PENDING) == 0)
		goto out;

	irq = OCW3_POLL_IRQ(irq);
	if (irq == IRQ_SLAVE) {
		ICU2_WRITE(PIC_OCW3, OCW3_SELECT | OCW3_POLL);
		irq = OCW3_POLL_IRQ(ICU2_READ(PIC_OCW3)) + 8;
	}

	ih = &icu_intrtab[irq];
	if (__predict_false(ih->ih_func == NULL)) {
		printf("icu_intr(): spurious interrupt (irq = %d)\n", irq);
	} else if (__predict_true((*ih->ih_func)(ih->ih_arg))) {
		ih->ih_evcnt.ev_count++;
		handled = 1;
	}

	/* issue EOI to ack */
	if (irq >= 8) {
		ICU2_WRITE(PIC_OCW2,
		    OCW2_SELECT | OCW2_SL | OCW2_EOI | OCW2_ILS(irq - 8));
		irq = IRQ_SLAVE;
	}
	ICU1_WRITE(PIC_OCW2, OCW2_SELECT | OCW2_SL | OCW2_EOI | OCW2_ILS(irq));

 out:
	return handled;
}

void *
cpu_intr_establish(int level, int ipl, int (*func)(void *), void *arg)
{
	struct cobalt_intrhand *ih;

	if (level < 0 || level >= NCPU_INT)
		panic("invalid interrupt level");

	ih = &cpu_intrtab[level];

	if (ih->ih_func != NULL)
		panic("cannot share CPU interrupts");

	ih->ih_cookie_type = COBALT_COOKIE_TYPE_CPU;
	ih->ih_func = func;
	ih->ih_arg = arg;
	snprintf(ih->ih_evname, sizeof(ih->ih_evname), "int %d", level);
	evcnt_attach_dynamic(&ih->ih_evcnt, EVCNT_TYPE_INTR, NULL,
	    "cpu", ih->ih_evname);

	return ih;
}

void
cpu_intr_disestablish(void *cookie)
{
	struct cobalt_intrhand *ih = cookie;

	if (ih->ih_cookie_type == COBALT_COOKIE_TYPE_CPU) {
		ih->ih_func = NULL;
		ih->ih_arg = NULL;
		ih->ih_cookie_type = 0;
		evcnt_detach(&ih->ih_evcnt);
	}
}

void
cpu_intr(uint32_t status, uint32_t cause, uint32_t pc, uint32_t ipending)
{
	struct clockframe cf;
	struct cobalt_intrhand *ih;

	uvmexp.intrs++;

	if (ipending & MIPS_INT_MASK_5) {
		/* call the common MIPS3 clock interrupt handler */ 
		cf.pc = pc;
		cf.sr = status;
		mips3_clockintr(&cf);

		cause &= ~MIPS_INT_MASK_5;
	}
	_splset((status & MIPS_INT_MASK_5) | MIPS_SR_INT_IE);

	if (__predict_false(ipending & MIPS_INT_MASK_0)) {
		/* GT64x11 timer0 */
		volatile uint32_t *irq_src =
		    (uint32_t *)MIPS_PHYS_TO_KSEG1(GT_BASE + GT_INTR_CAUSE);

		if (__predict_true((*irq_src & T0EXP) != 0)) {
			/* GT64x11 timer is no longer used for hardclock(9) */
			*irq_src = 0;
		}
		cause &= ~MIPS_INT_MASK_0;
	}
	_splset((status & ~cause & MIPS_HARD_INT_MASK) | MIPS_SR_INT_IE);

	if (ipending & MIPS_INT_MASK_3) {
		/* 16650 serial */
		ih = &cpu_intrtab[3];
		if (__predict_true(ih->ih_func != NULL)) {
			if (__predict_true((*ih->ih_func)(ih->ih_arg))) {
				cause &= ~MIPS_INT_MASK_3;
				ih->ih_evcnt.ev_count++;
			}
		}
	}
	_splset((status & ~cause & MIPS_HARD_INT_MASK) | MIPS_SR_INT_IE);

	if (ipending & MIPS_INT_MASK_1) {
		/* tulip primary */
		ih = &cpu_intrtab[1];
		if (__predict_true(ih->ih_func != NULL)) {
			if (__predict_true((*ih->ih_func)(ih->ih_arg))) {
				cause &= ~MIPS_INT_MASK_1;
				ih->ih_evcnt.ev_count++;
			}
		}
	}
	if (ipending & MIPS_INT_MASK_2) {
		/* tulip secondary */
		ih = &cpu_intrtab[2];
		if (__predict_true(ih->ih_func != NULL)) {
			if (__predict_true((*ih->ih_func)(ih->ih_arg))) {
				cause &= ~MIPS_INT_MASK_2;
				ih->ih_evcnt.ev_count++;
			}
		}
	}
	_splset((status & ~cause & MIPS_HARD_INT_MASK) | MIPS_SR_INT_IE);

	if (ipending & MIPS_INT_MASK_4) {
		/* ICU interrupts */
		ih = &cpu_intrtab[4];
		if (__predict_true(ih->ih_func != NULL)) {
			if (__predict_true((*ih->ih_func)(ih->ih_arg))) {
				cause &= ~MIPS_INT_MASK_4;
				/* evcnt for ICU is done in icu_intr() */
			}
		}
	}
	_splset((status & ~cause & MIPS_HARD_INT_MASK) | MIPS_SR_INT_IE);

	/* software interrupt */
	ipending &= (MIPS_SOFT_INT_MASK_1|MIPS_SOFT_INT_MASK_0);
	if (ipending == 0)
		return;

	_clrsoftintr(ipending);

	softintr_dispatch(ipending);
}


void
decode_bootstring(void)
{
	char *work;
	char *equ;
	int i;

	/* break apart bootstring on ' ' boundries  and itterate*/
	work = strtok_light(bootstring, ' ');
	while (work != '\0') {
		/* if starts with '-', we got options, walk its decode */
		if (work[0] == '-') {
			i = 1;
			while (work[i] != ' ' && work[i] != '\0') {
				BOOT_FLAG(work[i], boothowto);
				i++;
			}
		} else

		/* if it has a '=' its an assignment, switch and set */
		if ((equ = strchr(work,'=')) != '\0') {
			if(0 == memcmp("nfsroot=", work, 8)) {
				nfsroot_bstr = (equ +1);
			} else
			if(0 == memcmp("root=", work, 5)) {
				root_bstr = (equ +1);
			}
		} else

		/* else it a single value, switch and process */
		if (memcmp("single", work, 5) == 0) {
			boothowto |= RB_SINGLE;
		} else
		if (memcmp("ro", work, 2) == 0) {
			/* this is also inserted by the firmware */
		}

		/* grab next token */
		work = strtok_light(NULL, ' ');
	}

	if (root_bstr != NULL) {
		/* this should be of the form "/dev/hda1" */
		/* [abcd][1234]    drive partition  linux probe order */
		if ((memcmp("/dev/hd",root_bstr,7) == 0) &&
		    (strlen(root_bstr) == 9) ){
			bootunit = root_bstr[7] - 'a';
			bootpart = root_bstr[8] - '1';
		}
	}
}


static char *
strtok_light(char *str, const char sep)
{
	static char *proc;
	char *head;
	char *work;

	if (str != NULL)
		proc = str;
	if (proc == NULL)  /* end of string return NULL */
		return proc;

	head = proc;

	work = strchr (proc, sep);
	if (work == NULL) {  /* we hit the end */
		proc = work;
	} else {
		proc = (work +1 );
		*work = '\0';
	}

	return head;
}

/*
 * Look up information in bootinfo of boot loader.
 */
void *
lookup_bootinfo(int type)
{
	struct btinfo_common *bt;
	char *help = bootinfo;

	/* Check for a bootinfo record first. */
	if (help == NULL) {
		printf("##### help == NULL\n");
		return NULL;
	}

	do {
		bt = (struct btinfo_common *)help;
		printf("Type %d @0x%x\n", bt->type, (u_int)bt);
		if (bt->type == type)
			return (void *)help;
		help += bt->next;
	} while (bt->next != 0 &&
	    (size_t)help < (size_t)bootinfo + BOOTINFO_SIZE);

	return NULL;
}

/*
 * Get board ID of cobalt models.
 * 
 * The board ID info is stored at the PCI config register
 * on the PCI-ISA bridge part of the VIA VT82C586 chipset.
 * We can't use pci_conf_read(9) yet here, so read it directly.
 */
static u_int
read_board_id(void)
{
	volatile uint32_t *pcicfg_addr, *pcicfg_data;
	uint32_t reg;

#define PCIB_PCI_BUS		0
#define PCIB_PCI_DEV		9
#define PCIB_PCI_FUNC		0
#define PCIB_BOARD_ID_REG	0x94
#define COBALT_BOARD_ID(reg)	((reg & 0x000000f0) >> 4)

	pcicfg_addr = (uint32_t *)MIPS_PHYS_TO_KSEG1(GT_BASE + GT_PCICFG_ADDR);
	pcicfg_data = (uint32_t *)MIPS_PHYS_TO_KSEG1(GT_BASE + GT_PCICFG_DATA);

	*pcicfg_addr = PCICFG_ENABLE |
	    (PCIB_PCI_BUS << 16) | (PCIB_PCI_DEV << 11) | (PCIB_PCI_FUNC << 8) |
	    PCIB_BOARD_ID_REG;
	reg = *pcicfg_data;
	*pcicfg_addr = 0;

	return COBALT_BOARD_ID(reg); 
}

static const int ipl2spl_table[] = {
	[IPL_NONE] = 0,
	[IPL_SOFTCLOCK] = MIPS_SOFT_INT_MASK_0,
	[IPL_SOFTNET] = MIPS_SOFT_INT_MASK_0|MIPS_SOFT_INT_MASK_1,
	[IPL_SOFTSERIAL] = MIPS_SOFT_INT_MASK_0|MIPS_SOFT_INT_MASK_1,
	[IPL_BIO] = SPLBIO,
	[IPL_NET] = SPLNET,
	[IPL_TTY] = SPLTTY,
	[IPL_VM] = SPLCLOCK,
	[IPL_CLOCK] = SPLCLOCK,
	[IPL_STATCLOCK] = SPLCLOCK,
	[IPL_HIGH] = MIPS_INT_MASK,
};

ipl_cookie_t
makeiplcookie(ipl_t ipl)
{

	return (ipl_cookie_t){._spl = ipl2spl_table[ipl]};
}
