/*	$NetBSD: machdep.c,v 1.89.4.1 2002/05/17 13:49:57 gehenna Exp $	*/

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1982, 1986, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * from: Utah $Hdr: machdep.c 1.74 92/12/20$
 *
 *	@(#)machdep.c	8.10 (Berkeley) 4/20/94
 */

#include "opt_ddb.h"
#include "opt_compat_hpux.h"
#include "opt_m060sp.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/signalvar.h>
#include <sys/kernel.h>
#include <sys/map.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/reboot.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/clist.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/msgbuf.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/mount.h>
#include <sys/user.h>
#include <sys/exec.h>
#include <sys/core.h>
#include <sys/kcore.h>
#include <sys/vnode.h>
#include <sys/syscallargs.h>

#if defined(DDB) && defined(__ELF__)
#include <sys/exec_elf.h>
#endif

#include <uvm/uvm_extern.h>

#include <sys/sysctl.h>

#include <machine/cpu.h>
#define _MVME68K_BUS_DMA_PRIVATE
#include <machine/bus.h>
#undef _MVME68K_BUS_DMA_PRIVATE
#include <machine/reg.h>
#include <machine/prom.h>
#include <machine/psl.h>
#include <machine/pte.h>
#include <machine/vmparam.h>
#include <m68k/include/cacheops.h>
#include <dev/cons.h>

#include <machine/kcore.h>	/* XXX should be pulled in by sys/kcore.h */

#include <mvme68k/dev/mainbus.h>
#include <mvme68k/mvme68k/seglist.h>

#ifdef DDB
#include <machine/db_machdep.h>
#include <ddb/db_extern.h>
#include <ddb/db_output.h>
#endif

#define	MAXMEM	64*1024	/* XXX - from cmap.h */

/* the following is used externally (sysctl_hw) */
char	machine[] = MACHINE;	/* from <machine/param.h> */

/* Our exported CPU info; we can have only one. */  
struct cpu_info cpu_info_store;

struct vm_map *exec_map = NULL;
struct vm_map *mb_map = NULL;
struct vm_map *phys_map = NULL;

/*
 * Model information, filled in by the Bug; see locore.s
 */
struct	mvmeprom_brdid  boardid;

caddr_t	msgbufaddr;		/* KVA of message buffer */
paddr_t msgbufpa;		/* PA of message buffer */

int	maxmem;			/* max memory per process */
int	physmem;		/* size of physical memory */

/*
 * safepri is a safe priority for sleep to set for a spin-wait
 * during autoconfiguration or after a panic.
 */
int	safepri = PSL_LOWIPL;

/*
 * The driver for the ethernet chip appropriate to the
 * platform (lance or i82586) will use this variable
 * to size the chip's packet buffer.
 */
#ifndef ETHER_DATA_BUFF_PAGES
#define	ETHER_DATA_BUFF_PAGES	4
#endif
u_long	ether_data_buff_size = ETHER_DATA_BUFF_PAGES * NBPG;
u_char	mvme_ea[6];

extern	u_int lowram;
extern	short exframesize[];

#ifdef COMPAT_HPUX
extern struct emul emul_hpux;
#endif

/* prototypes for local functions */ 
void	identifycpu __P((void));
void	initcpu __P((void));
void	dumpsys __P((void));

int	cpu_dumpsize __P((void));
int	cpu_dump __P((int (*)(dev_t, daddr_t, caddr_t, size_t), daddr_t *));
void	cpu_init_kcore_hdr __P((void));
u_long	cpu_dump_mempagecnt __P((void));
int	cpu_exec_aout_makecmds __P((struct proc *, struct exec_package *));
void	straytrap __P((int, u_short));

/*
 * Machine-independent crash dump header info.
 */
cpu_kcore_hdr_t cpu_kcore_hdr;

/*
 * Memory segments initialized in locore, which are eventually loaded
 * as managed VM pages.
 */
phys_seg_list_t phys_seg_list[VM_PHYSSEG_MAX];

/*
 * Memory segments to dump.  This is initialized from the phys_seg_list
 * before pages are stolen from it for VM system overhead.  I.e. this
 * covers the entire range of physical memory.
 */
phys_ram_seg_t mem_clusters[VM_PHYSSEG_MAX];
int	mem_cluster_cnt;

/*
 * On the 68020/68030 (mvme14x), the value of delay_divisor is roughly
 * 8192 / cpuspeed (where cpuspeed is in MHz).
 *
 * On the other boards (mvme162 and up), the cpuspeed is passed
 * in from the firmware.
 */
int	cpuspeed;		/* only used for printing later */
int	delay_divisor = 512;	/* assume some reasonable value to start */

/*
 * Since mvme68k boards can have anything from 4MB of onboard RAM, we
 * would rather set the PAGER_MAP_SIZE at runtime based on the amount
 * of onboard RAM.
 */
int	mvme68k_pager_map_size;

/* Machine-dependent initialization routines. */
void	mvme68k_init __P((void));

#ifdef MVME147
#include <mvme68k/dev/pccreg.h>
void	mvme147_init __P((void));
#endif

#if defined(MVME162) || defined(MVME167) || defined(MVME172) || defined(MVME177)
#include <dev/mvme/pcctworeg.h>
void	mvme1xx_init __P((void));
#endif

/*
 * Early initialization, right before main is called.
 */
void
mvme68k_init()
{
	int i;

	/*
	 * Set PAGER_MAP_SIZE to half the size of onboard RAM, up to a
	 * maximum of 16MB.
	 * (Note: Just use ps_end here since onboard RAM starts at 0x0)
	 */
	mvme68k_pager_map_size = phys_seg_list[0].ps_end / 2;
	if (mvme68k_pager_map_size > (16 * 1024 * 1024))
		mvme68k_pager_map_size = 16 * 1024 * 1024;

	/*
	 * Tell the VM system about available physical memory.
	 */
	for (i = 0; i < mem_cluster_cnt; i++) {
		if (phys_seg_list[i].ps_start == phys_seg_list[i].ps_end) {
			/*
			 * Segment has been completely gobbled up.
			 */
			continue;
		}
		/*
		 * Note the index of the mem cluster is the free
		 * list we want to put the memory on (0 == default,
		 * 1 == VME).  There can only be two.
		 */
		uvm_page_physload(atop(phys_seg_list[i].ps_start),
				 atop(phys_seg_list[i].ps_end),
				 atop(phys_seg_list[i].ps_start),
				 atop(phys_seg_list[i].ps_end), i);
	}

	switch (machineid) {
#ifdef MVME147
	case MVME_147:
		mvme147_init();
		break;
#endif
#ifdef MVME167
	case MVME_167:
#endif
#ifdef MVME162
	case MVME_162:
#endif
#ifdef MVME177
	case MVME_177:
#endif
#ifdef MVME172
	case MVME_172:
#endif
#if defined(MVME162) || defined(MVME167) || defined(MVME172) || defined(MVME177)
		mvme1xx_init();
		break;
#endif
	default:
		panic("mvme68k_init: impossible machineid");
	}

	/*
	 * Initialize error message buffer (at end of core).
	 */
	for (i = 0; i < btoc(round_page(MSGBUFSIZE)); i++)
		pmap_enter(pmap_kernel(), (vaddr_t)msgbufaddr + i * NBPG,
		    msgbufpa + i * NBPG, VM_PROT_READ|VM_PROT_WRITE,
		    VM_PROT_READ|VM_PROT_WRITE|PMAP_WIRED);
	initmsgbuf(msgbufaddr, round_page(MSGBUFSIZE));
	pmap_update(pmap_kernel());
}

#ifdef MVME147
/*
 * MVME-147 specific initialization.
 */
void
mvme147_init()
{
	bus_space_tag_t bt = &_mainbus_space_tag;
	bus_space_handle_t bh;

	/*
	 * Set up a temporary mapping to the PCC's registers
	 */
	bus_space_map(bt, intiobase_phys + MAINBUS_PCC_OFFSET, PCCREG_SIZE, 0, &bh);

	/*
	 * calibrate delay() using the 6.25 usec counter.
	 * we adjust the delay_divisor until we get the result we want.
	 */
	bus_space_write_1(bt, bh, PCCREG_TMR1_CONTROL, PCC_TIMERCLEAR);
	bus_space_write_2(bt, bh, PCCREG_TMR1_PRELOAD, 0);
	bus_space_write_1(bt, bh, PCCREG_TMR1_INTR_CTRL, 0);

	for (delay_divisor = 512; delay_divisor > 0; delay_divisor--) {
		bus_space_write_1(bt, bh, PCCREG_TMR1_CONTROL, PCC_TIMERSTART);
		delay(10000);
		bus_space_write_1(bt, bh, PCCREG_TMR1_CONTROL, PCC_TIMERSTOP);

		/* 1600 * 6.25usec == 10000usec */
		if (bus_space_read_2(bt, bh, PCCREG_TMR1_COUNT) > 1600)
			break;	/* got it! */

		bus_space_write_1(bt, bh, PCCREG_TMR1_CONTROL, PCC_TIMERCLEAR);
		/* retry! */
	}

	bus_space_unmap(bt, bh, PCCREG_SIZE);

	/* calculate cpuspeed */
	cpuspeed = 8192 / delay_divisor;
	cpuspeed *= 100;
}
#endif /* MVME147 */

#if defined(MVME162) || defined(MVME167) || defined(MVME172) || defined(MVME177)
int	get_cpuspeed __P((void));

/*
 * MVME-1[67]x specific initializaion.
 */
void
mvme1xx_init()
{
	bus_space_tag_t bt = &_mainbus_space_tag;
	bus_space_handle_t bh;

	/*
	 * Set up a temporary mapping to the PCCChip2's registers
	 */
	bus_space_map(bt, intiobase_phys + MAINBUS_PCCTWO_OFFSET + PCCTWO_REG_OFF,
	    PCC2REG_SIZE, 0, &bh);

	bus_space_write_1(bt, bh, PCC2REG_TIMER1_ICSR, 0);

	for (delay_divisor = (cputype == CPU_68060) ? 20 : 154;
	    delay_divisor > 0; delay_divisor--) {
		bus_space_write_4(bt, bh, PCC2REG_TIMER1_COUNTER, 0);
		bus_space_write_1(bt, bh, PCC2REG_TIMER1_CONTROL,
		    PCCTWO_TT_CTRL_CEN);
		delay(10000);
		bus_space_write_1(bt, bh, PCC2REG_TIMER1_CONTROL, 0);
		if (bus_space_read_4(bt, bh, PCC2REG_TIMER1_COUNTER) > 10000)
			break;	/* got it! */
	}

	bus_space_unmap(bt, bh, PCC2REG_SIZE);

	/* calculate cpuspeed */
	cpuspeed = get_cpuspeed();
	if (cpuspeed < 1250 || cpuspeed > 6000) {
		printf("mvme1xx_init: Warning! Firmware has " \
		    "bogus CPU speed: `%s'\n", boardid.speed);
		cpuspeed = ((cputype == CPU_68060) ? 1000 : 3072) /
		    delay_divisor;
		cpuspeed *= 100;
		printf("mvme1xx_init: Approximating speed using "\
		    "delay_divisor\n");
	}
}

/*
 * Parse the `speed' field of Bug's boardid structure.
 */
int
get_cpuspeed()
{
	int rv, i;

	for (i = 0, rv = 0; i < sizeof(boardid.speed); i++) {
		if (boardid.speed[i] < '0' || boardid.speed[i] > '9')
			return (0);
		rv = (rv * 10) + (boardid.speed[i] - '0');
	}

	return (rv);
}
#endif

/*
 * Console initialization: called early on from main,
 * before vm init or startup.  Do enough configuration
 * to choose and initialize a console.
 */
void
consinit()
{

	/*
	 * Initialize the console before we print anything out.
	 */
	cninit();

#ifdef DDB
	{
		extern int end;
		extern int *esym;

#ifndef __ELF__
		ddb_init(*(int *)&end, ((int *)&end) + 1, esym);
#else
		ddb_init((int)esym - (int)&end - sizeof(Elf32_Ehdr),
		    (void *)&end, esym);
#endif
	}
	if (boothowto & RB_KDB)
		Debugger();
#endif
}

/*
 * cpu_startup: allocate memory for variable-sized tables,
 * initialize cpu, and do autoconfiguration.
 */
void
cpu_startup()
{
	extern char *kernel_text, *etext;
	unsigned i;
	caddr_t v;
	int base, residual;
	u_quad_t vmememsize;
	vaddr_t minaddr, maxaddr;
	vsize_t size;
	char pbuf[9];
#ifdef DEBUG
	extern int pmapdebug;
	int opmapdebug = pmapdebug;

	pmapdebug = 0;
#endif

	/*
	 * Initialize the kernel crash dump header.
	 */
	cpu_init_kcore_hdr();

	/*
	 * Good {morning,afternoon,evening,night}.
	 */
	printf(version);
	identifycpu();
	format_bytes(pbuf, sizeof(pbuf), ctob(physmem));
	printf("total memory = %s", pbuf);

	for (vmememsize = 0, i = 1; i < mem_cluster_cnt; i++)
		vmememsize += mem_clusters[i].size;
	if (vmememsize != 0) {
		format_bytes(pbuf, sizeof(pbuf), mem_clusters[0].size);
		printf(" (%s on-board", pbuf);
		format_bytes(pbuf, sizeof(pbuf), vmememsize);
		printf(", %s VMEbus)", pbuf);
	}

	printf("\n");

	/*
	 * Find out how much space we need, allocate it,
	 * and then give everything true virtual addresses.
	 */
	size = (vsize_t)allocsys(NULL, NULL);
	if ((v = (caddr_t)uvm_km_zalloc(kernel_map, round_page(size))) == 0)
		panic("startup: no room for tables");
	if ((allocsys(v, NULL) - v) != size)
		panic("startup: table size inconsistency");


	/*
	 * Now allocate buffers proper.  They are different than the above
	 * in that they usually occupy more virtual memory than physical.
	 */
	size = MAXBSIZE * nbuf;
	if (uvm_map(kernel_map, (vaddr_t *) &buffers, round_page(size),
		    NULL, UVM_UNKNOWN_OFFSET, 0,
		    UVM_MAPFLAG(UVM_PROT_NONE, UVM_PROT_NONE, UVM_INH_NONE,
				UVM_ADV_NORMAL, 0)) != 0)
		panic("startup: cannot allocate VM for buffers");
	minaddr = (vaddr_t)buffers;
	base = bufpages / nbuf;
	residual = bufpages % nbuf;
	for (i = 0; i < nbuf; i++) {
		vsize_t curbufsize;
		vaddr_t curbuf;
		struct vm_page *pg;

		/*
		 * Each buffer has MAXBSIZE bytes of VM space allocated.  Of
		 * that MAXBSIZE space, we allocate and map (base+1) pages
		 * for the first "residual" buffers, and then we allocate
		 * "base" pages for the rest.
		 */
		curbuf = (vaddr_t) buffers + (i * MAXBSIZE);
		curbufsize = NBPG * ((i < residual) ? (base+1) : base);

		while (curbufsize) {
			pg = uvm_pagealloc(NULL, 0, NULL, 0);
			if (pg == NULL)
				panic("cpu_startup: not enough memory for "
				      "buffer cache");
			pmap_kenter_pa(curbuf, VM_PAGE_TO_PHYS(pg),
				       VM_PROT_READ|VM_PROT_WRITE);
			curbuf += PAGE_SIZE;
			curbufsize -= PAGE_SIZE;
		}
	}
	pmap_update(pmap_kernel());

	/*
	 * Allocate a submap for exec arguments.  This map effectively
	 * limits the number of processes exec'ing at any time.
	 */
	exec_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
				 16*NCARGS, VM_MAP_PAGEABLE, FALSE, NULL);
	/*
	 * Allocate a submap for physio
	 */
	phys_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
				 VM_PHYS_SIZE, 0, FALSE, NULL);

	/*
	 * Finally, allocate mbuf cluster submap.
	 */
	mb_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
				 nmbclusters * mclbytes, VM_MAP_INTRSAFE,
				 FALSE, NULL);

