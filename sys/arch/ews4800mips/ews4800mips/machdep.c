/*	$NetBSD: machdep.c,v 1.1 2005/12/29 15:20:08 tsutsui Exp $	*/

/*-
 * Copyright (c) 2001, 2004, 2005 The NetBSD Foundation, Inc.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.1 2005/12/29 15:20:08 tsutsui Exp $");

#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/user.h>
#include <sys/buf.h>
#include <sys/reboot.h>
#include <sys/mount.h>
#include <sys/kcore.h>
#include <sys/boot_flag.h>

#include <uvm/uvm_extern.h>

#include <machine/bootinfo.h>
#include <machine/locore.h>
#include <machine/sbdvar.h>	/* System board */
#include <machine/wired_map.h>

#include <mips/cache.h>

#ifdef DDB
#include <machine/db_machdep.h>
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>
#include <ddb/db_output.h>
#ifndef DB_ELFSIZE
#error Must define DB_ELFSIZE!
#endif
#define	ELFSIZE		DB_ELFSIZE
#include <sys/exec_elf.h>
#endif

#include <dev/cons.h>

#include <ews4800mips/ews4800mips/cons_machdep.h>

vsize_t kseg2iobufsize;		/* to reserve PTEs for KSEG2 I/O space */

/* our exported CPU info */
struct cpu_info cpu_info_store;

/* maps for VM objects */
struct vm_map *exec_map;
struct vm_map *mb_map;
struct vm_map *phys_map;

/* for buffer cache, vnode cache estimation */
int physmem;		/* max supported memory, changes to actual */

/* referenced by mips_machdep.c:cpu_dump() */
int mem_cluster_cnt;
phys_ram_seg_t mem_clusters[VM_PHYSSEG_MAX];

void mach_init(int, char *[], struct bootinfo *);
void option(int, char *[], struct bootinfo *);
/* NMI */
void nmi_exception(void);

void
mach_init(int argc, char *argv[], struct bootinfo *bi)
{
	extern char kernel_text[], edata[], end[];
	extern struct user *proc0paddr;
	caddr_t v;
	int i;

	/* Clear BSS */
	memset(edata, 0, end - edata);

	/* Setup early-console with BIOS ROM routines */
	rom_cons_init();

	/* Initialize machine dependent System-Board ops. */
	sbd_init();

	__asm volatile("move %0, $29" : "=r"(v));
	printf("kernel_text=%p edata=%p end=%p sp=%p\n", kernel_text, edata,
	    end, v);

	option(argc, argv, bi);

	uvm_setpagesize();

	/* Fill mem_clusters and mem_cluster_cnt */
	(*platform.mem_init)(kernel_text,
	    (bi && bi->bi_nsym) ? (caddr_t)bi->bi_esym : end);

	/*
	 * make sure that we don't call BIOS console from now
	 * because wired mappings set up by BIOS will be discarded
	 * in mips_vector_init().
	 */
	cn_tab = NULL;

	mips_vector_init();

	memcpy((void *)0x80000200, ews4800mips_nmi_vec, 32); /* NMI */
	mips_dcache_wbinv_all();
	mips_icache_sync_all();

	/* setup cpu_info */
	curcpu()->ci_cycles_per_hz = (curcpu()->ci_cpu_freq + hz / 2) / hz;
	curcpu()->ci_divisor_delay =
	    ((curcpu()->ci_cpu_freq + 500000) / 1000000);
	if (mips_cpu_flags & CPU_MIPS_DOUBLE_COUNT) {
		curcpu()->ci_cycles_per_hz /= 2;
		curcpu()->ci_divisor_delay /= 2;
	}
	MIPS_SET_CI_RECIPRICAL(curcpu());

	/* Load memory to UVM */
	for (i = 1; i < mem_cluster_cnt; i++) {
		phys_ram_seg_t *p;
		paddr_t start;
		size_t size;
		p = &mem_clusters[i];
		start = p->start;
		size = p->size;
		uvm_page_physload(atop(start), atop(start + size),
		    atop(start), atop(start + size), VM_FREELIST_DEFAULT);
	}

	sprintf(cpu_model, "NEC %s", platform.name);

	mips_init_msgbuf();

	pmap_bootstrap();

	v = (caddr_t)uvm_pageboot_alloc(USPACE);	/* proc0 USPACE */
	lwp0.l_addr = proc0paddr = (struct user *) v;
	lwp0.l_md.md_regs = (struct frame *)(v + USPACE) - 1;
	curpcb = &lwp0.l_addr->u_pcb;
	curpcb->pcb_context[11] = MIPS_INT_MASK | MIPS_SR_INT_IE; /* SR */
}

