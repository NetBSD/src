/*	$NetBSD: vr.c,v 1.50 2008/01/04 22:13:57 ad Exp $	*/

/*-
 * Copyright (c) 1999-2002
 *         Shin Takemura and PocketBSD Project. All rights reserved.
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
 *	This product includes software developed by the PocketBSD project
 *	and its contributors.
 * 4. Neither the name of the project nor the names of its contributors
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
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: vr.c,v 1.50 2008/01/04 22:13:57 ad Exp $");

#include "opt_vr41xx.h"
#include "opt_tx39xx.h"
#include "opt_kgdb.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/reboot.h>
#include <sys/device.h>
#include <sys/bus.h>

#include <uvm/uvm_extern.h>

#include <machine/sysconf.h>
#include <machine/bootinfo.h>
#include <machine/bus_space_hpcmips.h>
#include <machine/platid.h>
#include <machine/platid_mask.h>

#include <dev/hpc/hpckbdvar.h>

#include <hpcmips/vr/vr.h>
#include <hpcmips/vr/vr_asm.h>
#include <hpcmips/vr/vrcpudef.h>
#include <hpcmips/vr/vripreg.h>
#include <hpcmips/vr/rtcreg.h>

#include <mips/cache.h>

#include "vrip_common.h"
#if NVRIP_COMMON > 0
#include <hpcmips/vr/vripvar.h>
#endif

#include "vrbcu.h"
#if NVRBCU > 0
#include <hpcmips/vr/bcuvar.h>
#endif

#include "vrdsu.h"
#if NVRDSU > 0
#include <hpcmips/vr/vrdsuvar.h>
#endif

#include "com.h"
#include "com_vrip.h"
#include "com_hpcio.h"
#if NCOM > 0
#include <sys/termios.h>
#include <sys/ttydefaults.h>
#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>
#if NCOM_VRIP > 0
#include <hpcmips/vr/siureg.h>
#include <hpcmips/vr/com_vripvar.h>
#endif
#if NCOM_HPCIO > 0
#include <hpcmips/dev/com_hpciovar.h>
#endif
#ifndef CONSPEED
#define CONSPEED TTYDEF_SPEED
#endif
#endif

#include "hpcfb.h"
#include "vrkiu.h"
#if (NVRKIU > 0) || (NHPCFB > 0)
#include <dev/wscons/wsdisplayvar.h>
#include <dev/rasops/rasops.h>
#endif

#if NHPCFB > 0
#include <dev/hpc/hpcfbvar.h>
#endif

#if NVRKIU > 0
#include <arch/hpcmips/vr/vrkiureg.h>
#include <arch/hpcmips/vr/vrkiuvar.h>
#endif

#ifdef DEBUG
#define STATIC
#else
#define STATIC	static
#endif

/*
 * This is a mask of bits to clear in the SR when we go to a
 * given interrupt priority level.
 */
const u_int32_t __ipl_sr_bits_vr[_IPL_N] = {
	0,					/* IPL_NONE */

	MIPS_SOFT_INT_MASK_0,			/* IPL_SOFTCLOCK */

	MIPS_SOFT_INT_MASK_0|
		MIPS_SOFT_INT_MASK_1,		/* IPL_SOFTNET */

	MIPS_SOFT_INT_MASK_0|
		MIPS_SOFT_INT_MASK_1|
		MIPS_INT_MASK_0,		/* IPL_VM */

	MIPS_SOFT_INT_MASK_0|
		MIPS_SOFT_INT_MASK_1|
		MIPS_INT_MASK_0|
		MIPS_INT_MASK_1,		/* IPL_SCHED */
};

#if defined(VR41XX) && defined(TX39XX)
#define	VR_INTR	vr_intr
#else
#define	VR_INTR	cpu_intr	/* locore_mips3 directly call this */
#endif