#ifdef DEBUG
	pmapdebug = opmapdebug;
#endif
	format_bytes(pbuf, sizeof(pbuf), ptoa(uvmexp.free));
	printf("avail memory = %s\n", pbuf);
	format_bytes(pbuf, sizeof(pbuf), bufpages * NBPG);
	printf("using %d buffers containing %s of memory\n", nbuf, pbuf);

	/*
	 * Tell the VM system that the area before the text segment
	 * is invalid.
	 *
	 * XXX Should just change KERNBASE and VM_MIN_KERNEL_ADDRESS,
	 * XXX but not right now.
	 */
	if (uvm_map_protect(kernel_map, 0, round_page((vaddr_t)&kernel_text),
	    UVM_PROT_NONE, TRUE) != 0)
		panic("can't mark pre-text pages off-limits");

	/*
	 * Tell the VM system that writing to the kernel text isn't allowed.
	 * If we don't, we might end up COW'ing the text segment!
	 */
	if (uvm_map_protect(kernel_map, trunc_page((vaddr_t)&kernel_text),
	    round_page((vaddr_t)&etext), UVM_PROT_READ|UVM_PROT_EXEC, TRUE)
	    != 0)
		panic("can't protect kernel text");

	/*
	 * Set up CPU-specific registers, cache, etc.
	 */
	initcpu();

	/*
	 * Set up buffers, so they can be used to read disk labels.
	 */
	bufinit();
}