void
option(int argc, char *argv[], struct bootinfo *bi)
{
	extern char __boot_kernel_name[];
	boolean_t boot_device_set;
	char *p;
	int i;

	printf("argc=%d argv=%p syminfo=%p\n", argc, argv, bi);
	printf("version=%d size=%d nsym=%d ssym=%p esym=%p\n",
	    bi->bi_version, bi->bi_size, bi->bi_nsym, bi->bi_ssym, bi->bi_esym);

	for (i = 0; i < argc; i++)
		printf("[%d] %s\n", i, argv[i]);

#ifdef DDB
	/* Load symbol table */
	if (bi->bi_nsym)
		ksyms_init(bi->bi_esym - bi->bi_ssym,
		    (void *)bi->bi_ssym, (void *)bi->bi_esym);
#endif
	/* Parse option */
	boot_device_set = FALSE;
	for (i = 2; i < argc; i++) {
		p = argv[i];
		/* prompt for root device */
		if (p[0] == '-' && p[1] == 'a')
			boot_device_set = TRUE;

		/* root device option. ex) -b=net:netbsd, -b=sd0k:netbsd */
		if (p[0] == '-' && p[1] == 'b') {
			boot_device_set = TRUE;
			strcpy(__boot_kernel_name, p + 3);
		}
	}

	if (!boot_device_set)
		strcpy(__boot_kernel_name, argv[1]);

	/* Memory address infomation from IPL */
	sbd_memcluster_init(bi->bi_mainfo);
}

void
mips_machdep_cache_config(void)
{

	/* Set L2-cache size */
	if (platform.cache_config)
		(*platform.cache_config)();
}

void
cpu_startup(void)
{
	vaddr_t minaddr, maxaddr;
	char pbuf[9];

	printf("%s%s", copyright, version);
	printf("%s %dMHz\n", cpu_model, platform.cpu_clock / 1000000);
	format_bytes(pbuf, sizeof(pbuf), ctob(physmem));
	printf("total memory = %s\n", pbuf);

	minaddr = 0;
	/*
	 * Allocate a submap for exec arguments.  This map effectively
	 * limits the number of processes exec'ing at any time.
	 */
	exec_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
	    16 * NCARGS, VM_MAP_PAGEABLE, FALSE, NULL);
	/*
	 * Allocate a submap for physio.
	 */
	phys_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
	    VM_PHYS_SIZE, 0, FALSE, NULL);

	/*
	 * (No need to allocate an mbuf cluster submap.  Mbuf clusters
	 * are allocated via the pool allocator, and we use KSEG to
	 * map those pages.)
	 */
	format_bytes(pbuf, sizeof(pbuf), ptoa(uvmexp.free));
	printf("avail memory = %s\n", pbuf);
}

void
cpu_reboot(int howto, char *bootstr)
{
	static int waittime = -1;

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

	if ((howto & RB_POWERDOWN) == RB_POWERDOWN) {
		if (platform.poweroff) {
			DELAY(1000000);
			(*platform.poweroff)();
		}
	}

	if (howto & RB_HALT) {
		printf("System halted.  Hit any key to reboot.\n\n");
		(void)cngetc();
	}

	printf("rebooting...\n");
	DELAY(1000000);
	if (platform.reboot)
		(*platform.reboot)();

	printf("reboot faild.\n");
	for (;;)
		;
	/* NOTREACHED */
}

#ifdef DDB
void
__db_print_symbol(db_expr_t value)
{
	const char *name;
	db_expr_t offset;

	db_find_xtrn_sym_and_offset((db_addr_t)value, &name, &offset);

	if (name != NULL && offset <= db_maxoff && offset != value)
		db_print_loc_and_inst(value);
	else
		db_printf("\n");

}
#endif