void vr_init(void);
void VR_INTR(u_int32_t, u_int32_t, u_int32_t, u_int32_t);
extern void vr_idle(void);
STATIC void vr_cons_init(void);
STATIC void vr_fb_init(void **);
STATIC void vr_mem_init(paddr_t);
STATIC void vr_find_dram(paddr_t, paddr_t);
STATIC void vr_reboot(int, char *);

/*
 * CPU interrupt dispatch table (HwInt[0:3])
 */
STATIC int vr_null_handler(void *, u_int32_t, u_int32_t);
STATIC int (*vr_intr_handler[4])(void *, u_int32_t, u_int32_t) = 
{
	vr_null_handler,
	vr_null_handler,
	vr_null_handler,
	vr_null_handler
};
STATIC void *vr_intr_arg[4];

#if NCOM > 0
/*
 * machine dependent serial console info
 */
static struct vr_com_platdep {
	platid_mask_t *platidmask;
	int (*attach)(bus_space_tag_t, int, int, int, tcflag_t, int);
	int addr;
	int freq;
} platdep_com_table[] = {
#if NCOM_HPCIO > 0
	{
		&platid_mask_MACH_NEC_MCR_SIGMARION2,
		com_hpcio_cndb_attach,	/* attach proc */
		0x0b600000,		/* base address */
		COM_FREQ,		/* frequency */
	},
#endif
#if NCOM_VRIP > 0
#ifdef VR4102
	{
		&platid_mask_CPU_MIPS_VR_4102,
		com_vrip_cndb_attach,	/* attach proc */
		VR4102_SIU_ADDR,	/* base address */
		VRCOM_FREQ,		/* frequency */
	},
#endif /* VR4102 */
#ifdef VR4111
	{
		&platid_mask_CPU_MIPS_VR_4111,
		com_vrip_cndb_attach,	/* attach proc */
		VR4102_SIU_ADDR,	/* base address */
		VRCOM_FREQ,		/* frequency */
	},
#endif /* VR4111 */
#ifdef VR4121
	{
		&platid_mask_CPU_MIPS_VR_4121,
		com_vrip_cndb_attach,	/* attach proc */
		VR4102_SIU_ADDR,	/* base address */
		VRCOM_FREQ,		/* frequency */
	},
#endif /* VR4121 */
#ifdef VR4122
	{
		&platid_mask_CPU_MIPS_VR_4122,
		com_vrip_cndb_attach,	/* attach proc */
		VR4122_SIU_ADDR,	/* base address */
		VRCOM_FREQ,		/* frequency */
	},
#endif /* VR4122 */
#ifdef VR4131
	{
		&platid_mask_CPU_MIPS_VR_4122,
		com_vrip_cndb_attach,	/* attach proc */
		VR4122_SIU_ADDR,	/* base address */
		VRCOM_FREQ,		/* frequency */
	},
#endif /* VR4131 */
#ifdef SINGLE_VRIP_BASE
	{
		&platid_wild,
		com_vrip_cndb_attach,	/* attach proc */
		VRIP_SIU_ADDR,		/* base address */
		VRCOM_FREQ,		/* frequency */
	},
#endif /* SINGLE_VRIP_BASE */
#else /* NCOM_VRIP > 0 */
	/* dummy */
	{
		&platid_wild,
		NULL,			/* attach proc */
		0,			/* base address */
		0,			/* frequency */
	},
#endif /* NCOM_VRIP > 0 */
};
#endif /* NCOM > 0 */

#if NVRKIU > 0
/*
 * machine dependent keyboard info
 */