/*
 * Set registers on exec.
 */
void
setregs(p, pack, stack)
	struct proc *p;
	struct exec_package *pack;
	u_long stack;
{
	struct frame *frame = (struct frame *)p->p_md.md_regs;
	extern void m68881_restore __P((struct fpframe *));

	frame->f_sr = PSL_USERSET;
	frame->f_pc = pack->ep_entry & ~1;
	frame->f_regs[D0] = 0;
	frame->f_regs[D1] = 0;
	frame->f_regs[D2] = 0;
	frame->f_regs[D3] = 0;
	frame->f_regs[D4] = 0;
	frame->f_regs[D5] = 0;
	frame->f_regs[D6] = 0;
	frame->f_regs[D7] = 0;
	frame->f_regs[A0] = 0;
	frame->f_regs[A1] = 0;
	frame->f_regs[A2] = (int)p->p_psstr;
	frame->f_regs[A3] = 0;
	frame->f_regs[A4] = 0;
	frame->f_regs[A5] = 0;
	frame->f_regs[A6] = 0;
	frame->f_regs[SP] = stack;

	/* restore a null state frame */
	p->p_addr->u_pcb.pcb_fpregs.fpf_null = 0;
	if (fputype)
		m68881_restore(&p->p_addr->u_pcb.pcb_fpregs);
}

/*
 * Info for CTL_HW
 */
char	cpu_model[124];

void
identifycpu()
{
	char board_str[16];
	char cpu_str[32];
	char mmu_str[16];
	char fpu_str[16];
	int len = 0;

	memset(cpu_model, 0, sizeof(cpu_model));
	memset(board_str, 0, sizeof(board_str));
	memset(cpu_str, 0, sizeof(cpu_str));
	memset(mmu_str, 0, sizeof(mmu_str));
	memset(fpu_str, 0, sizeof(cpu_str));

	/* Fill in the CPU string. */
	switch (cputype) {
#ifdef M68020
	case CPU_68020:
		sprintf(cpu_str, "MC68020 CPU");
		sprintf(fpu_str, "MC68881 FPU");	/* XXX */
		break;
#endif

#ifdef M68030
	case CPU_68030:
		sprintf(cpu_str, "MC68030 CPU+MMU");
		sprintf(fpu_str, "MC68882 FPU");	/* XXX */
		break;
#endif

#ifdef M68040
	case CPU_68040:
		sprintf(cpu_str, "MC68040 CPU+MMU+FPU");
		break;
#endif

#ifdef M68060
	case CPU_68060:
		sprintf(cpu_str, "MC68060 CPU+MMU+FPU");
		break;
#endif

	default:
		printf("unknown CPU type");
		panic("startup");
	}

	/* Fill in the MMU string; only need to handle one case. */
	switch (mmutype) {
	case MMU_68851:
		sprintf(mmu_str, "MC68851 MMU");
		break;
	}

	/* XXX Find out FPU type and fill in string here. */

	/* Fill in board model string. */
	switch (machineid) {
#ifdef MVME147
	case MVME_147: {
		char *suffix = (char *)&boardid.suffix;
		len = sprintf(board_str, "%x", machineid);
		if (suffix[0] != '\0') {
			board_str[len++] = suffix[0];
			if (suffix[1] != '\0')
				board_str[len++] = suffix[1];
		}
		break; }
#endif

#if defined(MVME162) || defined(MVME167) || defined(MVME172) || defined(MVME177)
	case MVME_162:
	case MVME_167:
	case MVME_172:
	case MVME_177: {
		char *suffix = (char *)&boardid.suffix;
		len = sprintf(board_str, "%x", machineid);
		if (suffix[0] != '\0') {
			board_str[len++] = suffix[0];
			if (suffix[1] != '\0')
				board_str[len++] = suffix[1];
		}
		break; }
#endif
	default:
		printf("unknown machine type: 0x%x\n", machineid);
		panic("startup");
	}

	len = sprintf(cpu_model, "Motorola MVME-%s: %d.%dMHz %s", board_str,
	    cpuspeed / 100, (cpuspeed % 100) / 10, cpu_str);

	cpuspeed /= 100;

	if (mmu_str[0] != '\0')
		len += sprintf(cpu_model + len, ", %s", mmu_str);

	if (fpu_str[0] != '\0')
		len += sprintf(cpu_model + len, ", %s", fpu_str);

#if defined(M68040) || defined(M68060)
	switch (cputype) {
#if defined(M68040)
	case CPU_68040:
		strcat(cpu_model, ", 4k+4k on-chip physical I/D caches");
		break;
#endif
#if defined(M68060)
	case CPU_68060:
		strcat(cpu_model, ", 8k+8k on-chip physical I/D caches");
		break;
#endif
	}
#endif

	printf("%s\n", cpu_model);
}