static struct vr_kiu_platdep {
	platid_mask_t *platidmask;
	int addr;
} platdep_kiu_table[] = {
#ifdef VR4102
	{
		&platid_mask_CPU_MIPS_VR_4102,
		VR4102_KIU_ADDR,	/* base address */
	},
#endif /* VR4102 */
#ifdef VR4111
	{
		&platid_mask_CPU_MIPS_VR_4111,
		VR4102_KIU_ADDR,	/* base address */
	},
#endif /* VR4111 */
#ifdef VR4121
	{
		&platid_mask_CPU_MIPS_VR_4121,
		VR4102_KIU_ADDR,	/* base address */
	},
#endif /* VR4121 */
	{
		&platid_wild,
#ifdef SINGLE_VRIP_BASE
		VRIP_KIU_ADDR,		/* base address */
#else
		VRIP_NO_ADDR,		/* base address */
#endif /* SINGLE_VRIP_BASE */
	},
};
#endif /* NVRKIU > 0 */

void
vr_init()
{
	/*
	 * Platform Specific Function Hooks
	 */
	platform.cpu_idle	= vr_idle;
	platform.cpu_intr	= VR_INTR;
	platform.cons_init	= vr_cons_init;
	platform.fb_init	= vr_fb_init;
	platform.mem_init	= vr_mem_init;
	platform.reboot		= vr_reboot;

#if NVRBCU > 0
	sprintf(cpu_name, "NEC %s rev%d.%d %d.%03dMHz", 
		vrbcu_vrip_getcpuname(),
		vrbcu_vrip_getcpumajor(),
		vrbcu_vrip_getcpuminor(),
		vrbcu_vrip_getcpuclock() / 1000000,
		(vrbcu_vrip_getcpuclock() % 1000000) / 1000);
#else
	sprintf(cpu_name, "NEC VR41xx");
#endif
}

void
vr_mem_init(paddr_t kernend)
{

	mem_clusters[0].start = 0;
	mem_clusters[0].size = kernend;
	mem_cluster_cnt = 1;

	vr_find_dram(kernend, 0x02000000);
	vr_find_dram(0x02000000, 0x04000000);
	vr_find_dram(0x04000000, 0x06000000);
	vr_find_dram(0x06000000, 0x08000000);
}

void
vr_find_dram(paddr_t addr, paddr_t end)
{
	int n;
	char *page;
#ifdef NARLY_MEMORY_PROBE
	int x, i;
#endif

#ifdef VR_FIND_DRAMLIM
	if (VR_FIND_DRAMLIM < end)
		end = VR_FIND_DRAMLIM;
#endif /* VR_FIND_DRAMLIM */
	n = mem_cluster_cnt;
	for (; addr < end; addr += PAGE_SIZE) {

		page = (char *)MIPS_PHYS_TO_KSEG1(addr);
		if (badaddr(page, 4))
			goto bad;

		/* stop memory probing at first memory image */
		if (bcmp(page, (void *)MIPS_PHYS_TO_KSEG0(0), 128) == 0)
			return;

		*(volatile int *)(page+0) = 0xa5a5a5a5;
		*(volatile int *)(page+4) = 0x5a5a5a5a;
		wbflush();
		if (*(volatile int *)(page+0) != 0xa5a5a5a5)
			goto bad;

		*(volatile int *)(page+0) = 0x5a5a5a5a;
		*(volatile int *)(page+4) = 0xa5a5a5a5;
		wbflush();
		if (*(volatile int *)(page+0) != 0x5a5a5a5a)
			goto bad;

#ifdef NARLY_MEMORY_PROBE
		x = random();
		for (i = 0; i < PAGE_SIZE; i += 4)
			*(volatile int *)(page+i) = (x ^ i);
		wbflush();
		for (i = 0; i < PAGE_SIZE; i += 4)
			if (*(volatile int *)(page+i) != (x ^ i))
				goto bad;

		x = random();
		for (i = 0; i < PAGE_SIZE; i += 4)
			*(volatile int *)(page+i) = (x ^ i);
		wbflush();
		for (i = 0; i < PAGE_SIZE; i += 4)
			if (*(volatile int *)(page+i) != (x ^ i))
				goto bad;
#endif /* NARLY_MEMORY_PROBE */

		if (!mem_clusters[n].size)
			mem_clusters[n].start = addr;
		mem_clusters[n].size += PAGE_SIZE;
		continue;

	bad:
		if (mem_clusters[n].size)
			++n;
		continue;
	}
	if (mem_clusters[n].size)
		++n;
	mem_cluster_cnt = n;
}