/*
 * machine dependent system variables.
 */
int
cpu_sysctl(name, namelen, oldp, oldlenp, newp, newlen, p)
	int *name;
	u_int namelen;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
	struct proc *p;
{
	dev_t consdev;

	/* all sysctl names at this level are terminal */
	if (namelen != 1)
		return (ENOTDIR);		/* overloaded */

	switch (name[0]) {
	case CPU_CONSDEV:
		if (cn_tab != NULL)
			consdev = cn_tab->cn_dev;
		else
			consdev = NODEV;
		return (sysctl_rdstruct(oldp, oldlenp, newp, &consdev,
		    sizeof consdev));
	default:
		return (EOPNOTSUPP);
	}
	/* NOTREACHED */
}

/* See: sig_machdep.c */

int	waittime = -1;

void
cpu_reboot(howto, bootstr)
	int howto;
	char *bootstr;
{
	extern void savectx __P((struct user *));

	/* take a snap shot before clobbering any registers */
	if (curproc && curproc->p_addr)
		savectx(curproc->p_addr);

	/* Save the RB_SBOOT flag. */
	howto |= (boothowto & RB_SBOOT);

	/* If system is hold, just halt. */
	if (cold) {
		howto |= RB_HALT;
		goto haltsys;
	}

	boothowto = howto;
	if ((howto & RB_NOSYNC) == 0 && waittime < 0) {
		waittime = 0;
		vfs_shutdown();
		/*
		 * If we've been adjusting the clock, the todr
		 * will be out of synch; adjust it now.
		 */
		resettodr();
	}

	/* Disable interrupts. */
	splhigh();

	/* If rebooting and a dump is requested, do it. */
	if (howto & RB_DUMP)
		dumpsys();

 haltsys:
	/* Run any shutdown hooks. */
	doshutdownhooks();

#if defined(PANICWAIT) && !defined(DDB)
	if ((howto & RB_HALT) == 0 && panicstr) {
		printf("hit any key to reboot...\n");
		(void)cngetc();
		printf("\n");
	}
#endif

	/* Finally, halt/reboot the system. */
	if (howto & RB_HALT) {
		printf("halted\n\n");
		doboot(RB_HALT);
		/* NOTREACHED */
	}

	printf("rebooting...\n");
	delay(1000000);
	doboot(RB_AUTOBOOT);
	/*NOTREACHED*/
}

/*
 * Initialize the kernel crash dump header.
 */
void
cpu_init_kcore_hdr()
{
	cpu_kcore_hdr_t *h = &cpu_kcore_hdr;
	struct m68k_kcore_hdr *m = &h->un._m68k;
	int i;
	extern char end[];

	memset(&cpu_kcore_hdr, 0, sizeof(cpu_kcore_hdr)); 

	/*
	 * Initialize the `dispatcher' portion of the header.
	 */
	strcpy(h->name, machine);
	h->page_size = NBPG;
	h->kernbase = KERNBASE;

	/*
	 * Fill in information about our MMU configuration.
	 */
	m->mmutype	= mmutype;
	m->sg_v		= SG_V;
	m->sg_frame	= SG_FRAME;
	m->sg_ishift	= SG_ISHIFT;
	m->sg_pmask	= SG_PMASK;
	m->sg40_shift1	= SG4_SHIFT1;
	m->sg40_mask2	= SG4_MASK2;
	m->sg40_shift2	= SG4_SHIFT2;
	m->sg40_mask3	= SG4_MASK3;
	m->sg40_shift3	= SG4_SHIFT3;
	m->sg40_addr1	= SG4_ADDR1;
	m->sg40_addr2	= SG4_ADDR2;
	m->pg_v		= PG_V;
	m->pg_frame	= PG_FRAME;

	/*
	 * Initialize pointer to kernel segment table.
	 */
	m->sysseg_pa = (u_int32_t)(pmap_kernel()->pm_stpa);

	/*
	 * Initialize relocation value such that:
	 *
	 *	pa = (va - KERNBASE) + reloc
	 *
	 * Since we're linked and loaded at the same place,
	 * and the kernel is mapped va == pa, this is 0.
	 */
	m->reloc = 0;

	/*
	 * Define the end of the relocatable range.
	 */
	m->relocend = (u_int32_t)end;

	/*
	 * The mvme68k has one or two memory segments.
	 */
	for (i = 0; i < mem_cluster_cnt; i++) {
		m->ram_segs[i].start = mem_clusters[i].start;
		m->ram_segs[i].size  = mem_clusters[i].size;
	}
}

/*
 * Compute the size of the machine-dependent crash dump header.
 * Returns size in disk blocks.
 */