void
vr_fb_init(void **kernend)
{
	/* Nothing to do */
}

void
vr_cons_init()
{
#if NCOM > 0 || NHPCFB > 0 || NVRKIU > 0
	bus_space_tag_t iot = hpcmips_system_bus_space();
#endif
#if NCOM > 0
	static struct vr_com_platdep *com_info;
#endif
#if NVRKIU > 0
	static struct vr_kiu_platdep *kiu_info;
#endif

#if NCOM > 0
	com_info = platid_search(&platid, platdep_com_table,
	    sizeof(platdep_com_table)/sizeof(*platdep_com_table),
	    sizeof(*platdep_com_table));
#ifdef KGDB
	if (com_info->attach != NULL) {
		/* if KGDB is defined, always use the serial port for KGDB */
		if ((*com_info->attach)(iot, com_info->addr, 9600,
		    com_info->freq,
		    (TTYDEF_CFLAG & ~(CSIZE | PARENB)) | CS8, 1)) {
			printf("%s(%d): can't init kgdb's serial port",
			    __FILE__, __LINE__);
		}
#else /* KGDB */
	if (com_info->attach != NULL && (bootinfo->bi_cnuse&BI_CNUSE_SERIAL)) {
		/* Serial console */
		if ((*com_info->attach)(iot, com_info->addr, CONSPEED,
		    com_info->freq,
		    (TTYDEF_CFLAG & ~(CSIZE | PARENB)) | CS8, 0)) {
			printf("%s(%d): can't init serial console",
			    __FILE__, __LINE__);
		} else {
			return;
		}
	}
#endif /* KGDB */
#endif /* NCOM > 0 */

#if NHPCFB > 0
	if (hpcfb_cnattach(NULL)) {
		printf("%s(%d): can't init fb console", __FILE__, __LINE__);
	} else {
		goto find_keyboard;
	}
 find_keyboard:
#endif /* NHPCFB > 0 */

#if NVRKIU > 0
	kiu_info = platid_search(&platid, platdep_kiu_table,
	    sizeof(platdep_kiu_table)/sizeof(*platdep_kiu_table),
	    sizeof(*platdep_kiu_table));
	if (kiu_info->addr != VRIP_NO_ADDR) {
		if (vrkiu_cnattach(iot, kiu_info->addr)) {
			printf("%s(%d): can't init vrkiu as console",
			    __FILE__, __LINE__);
		} else {
			return;
		}
	}
#endif /* NVRKIU > 0 */
}

extern char vr_hibernate[];
extern char evr_hibernate[];

void
vr_reboot(int howto, char *bootstr)
{
	/*
	 * power down
	 */
	if ((howto & RB_POWERDOWN) == RB_POWERDOWN) {
		printf("fake powerdown\n");
		/*
		 * copy vr_hibernate() to top of physical memory.
		 */
		memcpy((void *)MIPS_KSEG0_START, vr_hibernate,
		   evr_hibernate - (char *)vr_hibernate);
		/* sync I&D cache */
		mips_dcache_wbinv_all();
		mips_icache_sync_all();
		/*
		 * call vr_hibernate() at MIPS_KSEG0_START.
		 */
		((void (*)(void *,int))MIPS_KSEG0_START)(
		    (void *)MIPS_KSEG0_START, ptoa(physmem));
		/* not reach */
		vr_reboot(howto&~RB_HALT, bootstr);
	}
	/*
	 * halt
	 */
	if (howto & RB_HALT) {
#if NVRIP_COMMON > 0
		_spllower(~MIPS_INT_MASK_0);
		vrip_intr_suspend();
#else
		splhigh();
#endif
		__asm(".set noreorder");
		__asm(".word	" ___STRING(VR_OPCODE_SUSPEND));
		__asm("nop");
		__asm("nop");
		__asm("nop");
		__asm("nop");
		__asm("nop");
		__asm(".set reorder");
#if NVRIP_COMMON > 0
		vrip_intr_resume();
#endif
	}
	/*
	 * reset
	 */
#if NVRDSU
	vrdsu_reset();
#else
	printf("%s(%d): There is no DSU.", __FILE__, __LINE__);
#endif
}

/*
 * Handle interrupts.
 */
void
VR_INTR(u_int32_t status, u_int32_t cause, u_int32_t pc, u_int32_t ipending)
{
	uvmexp.intrs++;

	/* Deal with unneded compare interrupts occasionally so that we can
	 * keep spllowersoftclock. */
	if (ipending & MIPS_INT_MASK_5) {
		mips3_cp0_compare_write(0);
	}

	if (ipending & MIPS_INT_MASK_1) {
		_splset(MIPS_SR_INT_IE); /* for spllowersoftclock */
		/* Remove the lower priority pending bits from status so that
		 * spllowersoftclock will not happen if other interrupts are
		 * pending. */
		(*vr_intr_handler[1])(vr_intr_arg[1], pc, status & ~(ipending
		& (MIPS_INT_MASK_0|MIPS_SOFT_INT_MASK_0|MIPS_SOFT_INT_MASK_1)));
	}

	if (ipending & MIPS_INT_MASK_0) {
		_splset(MIPS_INT_MASK_1|MIPS_SR_INT_IE);
		(*vr_intr_handler[0])(vr_intr_arg[0], pc, status);
	}

#ifdef __HAVE_FAST_SOFTINTS
	if (ipending & MIPS_SOFT_INT_MASK_1) {
		_splset(MIPS_INT_MASK_1|MIPS_INT_MASK_0|MIPS_SR_INT_IE);
		softintr(MIPS_SOFT_INT_MASK_1);
	}

	if (ipending & MIPS_SOFT_INT_MASK_0) {
		_splset(MIPS_SOFT_INT_MASK_1|MIPS_INT_MASK_1|MIPS_INT_MASK_0|
		    MIPS_SR_INT_IE);
		softintr(MIPS_SOFT_INT_MASK_0);
	}
#endif
}

void *
vr_intr_establish(int line, int (*ih_fun)(void *, u_int32_t, u_int32_t),
    void *ih_arg)
{

	KDASSERT(vr_intr_handler[line] == vr_null_handler);

	vr_intr_handler[line] = ih_fun;
	vr_intr_arg[line] = ih_arg;

	return ((void *)line);
}

void
vr_intr_disestablish(void *ih)
{
	int line = (int)ih;

	vr_intr_handler[line] = vr_null_handler;
	vr_intr_arg[line] = NULL;
}

int
vr_null_handler(void *arg, u_int32_t pc, u_int32_t status)
{

	printf("vr_null_handler\n");

	return (0);
}

/*
int x4181 = VR4181;
int x4101 = VR4101;
int x4102 = VR4102;
int x4111 = VR4111;
int x4121 = VR4121;
int x4122 = VR4122;
int xo4181 = ONLY_VR4181;
int xo4101 = ONLY_VR4101;
int xo4102 = ONLY_VR4102;
int xo4111_4121 = ONLY_VR4111_4121;
int g4101=VRGROUP_4101;
int g4102=VRGROUP_4102;
int g4181=VRGROUP_4181;
int g4102_4121=VRGROUP_4102_4121;
int g4111_4121=VRGROUP_4111_4121;
int g4102_4122=VRGROUP_4102_4122;
int g4111_4122=VRGROUP_4111_4122;
int single_vrip_base=SINGLE_VRIP_BASE;
int vrip_base_addr=VRIP_BASE_ADDR;
*/