int
cpu_dumpsize()
{
	int size;

	size = ALIGN(sizeof(kcore_seg_t)) + ALIGN(sizeof(cpu_kcore_hdr_t));
	return (btodb(roundup(size, dbtob(1))));
}

/*
 * Calculate size of RAM (in pages) to be dumped.
 */
u_long
cpu_dump_mempagecnt()
{
	u_long i, n;

	n = 0;
	for (i = 0; i < mem_cluster_cnt; i++)
		n += atop(mem_clusters[i].size);
	return (n);
}

/*
 * Called by dumpsys() to dump the machine-dependent header.
 */
int
cpu_dump(dump, blknop)
	int (*dump) __P((dev_t, daddr_t, caddr_t, size_t)); 
	daddr_t *blknop;
{
	int buf[dbtob(1) / sizeof(int)]; 
	cpu_kcore_hdr_t *chdr;
	kcore_seg_t *kseg;
	int error;

	kseg = (kcore_seg_t *)buf;
	chdr = (cpu_kcore_hdr_t *)&buf[ALIGN(sizeof(kcore_seg_t)) /
	    sizeof(int)];

	/* Create the segment header. */
	CORE_SETMAGIC(*kseg, KCORE_MAGIC, MID_MACHINE, CORE_CPU);
	kseg->c_size = dbtob(1) - ALIGN(sizeof(kcore_seg_t));

	memcpy(chdr, &cpu_kcore_hdr, sizeof(cpu_kcore_hdr_t));
	error = (*dump)(dumpdev, *blknop, (caddr_t)buf, sizeof(buf));
	*blknop += btodb(sizeof(buf));
	return (error);
}

/*
 * These variables are needed by /sbin/savecore
 */
u_int32_t dumpmag = 0x8fca0101;	/* magic number */
int	dumpsize = 0;		/* pages */
long	dumplo = 0;		/* blocks */

/*
 * This is called by main to set dumplo and dumpsize.
 * Dumps always skip the first NBPG of disk space
 * in case there might be a disk label stored there.
 * If there is extra space, put dump at the end to
 * reduce the chance that swapping trashes it.
 */
void
cpu_dumpconf()
{
	const struct bdevsw *bdev;
	int nblks, dumpblks;	/* size of dump area */

	if (dumpdev == NODEV)
		goto bad;
	bdev = bdevsw_lookup(dumpdev);
		panic("dumpconf: bad dumpdev=0x%x", dumpdev);
	if (bdev->d_psize == NULL)
		goto bad;
	nblks = (*bdev->d_psize)(dumpdev);
	if (nblks <= ctod(1))
		goto bad;

	dumpblks = cpu_dumpsize();
	if (dumpblks < 0)
		goto bad;
	dumpblks += ctod(cpu_dump_mempagecnt());

	/* If dump won't fit (incl. room for possible label), punt. */
	if (dumpblks > (nblks - ctod(1)))
		goto bad;

	/* Put dump at end of partition */
	dumplo = nblks - dumpblks;

	/* dumpsize is in page units, and doesn't include headers. */
	dumpsize = cpu_dump_mempagecnt();
	return;

 bad:
	dumpsize = 0;
}

/*
 * Dump physical memory onto the dump device.  Called by cpu_reboot().
 */
void
dumpsys()
{
	const struct bdevsw *bdev;
	u_long totalbytesleft, bytes, i, n, memcl;
	u_long maddr;
	int psize;
	daddr_t blkno;
	int (*dump) __P((dev_t, daddr_t, caddr_t, size_t));
	int error;

	/* XXX Should save registers. */

	if (dumpdev == NODEV)
		return;
	bdev = bdevsw_lookup(dumpdev);
	if (bdev == NULL || bdev->d_psize == NULL)
		return;

	/*
	 * For dumps during autoconfiguration,
	 * if dump device has already configured...
	 */
	if (dumpsize == 0)
		cpu_dumpconf();
	if (dumplo <= 0) {
		printf("\ndump to dev %u,%u not possible\n", major(dumpdev),
		    minor(dumpdev));
		return;
	}
	printf("\ndumping to dev %u,%u offset %ld\n", major(dumpdev),
	    minor(dumpdev), dumplo);

	psize = (*bdev->d_psize)(dumpdev);
	printf("dump ");
	if (psize == -1) {
		printf("area unavailable\n");
		return;
	}

	/* XXX should purge all outstanding keystrokes. */

	dump = bdev->d_dump;
	blkno = dumplo;

	if ((error = cpu_dump(dump, &blkno)) != 0)
		goto err;

	totalbytesleft = ptoa(cpu_dump_mempagecnt());

	for (memcl = 0; memcl < mem_cluster_cnt; memcl++) {
		maddr = mem_clusters[memcl].start;
		bytes = mem_clusters[memcl].size;

		for (i = 0; i < bytes; i += n, totalbytesleft -= n) {

			/* Print out how many MBs we have left to go. */
			if ((totalbytesleft % (1024*1024)) == 0)
				printf("%ld ", totalbytesleft / (1024 * 1024));

			/* Limit size for next transfer. */
			n = bytes - i;
			if (n > NBPG)
				n = NBPG;

			pmap_enter(pmap_kernel(), (vaddr_t)vmmap, maddr,
			    VM_PROT_READ, VM_PROT_READ|PMAP_WIRED);
			pmap_update(pmap_kernel());

			error = (*dump)(dumpdev, blkno, vmmap, n);
			if (error)
				goto err;
			maddr += n;
			blkno += btodb(n);
		}
	}

 err:
	switch (error) {

	case ENXIO:
		printf("device bad\n");
		break;

	case EFAULT:
		printf("device not ready\n");
		break;

	case EINVAL:
		printf("area improper\n");
		break;

	case EIO:
		printf("i/o error\n");
		break;

	case EINTR:
		printf("aborted from console\n");
		break;

	case 0:
		printf("succeeded\n");
		break;

	default:
		printf("error %d\n", error);
		break;
	}
	printf("\n\n");
	delay(5000);
}

void
initcpu()
{
#if defined(M68060)
	extern caddr_t vectab[256];
#if defined(M060SP)
	extern u_int8_t I_CALL_TOP[];
	extern u_int8_t FP_CALL_TOP[];
#else
	extern u_int8_t illinst;
#endif
	extern u_int8_t fpfault;
#endif

#ifdef MAPPEDCOPY
	extern u_int mappedcopysize;

	/*
	 * Initialize lower bound for doing copyin/copyout using
	 * page mapping (if not already set).  We don't do this on
	 * VAC machines as it loses big time.
	 */
	if (mappedcopysize == 0) {
		mappedcopysize = NBPG;
	}
#endif

#if defined(M68060)
	if (cputype == CPU_68060) {
#if defined(M060SP)
		/* integer support */
		vectab[61] = &I_CALL_TOP[128 + 0x00];

		/* floating point support */
		vectab[11] = &FP_CALL_TOP[128 + 0x30];
		vectab[55] = &FP_CALL_TOP[128 + 0x38];
		vectab[60] = &FP_CALL_TOP[128 + 0x40];

		vectab[54] = &FP_CALL_TOP[128 + 0x00];
		vectab[52] = &FP_CALL_TOP[128 + 0x08];
		vectab[53] = &FP_CALL_TOP[128 + 0x10];
		vectab[51] = &FP_CALL_TOP[128 + 0x18];
		vectab[50] = &FP_CALL_TOP[128 + 0x20];
		vectab[49] = &FP_CALL_TOP[128 + 0x28];
#else
		vectab[61] = &illinst;
#endif
		vectab[48] = &fpfault;
	}
	DCIS();
#endif
}

void
straytrap(pc, evec)
	int pc;
	u_short evec;
{
	printf("unexpected trap (vector offset %x) from %x\n",
	       evec & 0xFFF, pc);
}

/*
 * Level 7 interrupts are caused by e.g. the ABORT switch.
 *
 * If we have DDB, then break into DDB on ABORT.  In a production
 * environment, bumping the ABORT switch would be bad, so we enable
 * panic'ing on ABORT with the kernel option "PANICBUTTON".
 */
int
nmihand(arg)
	void *arg;
{
	mvme68k_abort("ABORT SWITCH");

	return 1;
}

/*
 * Common code for handling ABORT signals from buttons, switches,
 * serial lines, etc.
 */
void
mvme68k_abort(cp)
	const char *cp;
{
#ifdef DDB
	db_printf("%s\n", cp);
	Debugger();
#else
#ifdef PANICBUTTON
	panic(cp);
#else
	printf("%s ignored\n", cp);
#endif /* PANICBUTTON */
#endif /* DDB */
}

/*
 * cpu_exec_aout_makecmds():
 *	cpu-dependent a.out format hook for execve().
 * 
 * Determine of the given exec package refers to something which we
 * understand and, if so, set up the vmcmds for it.
 */
int
cpu_exec_aout_makecmds(p, epp)
    struct proc *p;
    struct exec_package *epp;
{
    return ENOEXEC;
}
