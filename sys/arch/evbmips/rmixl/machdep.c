/*	machdep.c,v 1.1.2.34 2011/04/29 08:26:18 matt Exp	*/

/*
 * Copyright 2001, 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe and Simon Burge for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department, The Mach Operating System project at
 * Carnegie-Mellon University and Ralph Campbell.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
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
 *	@(#)machdep.c   8.3 (Berkeley) 1/12/94
 *	from: Utah Hdr: machdep.c 1.63 91/04/24
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "machdep.c,v 1.1.2.34 2011/04/29 08:26:18 matt Exp");

#define __INTR_PRIVATE
#define __MUTEX_PRIVATE
#define _MIPS_BUS_DMA_PRIVATE

#include "opt_multiprocessor.h"
#include "opt_ddb.h"
#include "opt_com.h"
#include "opt_execfmt.h"
#include "opt_memsize.h"
#include "rmixl_pcix.h"
#include "rmixl_pcie.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/buf.h>
#include <sys/reboot.h>
#include <sys/mount.h>
#include <sys/kcore.h>
#include <sys/boot_flag.h>
#include <sys/termios.h>
#include <sys/ksyms.h>
#include <sys/intr.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/extent.h>
#include <sys/malloc.h>

#include <uvm/uvm_extern.h>

#include <dev/cons.h>

#include "ksyms.h"

#if NKSYMS || defined(DDB) || defined(LKM)
#include <mips/db_machdep.h>
#include <ddb/db_extern.h>
#endif

#include <mips/cpu.h>
#include <mips/psl.h>
#include <mips/cache.h>
#include <mips/mipsNN.h>
#include <mips/mips_opcode.h>
#include <mips/pte.h>

#include "com.h"
#if NCOM == 0
#error no serial console
#endif

#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pciconf.h>

#include <mips/rmi/rmixlreg.h>
#include <mips/rmi/rmixlvar.h>
#include <mips/rmi/rmixl_intr.h>
#include <mips/rmi/rmixl_firmware.h>
#include <mips/rmi/rmixl_comvar.h>
#include <mips/rmi/rmixl_pcievar.h>
#include <mips/rmi/rmixl_pcixvar.h>

//#define MACHDEP_DEBUG 1
#ifdef MACHDEP_DEBUG
int machdep_debug=MACHDEP_DEBUG;
# define DPRINTF(x,...)	do { if (machdep_debug) printf(x, ## __VA_ARGS__); } while(0)
#else
# define DPRINTF(x,...)
#endif

#ifdef __HAVE_PCI_CONF_HOOK
static int rmixl_pci_conf_hook(void *, int, int, int, pcireg_t);
#endif

#ifndef CONSFREQ
# define CONSFREQ 66000000
#endif
#ifndef CONSPEED
# define CONSPEED 38400
#endif
#ifndef CONMODE
# define CONMODE ((TTYDEF_CFLAG & ~(CSIZE | PARENB)) | CS8)
#endif
#ifndef CONSADDR
# define CONSADDR 0
#endif

int		comcnfreq  = CONSFREQ;
int		comcnspeed = CONSPEED;
tcflag_t	comcnmode  = CONMODE;
bus_addr_t	comcnaddr  = (bus_addr_t)CONSADDR;

struct rmixl_config rmixl_configuration = {
	.rc_io = {
		.r_pbase = (bus_addr_t)-1,
	},
	.rc_flash[0] = {
		.r_pbase = (bus_addr_t)-1,
	},
	.rc_flash[1] = {
		.r_pbase = (bus_addr_t)-1,
	},
	.rc_flash[2] = {
		.r_pbase = (bus_addr_t)-1,
	},
	.rc_flash[3] = {
		.r_pbase = (bus_addr_t)-1,
	},
	.rc_pci_cfg = {
		.r_pbase = (bus_addr_t)-1,
	},
	.rc_pci_ecfg = {
		.r_pbase = (bus_addr_t)-1,
	},
	.rc_pci_mem = {
		.r_pbase = (bus_addr_t)-1,
	},
	.rc_pci_io = {
		.r_pbase = (bus_addr_t)-1,
	},
	.rc_pci_link_mem[0] = {
		.r_pbase = (bus_addr_t)-1,
	},
	.rc_pci_link_mem[1] = {
		.r_pbase = (bus_addr_t)-1,
	},
	.rc_pci_link_mem[2] = {
		.r_pbase = (bus_addr_t)-1,
	},
	.rc_pci_link_mem[3] = {
		.r_pbase = (bus_addr_t)-1,
	},
	.rc_pci_link_io[0] = {
		.r_pbase = (bus_addr_t)-1,
	},
	.rc_pci_link_io[1] = {
		.r_pbase = (bus_addr_t)-1,
	},
	.rc_pci_link_io[2] = {
		.r_pbase = (bus_addr_t)-1,
	},
	.rc_pci_link_io[3] = {
		.r_pbase = (bus_addr_t)-1,
	},
	.rc_srio_mem = {
		.r_pbase = (bus_addr_t)-1,
	},
	/*
	 * Staticly initialize the 64-bit dmatag.
	 */
	.rc_dmat64 = &rmixl_configuration.rc_dma_tag,
	.rc_dma_tag = {
		._cookie = &rmixl_configuration.rc_dma_tag,
		._dmamap_ops = _BUS_DMAMAP_OPS_INITIALIZER,
		._dmamem_ops = _BUS_DMAMEM_OPS_INITIALIZER,
		._dmatag_ops = _BUS_DMATAG_OPS_INITIALIZER,
	},
#ifdef __HAVE_PCI_CONF_HOOK
	.rc_pci_chipset = {
		.pc_conf_hook = rmixl_pci_conf_hook,
	}
#endif
};

#ifdef ENABLE_MIPS_KSEGX
pt_entry_t mips_ksegx_pte;
paddr_t mips_ksegx_start;
#endif

/*
 * array of tested firmware versions
 * if you find new ones and they work
 * please add them
 */
typedef struct rmiclfw_psb_id {
	uint64_t		psb_version;
	rmixlfw_psb_type_t	psb_type;
} rmiclfw_psb_id_t;
static rmiclfw_psb_id_t rmiclfw_psb_id[] = {
	{	0x4958d4fb00000056ULL, PSB_TYPE_RMI  },
	{	0x4aacdb6a00000056ULL, PSB_TYPE_RMI  },
	{	0x4b67d03200000056ULL, PSB_TYPE_RMI  },
	{	0x4c17058b00000056ULL, PSB_TYPE_RMI  },
	{	0x49a5a8fa00000056ULL, PSB_TYPE_DELL },
	{	0x4b8ead3100000056ULL, PSB_TYPE_DELL },
};
#define RMICLFW_PSB_VERSIONS_LEN \
	(sizeof(rmiclfw_psb_id)/sizeof(rmiclfw_psb_id[0]))

/*
 * storage for fixed extent used to allocate physical address regions
 * because extent(9) start and end values are u_long, they are only
 * 32 bits on a 32 bit kernel, which is insuffucuent since XLS physical
 * address is 40 bits wide.  So the "physaddr" map stores regions
 * in units of megabytes.
 */
static u_long rmixl_physaddr_storage[
	EXTENT_FIXED_STORAGE_SIZE(32)/sizeof(u_long)
];

/* For sysctl_hw. */
extern char cpu_model[];

/* Our exported CPU info; we can have only one. */
struct cpu_info cpu_info_store;

/* Maps for VM objects. */
struct vm_map *mb_map = NULL;
struct vm_map *phys_map = NULL;

int	physmem;		/* Total physical memory */

int	netboot;		/* Are we netbooting? */


phys_ram_seg_t mem_clusters[VM_PHYSSEG_MAX];
u_quad_t mem_cluster_maxaddr;
u_int mem_cluster_cnt;


void configure(void);
void mach_init(int, int32_t *, void *, int64_t);
static uint64_t rmixlfw_init(int64_t);
static uint64_t mem_clusters_init(rmixlfw_mmap_t *, rmixlfw_mmap_t *);
static void __attribute__((__noreturn__)) rmixl_reset(void);
static uint64_t rmixl_physaddr_init(void);
static u_int ram_seg_resv(phys_ram_seg_t *, u_int, u_quad_t, u_quad_t);
void rmixlfw_mmap_print(const char *, rmixlfw_mmap_t *);


#ifdef MULTIPROCESSOR
static bool rmixl_fixup_cop0_oscratch(int32_t, uint32_t [2]);
void rmixl_get_wakeup_info(struct rmixl_config *);
#ifdef MACHDEP_DEBUG
static void rmixl_wakeup_info_print(volatile rmixlfw_cpu_wakeup_info_t *);
#endif	/* MACHDEP_DEBUG */
#endif	/* MULTIPROCESSOR */
static void rmixl_fixup_curcpu(void);

#if NCOM > 0
static volatile uint32_t *rmixl_com0addr;

static int
rmixl_cngetc(dev_t dv)
{
	volatile uint32_t * const com0addr = rmixl_com0addr;

        if ((be32toh(com0addr[com_lsr]) & LSR_RXRDY) == 0)
		return -1;

	return be32toh(com0addr[com_data]) & 0xff;
}

static void
rmixl_cnputc(dev_t dv, int c)
{
	volatile uint32_t * const com0addr = rmixl_com0addr;
	int timo = 150000;

	while ((be32toh(com0addr[com_lsr]) & LSR_TXRDY) == 0 && --timo > 0)
		;

	com0addr[com_data] = htobe32(c);
	__asm __volatile("sync");

	while ((be32toh(com0addr[com_lsr]) & LSR_TSRE) == 0 && --timo > 0)
		;
}

struct consdev rmixl_earlycons = {
	.cn_putc = rmixl_cnputc,
	.cn_getc = rmixl_cngetc,
	.cn_pollc = nullcnpollc,
};
#endif

/*
 * Do all the stuff that locore normally does before calling main().
 */
void
mach_init(int argc, int32_t *argv, void *envp, int64_t infop)
{
	struct rmixl_config *rcp = &rmixl_configuration;
	void *kernend;
	uint64_t memsize;
	extern char edata[], end[];
	size_t fl_count = 0;
	struct mips_vmfreelist fl[1];
	bool uboot_p = false;

	const uint32_t cfg0 = mips3_cp0_config_read();
#if (MIPS64_XLR + MIPS64_XLS) > 0 && (MIPS64_XLP) == 0
	const bool is_xlp_p = false	/* make sure cfg0 is used */
	    && MIPSNN_GET(CFG_AR, cfg0) == MIPSNN_CFG_AR_REV2;
	KASSERT(MIPSNN_GET(CFG_AR, cfg0) == MIPSNN_CFG_AR_REV1);
#elif (MIPS64_XLR + MIPS64_XLS) == 0 && (MIPS64_XLP) > 0
	const bool is_xlp_p = true	/* make sure cfg0 is used */
	    || MIPSNN_GET(CFG_AR, cfg0) == MIPSNN_CFG_AR_REV2;
	KASSERT(MIPSNN_GET(CFG_AR, cfg0) == MIPSNN_CFG_AR_REV2);
#else
	const bool is_xlp_p = (MIPSNN_GET(CFG_AR, cfg0) == MIPSNN_CFG_AR_REV2);
#endif

	rmixl_pcr_init_core(is_xlp_p);

#ifdef MULTIPROCESSOR
	__asm __volatile("dmtc0 %0,$%1,2"
	    ::	"r"(&pmap_tlb0_info.ti_hwlock->mtx_lock),
		"n"(MIPS_COP_0_OSSCRATCH));
#endif

	/*
	 * Clear the BSS segment.
	 */
	kernend = (void *)mips_round_page(end);
	memset(edata, 0, (char *)kernend - edata);

#if NCOM > 0
	/*
	 * If no comcnaddr has been set, pick an appropriate one.
	 */
	if (comcnaddr == 0) {
		comcnaddr = is_xlp_p
		    ? RMIXLP_UART1_PCITAG
		    : RMIXL_IO_DEV_UART_1;
	}
	if (is_xlp_p) {
#if (MIPS64_XLP) > 0
		rmixl_com0addr =
		    (void *)(vaddr_t)(RMIXLP_SBC_PCIE_ECFG_VBASE | comcnaddr | 0x100);
#endif /* MIPS64_XLP */
	} else {
#if (MIPS64_XLR + MIPS64_XLS) > 0
		rcp->rc_io.r_pbase = RMIXL_IO_DEV_PBASE;
		rmixl_com0addr =
		    (void *)(vaddr_t)(RMIXL_IO_DEV_VBASE | comcnaddr);
#endif /* (MIPS64_XLR + MIPS64_XLS) > 0 */
	}
	cn_tab = &rmixl_earlycons;
#endif

	/*
	 * Set up the exception vectors and CPU-specific function
	 * vectors early on.  We need the wbflush() vector set up
	 * before comcnattach() is called (or at least before the
	 * first printf() after that is called).
	 * Also clears the I+D caches.
	 *
	 * specify chip-specific EIRR/EIMR based spl functions
	 */
#ifdef MULTIPROCESSOR
	mips_vector_init(&rmixl_splsw, true);
#else
	mips_vector_init(&rmixl_splsw, false);
#endif

	if (argc < 0) {
		void *bd = (void *)(intptr_t)argc;
		void *imgaddr = argv;
		void *consdev = envp;
		char *bootargs = (void *)(intptr_t)infop;
		printf("%s: u-boot: boardinfo=%p, image-addr=%p, consdev=%p, bootargs=%p <%s>\n",
		    __func__, bd, imgaddr, consdev, bootargs, bootargs);
		uboot_p = true;
		printf("%s: u-boot: console baudrate=%d\n", __func__,
		    *(int *)bd);
		if (*(int *)bd % 1200 == 0)
			comcnspeed = *(int *)bd;
	} else {
		DPRINTF("%s: argc=%d, argv=%p, envp=%p, info=%#"PRIx64"\n",
		    __func__, argc, argv, envp, infop);
	}

	/* mips_vector_init initialized mips_options */
	strcpy(cpu_model, mips_options.mips_cpu->cpu_name);

	if (is_xlp_p) {
#if (MIPS64_XLP) > 0
		uint32_t cfg6 = mipsNN_cp0_config6_read();
		printf("%s: cfg6=%#x "
		    "<ctlb=%u,vtlb=%u,elvt=%u,epw=%u,eft=%u,pwi=%u,fti=%u>\n",
		    __func__, cfg6,
		    MIPSNN_GET(RMIXLP_CFG6_CTLB_SIZE, cfg6),
		    MIPSNN_GET(RMIXLP_CFG6_VTLB_SIZE, cfg6),
		    __SHIFTOUT(cfg6, MIPSNN_RMIXLP_CFG6_ELVT),
		    __SHIFTOUT(cfg6, MIPSNN_RMIXLP_CFG6_EPW),
		    __SHIFTOUT(cfg6, MIPSNN_RMIXLP_CFG6_EFT),
		    __SHIFTOUT(cfg6, MIPSNN_RMIXLP_CFG6_PWI),
		    __SHIFTOUT(cfg6, MIPSNN_RMIXLP_CFG6_FTI));
		rcp->rc_pci_ecfg.r_pbase = RMIXLP_SBC_PCIE_ECFG_PBASE;
		rcp->rc_pci_ecfg.r_size = RMIXLP_SBC_PCIE_ECFG_SIZE(
		    RMIXLP_SBC_PCIE_ECFG_PBASE,
		    RMIXLP_SBC_PCIE_ECFG_TO_PA(
			rmixlp_read_4(RMIXLP_SBC_PCITAG,
			    RMIXLP_SBC_PCIE_ECFG_LIMIT)));

		DPRINTF("%s: ecfg pbase=%#"PRIxBUSADDR" size=%#"PRIxBUSSIZE"\n",
		    __func__, rcp->rc_pci_ecfg.r_pbase,
		    rcp->rc_pci_ecfg.r_size);

		rmixl_pci_ecfg_eb_bus_mem_init(&rcp->rc_pci_ecfg_eb_memt, rcp);
		rmixl_pci_ecfg_el_bus_mem_init(&rcp->rc_pci_ecfg_el_memt, rcp);
		rcp->rc_pci_ecfg_eb_memh = MIPS_PHYS_TO_KSEG1(rcp->rc_pci_ecfg.r_pbase);
		rcp->rc_pci_ecfg_el_memh = rcp->rc_pci_ecfg_eb_memh;
		DPRINTF("%s: pci ecfg bus space done!\n", __func__);
		rmixlp_pcie_pc_init();
		DPRINTF("%s: pci chipset init done!\n", __func__);
#if NCOM > 0
		comcnfreq = 133333333;
		com_pci_cnattach(comcnaddr, comcnspeed,
		    comcnfreq, COM_TYPE_NORMAL, comcnmode);
		DPRINTF("%s: com@pci console attached!\n", __func__);
#endif
#endif /* MIPS64_XLP */
	}

	/* determine DRAM first */
	memsize = rmixl_physaddr_init();
	DPRINTF("%s: physaddr init done (memsize=%"PRIu64"MB)!\n",
	    __func__, memsize >> 20);

	if (!uboot_p) {
		/* get system info from firmware */
		memsize = rmixlfw_init(infop);
		DPRINTF("%s: firmware init done (memsize=%"PRIu64"MB)!\n",
		    __func__, memsize >> 20);
	} else {
		rcp->rc_psb_info.userapp_cpu_map = 1;
	}

	/* set the VM page size */
	uvm_setpagesize();

	physmem = btoc(memsize);

	if (!is_xlp_p) {
#if (MIPS64_XLR + MIPS64_XLS) > 0
		rmixl_obio_eb_bus_mem_init(&rcp->rc_obio_eb_memt, rcp);
#if NCOM > 0
		rmixl_com_cnattach(comcnaddr, comcnspeed, comcnfreq,
		    COM_TYPE_NORMAL, comcnmode);
#endif
#endif /* (MIPS64_XLR + MIPS64_XLS) > 0 */
	}

	printf("\nNetBSD/rmixl\n");
	printf("memsize = %#"PRIx64"\n", memsize);
#ifdef MEMLIMIT
	printf("memlimit = %#"PRIx64"\n", (uint64_t)MEMLIMIT);
#endif

#if defined(MULTIPROCESSOR) && defined(MACHDEP_DEBUG)
	if (!uboot_p) {
		rmixl_wakeup_info_print(rcp->rc_cpu_wakeup_info);
		rmixl_wakeup_info_print(rcp->rc_cpu_wakeup_info + 1);
		printf("cpu_wakeup_info %p, cpu_wakeup_end %p\n",
			rcp->rc_cpu_wakeup_info,
			rcp->rc_cpu_wakeup_end);
		printf("userapp_cpu_map: %#"PRIx64"\n",
			rcp->rc_psb_info.userapp_cpu_map);
		printf("wakeup: %#"PRIx64"\n", rcp->rc_psb_info.wakeup);
	}
{
	register_t sp;
	asm volatile ("move	%0, $sp\n" : "=r"(sp));
	printf("sp: %#"PRIx64"\n", sp);
}
#endif

	/*
	 * Obtain the cpu frequency
	 * Compute the number of ticks for hz.
	 * Compute the delay divisor.
	 * Double the Hz if this CPU runs at twice the
         *  external/cp0-count frequency
	 */
	if (uboot_p) {
		/*
		 * Since u-boot doesn't tell us, we have to figure it out
		 */
		if (is_xlp_p) {
#if (MIPS64_XLP) > 0
			uint32_t por_cfg = rmixlp_read_4(RMIXLP_SM_PCITAG,
			    RMIXLP_SM_POWER_ON_RESET_CFG);
			u_int cdv = __SHIFTOUT(por_cfg, RMIXLP_SM_POWER_ON_RESET_CFG_CDV) + 1;
			u_int cdf = __SHIFTOUT(por_cfg, RMIXLP_SM_POWER_ON_RESET_CFG_CDF) + 1;
			u_int cdr = __SHIFTOUT(por_cfg, RMIXLP_SM_POWER_ON_RESET_CFG_CDR) + 1;
			u_int cpll_dfs = __SHIFTOUT(por_cfg, RMIXLP_SM_POWER_ON_RESET_CFG_CPLL_DFS) + 1;

			uint64_t freq_in = 133333333;
			uint64_t freq_out = (freq_in / cdr) * cdf / (cdv * cpll_dfs);
			if (freq_out % 1000 > 900) {
				freq_out = (freq_out + 99) / 100;
				freq_out *= 100;
			}
			rcp->rc_psb_info.cpu_frequency = freq_out;
#endif /* MIPS64_XLP > 0 */
		} else {
#if (MIPS64_XLR + MIPS64_XLS) > 0
			const uint32_t por_cfg = RMIXL_IOREG_READ(
			    RMIXL_IO_DEV_GPIO + RMIXL_GPIO_RESET_CFG);

			const u_int divq = __SHIFTOUT(por_cfg,
			    RMIXL_GPIO_RESET_CFG_PLL1_OUT_DIV);
			const u_int divf = __SHIFTOUT(por_cfg,
			    RMIXL_GPIO_RESET_CFG_PLL1_FB_DIV) + 1;

			uint64_t freq_in = 66666666;
			uint64_t freq_out = (freq_in / 4) * divf / divq;

			if (freq_out % 1000 > 900) {
				freq_out = (freq_out + 99) / 100;
				freq_out *= 100;
			}
			rcp->rc_psb_info.cpu_frequency = freq_out;
#endif /* (MIPS64_XLR + MIPS64_XLS) > 0 */
		}
	}
	DPRINTF("%s: cpu_freq=%"PRIu64"\n", __func__,
	    rcp->rc_psb_info.cpu_frequency);
	curcpu()->ci_cpu_freq = rcp->rc_psb_info.cpu_frequency;
	curcpu()->ci_cctr_freq = curcpu()->ci_cpu_freq;
	curcpu()->ci_cycles_per_hz = (curcpu()->ci_cpu_freq + hz / 2) / hz;
	curcpu()->ci_divisor_delay =
		((curcpu()->ci_cpu_freq + 500000) / 1000000);
        if (mips_options.mips_cpu_flags & CPU_MIPS_DOUBLE_COUNT)
		curcpu()->ci_cpu_freq *= 2;

	/*
	 * Look at arguments passed to us and compute boothowto.
	 * - rmixl firmware gives us a 32 bit argv[i], so adapt
	 *   by forcing sign extension in cast to (char *)
	 */
	boothowto = RB_AUTOBOOT;
	// boothowto |= AB_VERBOSE;
	// boothowto |= AB_DEBUG;
	if (!uboot_p) {
		for (int i = 1; i < argc; i++) {
			for (char *cp = (char *)(intptr_t)argv[i]; *cp; cp++) {
				int howto;
				/* Ignore superfluous '-', if there is one */
				if (*cp == '-')
					continue;

				howto = 0;
				BOOT_FLAG(*cp, howto);
				if (howto != 0)
					boothowto |= howto;
#ifdef DIAGNOSTIC
				else
					printf("bootflag '%c' not recognised\n",
					     *cp);
#endif
			}
		}
	}
#ifdef DIAGNOSTIC
	printf("boothowto %#x\n", boothowto);
#endif

	/*
	 * Reserve pages from the VM system.
	 */

	/* reserve 0..start..kernend pages */
	mem_cluster_cnt = ram_seg_resv(mem_clusters, mem_cluster_cnt,
		0, round_page(MIPS_KSEG0_TO_PHYS(kernend)));

	/* reserve reset exception vector page */
	/* should never be in our clusters anyway... */
	mem_cluster_cnt = ram_seg_resv(mem_clusters, mem_cluster_cnt,
		0x1FC00000, 0x1FC00000+NBPG);

	/* Stop this abomination */
	mem_cluster_cnt = ram_seg_resv(mem_clusters, mem_cluster_cnt,
		0x18000000, 0x20000000);

#ifdef MULTIPROCESSOR
	/* reserve the cpu_wakeup_info area */
	mem_cluster_cnt = ram_seg_resv(mem_clusters, mem_cluster_cnt,
		(u_quad_t)trunc_page((vaddr_t)rcp->rc_cpu_wakeup_info),
		(u_quad_t)round_page((vaddr_t)rcp->rc_cpu_wakeup_end));
#endif

#ifdef MEMLIMIT
	/* reserve everything >= MEMLIMIT */
	mem_cluster_cnt = ram_seg_resv(mem_clusters, mem_cluster_cnt,
		(u_quad_t)MEMLIMIT, (u_quad_t)~0);
#endif

#ifdef ENABLE_MIPS_KSEGX
	/*
	 * Now we need to reserve an aligned block of memory for pre-init
	 * allocations so we don't deplete KSEG0.
	 */
	for (u_int i=0; i < mem_cluster_cnt; i++) {
		u_quad_t finish = round_page(
			mem_clusters[i].start + mem_clusters[i].size);
		u_quad_t start = roundup2(mem_clusters[i].start, VM_KSEGX_SIZE);
		if (start > MIPS_PHYS_MASK && start + VM_KSEGX_SIZE <= finish) {
			mips_ksegx_start = start;
			mips_ksegx_pte.pt_entry = mips_paddr_to_tlbpfn(start)
			    | MIPS3_PG_D | MIPS3_PG_CACHED
			    | MIPS3_PG_V | MIPS3_PG_G;
			fl[0].fl_start = start;
			fl[0].fl_end = start + VM_KSEGX_SIZE;
			fl[0].fl_freelist = VM_FREELIST_FIRST512M;
			fl_count++;
			DPRINTF("mips_ksegx_start %#"PRIxPADDR"\n",
			    fl[0].fl_start);
			break;
		}
	}
#endif

	/* get maximum RAM address from the VM clusters */
	mem_cluster_maxaddr = 0;
	for (u_int i=0; i < mem_cluster_cnt; i++) {
		u_quad_t tmp = round_page(
			mem_clusters[i].start + mem_clusters[i].size);
		if (tmp > mem_cluster_maxaddr)
			mem_cluster_maxaddr = tmp;
	}
	DPRINTF("mem_cluster_maxaddr %#"PRIx64"\n", mem_cluster_maxaddr);

	/*
	 * Load mem_clusters[] into the VM system.
	 */
	mips_page_physload(MIPS_KSEG0_START, (vaddr_t) kernend,
	    mem_clusters, mem_cluster_cnt, fl, fl_count);

	/*
	 * Initialize error message buffer (at end of core).
	 */
	mips_init_msgbuf();

	pmap_bootstrap();

	/*
	 * Allocate uarea page for lwp0 and set it.
	 */
	mips_init_lwp0_uarea();

	/*
	 * Initialize debuggers, and break into them, if appropriate.
	 */
#if NKSYMS || defined(DDB) || defined(LKM)
	ksyms_init(0, 0, 0);
#endif

#if defined(DDB)
	if (boothowto & RB_KDB)
		Debugger();
#endif
	/*
	 * store (cpu#0) curcpu in COP0 OSSCRATCH0
	 * used in exception vector
	 */
	__asm __volatile("dmtc0 %0,$%1"
		:: "r"(&cpu_info_store), "n"(MIPS_COP_0_OSSCRATCH));
#ifdef MULTIPROCESSOR
	mips_fixup_exceptions(rmixl_fixup_cop0_oscratch);
#endif
	rmixl_fixup_curcpu();
}

/*
 * set up Processor Control Regs for this core
 */
void
rmixl_pcr_init_core(bool is_xlp_p)
{
	uint32_t r;


	if (is_xlp_p) {
#if (MIPS64_XLP) > 0
#ifndef MULTIPROCESSOR
		rmixl_mtcr(RMIXLP_PCR_IFU_THREAD_EN, 1);
			/* disable all threads except #0 */
#endif
		rmixl_mtcr(RMIXLP_PCR_MMU_SETUP, 1);
			/* enable MMU clock gating */
			/* TLB is global */
#ifdef MIPS_DISABLE_L1_CACHE
		r = rmixl_mfcr(RMIXLP_PCR_L1D_CONFIG0);
		r &= ~__BIT(0);				/* disable L1D cache */
		rmixl_mtcr(RMIXLP_PCR_L1D_CONFIG0, r);
#endif
		r = rmixl_mfcr(RMIXLP_PCR_LSU_DEFEATURE);
		r &= ~RMIXLP_PCR_LSE_DEFEATURE_EUL;
		rmixl_mtcr(RMIXLP_PCR_LSU_DEFEATURE, r);

		/*
		 * Enable Large Variable TLB.
	 	 */
		uint32_t cfg6 = mipsNN_cp0_config6_read();
		cfg6 |= MIPSNN_RMIXLP_CFG6_ELVT;
		mipsNN_cp0_config6_write(cfg6);
		/*
		 * Force TLB Random to be rewritten.
		 */
		mips3_cp0_wired_write(0);
#endif /* MIPS64_XLP */
	} else {
#if (MIPS64_XLR + MIPS64_XLS) > 0
#ifdef MULTIPROCESSOR
		rmixl_mtcr(RMIXL_PCR_MMU_SETUP, __BITS(2,0));
			/* enable MMU clock gating */
			/* 4 threads active -- why needed if Global? */
			/* enable global TLB mode */
#else
		rmixl_mtcr(RMIXL_PCR_THREADEN, 1);
			/* disable all threads except #0 */
		rmixl_mtcr(RMIXL_PCR_MMU_SETUP, 0);
			/* enable MMU clock gating */
			/* set single MMU Thread Mode */
			/* TLB is partitioned (1 partition) */
#endif
		r = rmixl_mfcr(RMIXL_PCR_L1D_CONFIG0);
		r &= ~__BIT(14);		/* disable Unaligned Access */
		rmixl_mtcr(RMIXL_PCR_L1D_CONFIG0, r);
#endif /* (MIPS64_XLR + MIPS64_XLS) > 0 */
	}

#if defined(DDB) && defined(MIPS_DDB_WATCH)
	/*
	 * clear IEU_DEFEATURE[DBE]
	 * this enables COP0 watchpoint to trigger T_WATCH exception
	 * instead of signaling JTAG.
	 */
	r = rmixl_mfcr(RMIXL_PCR_IEU_DEFEATURE);
	r &= ~__BIT(7);
	rmixl_mtcr(RMIXL_PCR_IEU_DEFEATURE, r);
#endif
}

#ifdef MULTIPROCESSOR
static bool
rmixl_fixup_cop0_oscratch(int32_t load_addr, uint32_t new_insns[2])
{
	size_t offset = load_addr - (intptr_t)&cpu_info_store;

	KASSERT(MIPS_KSEG0_P(load_addr));
	KASSERT(offset < sizeof(struct cpu_info));

	/*
	 * Fixup this direct load cpu_info_store to actually get the current
	 * CPU's cpu_info from COP0 OSSCRATCH0 and then fix the load to be
	 * relative from the start of struct cpu_info.
	 */

	/* [0] = [d]mfc0 rX, $22 (OSScratch) */
	new_insns[0] = (020 << 26)
#ifdef _LP64
	    | (1 << 21)		/* double move */
#endif
	    | (new_insns[0] & 0x001f0000)
	    | (MIPS_COP_0_OSSCRATCH << 11) | (0 << 0);

	/* [1] = [ls][dw] rX, offset(rX) */
	new_insns[1] = (new_insns[1] & 0xffff0000) | offset;

	return true;
}
#endif /* MULTIPROCESSOR */

/*
 * The following changes all	lX	rN, L_CPU(MIPS_CURLWP) [curlwp->l_cpu]
 * to			     	[d]mfc0	rN, $22 [MIPS_COP_0_OSSCRATCH]
 *
 * the mfc0 is 3 cycles shorter than the load.
 */
#define	LOAD_CURCPU_0	((MIPS_CURLWP_REG << 21) | offsetof(lwp_t, l_cpu))
#define	MFC0_CURCPU_0	((OP_COP0 << 26) | (MIPS_COP_0_OSSCRATCH << 11))
#ifdef _LP64
#define	LOAD_CURCPU	((uint32_t)(OP_LD << 26) | LOAD_CURCPU_0)
#define	MFC0_CURCPU	((uint32_t)(OP_DMF << 21) | MFC0_CURCPU_0)
#else
#define	LOAD_CURCPU	((uint32_t)(OP_LW << 26) | LOAD_CURCPU_0)
#define	MFC0_CURCPU	((uint32_t)(OP_MF << 21) | MFC0_CURCPU_0)
#endif
#define	LOAD_CURCPU_MASK	0xffe0ffff

static void
rmixl_fixup_curcpu(void)
{
	extern uint32_t _ftext[];
	extern uint32_t _etext[];

	for (uint32_t *insnp = _ftext; insnp < _etext; insnp++) {
		const uint32_t insn = *insnp;
		if (__predict_false((insn & LOAD_CURCPU_MASK) == LOAD_CURCPU)) {
			/*
			 * Since the register to loaded is located in bits
			 * 16-20 for the mfc0 and the load instruction we can
			 * just change the instruction bits around it.
			 */
			*insnp = insn ^ LOAD_CURCPU ^ MFC0_CURCPU;
			mips_icache_sync_range((vaddr_t)insnp, 4);
		}
	}
}

/*
 * ram_seg_resv - cut reserved regions out of segs, fragmenting as needed
 *
 * we simply build a new table of segs, then copy it back over the given one
 * this is inefficient but simple and called only a few times
 *
 * note: 'last' here means 1st addr past the end of the segment (start+size)
 */
static u_int
ram_seg_resv(phys_ram_seg_t *segs, u_int nsegs,
	u_quad_t resv_first, u_quad_t resv_last)
{
        u_quad_t first, last;
	int new_nsegs=0;
	int resv_flag;
	phys_ram_seg_t new_segs[VM_PHYSSEG_MAX];

	for (u_int i=0; i < nsegs; i++) {
		resv_flag = 0;
		first = trunc_page(segs[i].start);
		last = round_page(segs[i].start + segs[i].size);

		KASSERT(new_nsegs < VM_PHYSSEG_MAX);
		if ((resv_first <= first) && (resv_last >= last)) {
			/* whole segment is resverved */
			continue;
		}
		if ((resv_first > first) && (resv_first < last)) {
			u_quad_t new_last;

			/*
			 * reserved start in segment
			 * salvage the leading fragment
			 */
			resv_flag = 1;
			new_last = last - (last - resv_first);
			KASSERT (new_last > first);
			new_segs[new_nsegs].start = first;
			new_segs[new_nsegs].size = new_last - first;
			new_nsegs++;
		}
		if ((resv_last > first) && (resv_last < last)) {
			u_quad_t new_first;

			/*
			 * reserved end in segment
			 * salvage the trailing fragment
			 */
			resv_flag = 1;
			new_first = first + (resv_last - first);
			KASSERT (last > (new_first + NBPG));
			new_segs[new_nsegs].start = new_first;
			new_segs[new_nsegs].size = last - new_first;
			new_nsegs++;
		}
		if (resv_flag == 0) {
			/*
			 * nothing reserved here, take it all
			 */
			new_segs[new_nsegs].start = first;
			new_segs[new_nsegs].size = last - first;
			new_nsegs++;
		}

	}

	memcpy(segs, new_segs, sizeof(new_segs));

	return new_nsegs;
}

#if (MIPS64_XLP) > 0
static void
rmixlp_physaddr_pcie_cfg_init(struct extent *ext)
{
	struct rmixl_config * const rcp = &rmixl_configuration;

	uint64_t xbase = RMIXLP_SBC_PCIE_CFG_TO_PA(
	    rmixlp_read_4(RMIXLP_SBC_PCITAG, RMIXLP_SBC_PCIE_CFG_BASE));
	uint64_t xlimit = RMIXLP_SBC_PCIE_CFG_TO_PA(
	    rmixlp_read_4(RMIXLP_SBC_PCITAG, RMIXLP_SBC_PCIE_CFG_LIMIT));

	if (xlimit < xbase || xbase == 0)
		return;	/* not enabled */

	uint64_t xsize = RMIXLP_SBC_PCIE_CFG_SIZE(xbase, xlimit);

	DPRINTF("%s: %s: %#"PRIx64":%"PRIu64" MB\n", __func__,
	    "pci-cfg", xbase, xsize >> 20);

	rmixl_physaddr_add(ext, "pcicfg", &rcp->rc_pci_cfg, xbase, xsize);
}

static void
rmixlp_physaddr_pcie_ecfg_init(struct extent *ext)
{
	struct rmixl_config * const rcp = &rmixl_configuration;

	uint64_t xbase = RMIXLP_SBC_PCIE_ECFG_TO_PA(
	    rmixlp_read_4(RMIXLP_SBC_PCITAG, RMIXLP_SBC_PCIE_ECFG_BASE));
	uint64_t xlimit = RMIXLP_SBC_PCIE_ECFG_TO_PA(
	    rmixlp_read_4(RMIXLP_SBC_PCITAG, RMIXLP_SBC_PCIE_ECFG_LIMIT));

	if (xlimit < xbase || xbase == 0)
		return;	/* not enabled */

	uint64_t xsize = RMIXLP_SBC_PCIE_ECFG_SIZE(xbase, xlimit);

	KASSERT(rcp->rc_pci_ecfg.r_pbase == xbase);

	DPRINTF("%s: %s: %#"PRIx64":%"PRIu64" MB\n", __func__,
	    "pci-ecfg", xbase, xsize >> 20);

	rmixl_physaddr_add(ext, "pciecfg", &rcp->rc_pci_ecfg, xbase, xsize);
}

static void
rmixlp_physaddr_pcie_mem_init(struct extent *ext)
{
	struct rmixl_config * const rcp = &rmixl_configuration;
	for (size_t i = 0; i < RMIXLP_SBC_NPCIE_MEM; i++) {
		uint64_t xbase = RMIXLP_SBC_PCIE_MEM_TO_PA(
		    rmixlp_read_4(RMIXLP_SBC_PCITAG,
			RMIXLP_SBC_PCIE_MEM_BASEn(i)));
		uint64_t xlimit = RMIXLP_SBC_PCIE_MEM_TO_PA(
		    rmixlp_read_4(RMIXLP_SBC_PCITAG,
			RMIXLP_SBC_PCIE_MEM_LIMITn(i)));

		if (xlimit < xbase || xbase == 0)
			continue;	/* not enabled */

		uint64_t xsize = RMIXLP_SBC_PCIE_MEM_SIZE(xbase, xlimit);

		DPRINTF("%s: %s %zu: %#"PRIx64":%"PRIu64" MB\n", __func__,
		    "pci-mem", i, xbase, xsize >> 20);

		rmixl_physaddr_add(ext, "pcimem", &rcp->rc_pci_link_mem[i],
		    xbase, xsize);
	}
}

static void
rmixlp_physaddr_pcie_io_init(struct extent *ext)
{
	struct rmixl_config * const rcp = &rmixl_configuration;
	for (size_t i = 0; i < RMIXLP_SBC_NPCIE_IO; i++) {
		uint64_t xbase = RMIXLP_SBC_PCIE_IO_TO_PA(
		    rmixlp_read_4(RMIXLP_SBC_PCITAG,
			RMIXLP_SBC_PCIE_IO_BASEn(i)));
		uint64_t xlimit = RMIXLP_SBC_PCIE_IO_TO_PA(
		    rmixlp_read_4(RMIXLP_SBC_PCITAG,
			RMIXLP_SBC_PCIE_IO_LIMITn(i)));

		if (xlimit < xbase || xbase == 0)
			continue;	/* not enabled */

		uint64_t xsize = RMIXLP_SBC_PCIE_IO_SIZE(xbase, xlimit);

		DPRINTF("%s: %s %zu: %#"PRIx64":%"PRIu64" MB\n", __func__,
		    "pci-io", i, xbase, xsize >> 20);

		rmixl_physaddr_add(ext, "pci-io", &rcp->rc_pci_link_io[i],
		    xbase, xsize);
	}
}

static void
rmixlp_physaddr_srio_mem_init(struct extent *ext)
{
	struct rmixl_config * const rcp = &rmixl_configuration;
	uint64_t xbase = RMIXLP_SBC_SRIO_MEM_TO_PA(
	    rmixlp_read_4(RMIXLP_SBC_PCITAG, RMIXLP_SBC_SRIO_MEM_BASE));
	uint64_t xlimit = RMIXLP_SBC_SRIO_MEM_TO_PA(
	    rmixlp_read_4(RMIXLP_SBC_PCITAG, RMIXLP_SBC_SRIO_MEM_LIMIT));

	if (xlimit < xbase || xbase == 0)
	    return;	/* not enabled */

	uint64_t xsize = RMIXLP_SBC_SRIO_MEM_SIZE(xbase, xlimit);

	DPRINTF("%s: %s: %#"PRIx64":%"PRIu64" MB\n", __func__,
	    "srio-mem", xbase, xsize >> 20);

	rmixl_physaddr_add(ext, "sriomem", &rcp->rc_srio_mem, xbase, xsize);
}

static void
rmixlp_physaddr_nor_init(struct extent *ext)
{
	struct rmixl_config * const rcp = &rmixl_configuration;
	for (size_t i = 0; i < RMIXLP_NOR_NCS; i++) {
		uint64_t xbase = RMIXLP_NOR_CS_ADDRESS_TO_PA(
		    rmixlp_read_4(RMIXLP_NOR_PCITAG,
			RMIXLP_NOR_CS_BASEADDRESSn(i)));
		uint64_t xlimit = RMIXLP_NOR_CS_ADDRESS_TO_PA(
		    rmixlp_read_4(RMIXLP_NOR_PCITAG,
			RMIXLP_NOR_CS_BASELIMITn(i)));

		if (xlimit < xbase || xbase == 0)
			continue;	/* not enabled */

		uint64_t xsize = RMIXLP_NOR_CS_SIZE(xbase, xlimit);

		DPRINTF("%s: %s %zu: %#"PRIx64":%"PRIu64" MB\n", __func__,
		    "nor", i, xbase, xsize >> 20);

		rmixl_physaddr_add(ext, "nor", &rcp->rc_norflash[i],
		    xbase, xsize);
	}
}

static uint64_t
rmixlp_physaddr_dram_init(struct extent *ext)
{
	uint64_t memsize = 0;
	/*
	 * grab regions per DRAM BARs
	 */
	phys_ram_seg_t *mp = mem_clusters;
	for (u_int i = 0; i < RMIXLP_SBC_NDRAM; i++) {
		uint64_t xbase =
		    RMIXLP_SBC_DRAM_TO_PA(
			rmixlp_read_4(RMIXLP_SBC_PCITAG,
			    RMIXLP_SBC_DRAM_BASEn(i)));
		uint64_t xlimit =
		    RMIXLP_SBC_DRAM_TO_PA(
			rmixlp_read_4(RMIXLP_SBC_PCITAG,
			    RMIXLP_SBC_DRAM_LIMITn(i)));

		if (xlimit < xbase)
			continue;	/* not enabled */

		mp->start = xbase;
		mp->size = RMIXLP_SBC_DRAM_SIZE(xbase, xlimit);

		memsize += mp->size;

		u_long base = mp->start >> 20;
		u_long size = mp->size >> 20;

		mp++;

		DPRINTF("%s: dram %u: 0x%05lx00000:%lu MB\n",
			__func__, i, base, size);
		if (extent_alloc_region(ext, base, size, EX_NOWAIT) != 0)
			panic("%s: extent_alloc_region(%p, %#lx, %#lx, %#x) "
				"failed", __func__, ext, base, size, EX_NOWAIT);
	}

	mem_cluster_cnt = mp - mem_clusters;
	return memsize;
}
#endif /* MIPS64_XLP */

#if (MIPS64_XLR + MIPS64_XLS) > 0
static uint64_t
rmixl_physaddr_dram_init(struct extent *ext)
{
	uint64_t memsize = 0;
	/*
	 * grab regions per DRAM BARs
	 */
	phys_ram_seg_t *mp = mem_clusters;
	for (u_int i=0; i < RMIXL_SBC_DRAM_NBARS; i++) {
		uint32_t r = RMIXL_IOREG_READ(RMIXL_SBC_DRAM_BAR(i));
		if ((r & RMIXL_DRAM_BAR_STATUS) == 0)
			continue;	/* not enabled */

		mp->start = DRAM_BAR_TO_BASE((uint64_t)r);
		mp->size  = DRAM_BAR_TO_SIZE((uint64_t)r);

		u_long base = mp->start >> 20;
		u_long size = mp->size >> 20;

		memsize += mp->size;

		mp++;

		DPRINTF("%s: dram %u: 0x%08x -- 0x%010lx:%lu MB\n",
			__func__, i, r, base * (1024 * 1024), size);
		if (extent_alloc_region(ext, base, size, EX_NOWAIT) != 0)
			panic("%s: extent_alloc_region(%p, %#lx, %#lx, %#x) "
				"failed", __func__, ext, base, size, EX_NOWAIT);
	}

	mem_cluster_cnt = mp - mem_clusters;

	return memsize;
}
#endif /* (MIPS64_XLR + MIPS64_XLS) > 0 */

/*
 * create an extent for physical address space
 * these are in units of MB for sake of compression (for sake of 32 bit kernels)
 * allocate the regions where we have known functions (DRAM, IO, etc)
 * what remains can be allocated as needed for other stuff
 * e.g. to configure BARs that are not already initialized and enabled.
 */
static uint64_t
rmixl_physaddr_init(void)
{
	struct extent *ext;
	unsigned long start = 0UL;
	unsigned long end = (__BIT(40) / (1024 * 1024)) - 1;
	const bool is_xlp_p = cpu_rmixlp(mips_options.mips_cpu);
	uint64_t memsize;

	ext = extent_create("physaddr", start, end, M_DEVBUF,
		(void *)rmixl_physaddr_storage, sizeof(rmixl_physaddr_storage),
		EX_NOWAIT | EX_NOCOALESCE);

	if (ext == NULL)
		panic("%s: extent_create failed", __func__);

	if (is_xlp_p) {
#if (MIPS64_XLP) > 0
		memsize = rmixlp_physaddr_dram_init(ext);
		rmixlp_physaddr_pcie_cfg_init(ext);
		rmixlp_physaddr_pcie_ecfg_init(ext);
		rmixlp_physaddr_pcie_mem_init(ext);
		rmixlp_physaddr_pcie_io_init(ext);
		rmixlp_physaddr_srio_mem_init(ext);
		rmixlp_physaddr_nor_init(ext);
#else
		memsize = 0;
#endif /* MIPS64_XLP */
	} else {
#if (MIPS64_XLR + MIPS64_XLS) > 0
		memsize = rmixl_physaddr_dram_init(ext);

		/*
		 * get chip-dependent physaddr regions
		 */
		switch(cpu_rmixl_chip_type(mips_options.mips_cpu)) {
		case CIDFL_RMI_TYPE_XLR:
#if NRMIXL_PCIX
			rmixl_physaddr_init_pcix(ext);
#endif
			break;
		case CIDFL_RMI_TYPE_XLS:
#if NRMIXL_PCIE
			rmixl_physaddr_init_pcie(ext);
#endif
			break;
		default:
			panic("%s: unknown chip type %d", __func__,
			    cpu_rmixl_chip_type(mips_options.mips_cpu));
		}
#else
		memsize = 0;
#endif /* (MIPS64_XLR + MIPS64_XLS) > 0 */
	}

	/*
	 *  at this point all regions left in "physaddr" extent
	 *  are unused holes in the physical adress space
	 *  available for use as needed.
	 */
	rmixl_configuration.rc_phys_ex = ext;
#ifdef MACHDEP_DEBUG
	extent_print(ext);
#endif
	return memsize;
}

static uint64_t
rmixlfw_init(int64_t infop)
{
	struct rmixl_config * const rcp = &rmixl_configuration;
	const bool is_xlp_p = cpu_rmixlp(mips_options.mips_cpu);

#ifdef MULTIPROCESSOR
	rmixl_get_wakeup_info(rcp);
#endif

	infop |= MIPS_KSEG0_START;
	rcp->rc_psb_info = *(rmixlfw_info_t *)(intptr_t)infop;

	rcp->rc_psb_type = PSB_TYPE_UNKNOWN;
	for (int i=0; i < RMICLFW_PSB_VERSIONS_LEN; i++) {
		if (rmiclfw_psb_id[i].psb_version ==
		    rcp->rc_psb_info.psb_version) {
			rcp->rc_psb_type = rmiclfw_psb_id[i].psb_type;
			goto found;
		}
	}

	if (is_xlp_p) {
#if (MIPS64_XLP) > 0
		rcp->rc_pci_ecfg.r_pbase = RMIXLP_SBC_PCIE_ECFG_PBASE;
#endif /* MIPS64_XLP */
	} else {
#if (MIPS64_XLR + MIPS64_XLS) > 0
		rcp->rc_io.r_pbase = RMIXL_IO_DEV_PBASE;
#endif /* (MIPS64_XLR + MIPS64_XLS) > 0 */
	}

#ifdef DIAGNOSTIC
	printf("\nWARNING: untested psb_version: %#"PRIx64"\n",
	    rcp->rc_psb_info.psb_version);
#endif

#ifdef MEMSIZE
	/* XXX trust and use MEMSIZE */
	mem_clusters[0].start = 0;
	mem_clusters[0].size = MEMSIZE;
	mem_cluster_cnt = 1;
	return MEMSIZE;
#else
	uint64_t memsize = 0;
	for (size_t i = 0; i < mem_cluster_cnt; i++) {
		memsize += mem_clusters[i].size;
	}
	if (memsize)
		return memsize;

	printf("\nERROR: configure MEMSIZE\n");
	cpu_reboot(RB_HALT, NULL);
	/* NOTREACHED */
#endif

 found:
	rcp->rc_io.r_pbase = MIPS_KSEG1_TO_PHYS(rcp->rc_psb_info.io_base);
	DPRINTF("\ninfop: %#"PRIx64"\n", infop);
#ifdef DIAGNOSTIC
	printf("\nrecognized psb_version=%#"PRIx64", psb_type=%s\n",
	    rcp->rc_psb_info.psb_version,
	    rmixlfw_psb_type_name(rcp->rc_psb_type));
#endif

	return mem_clusters_init(
		(rmixlfw_mmap_t *)(intptr_t)rcp->rc_psb_info.psb_physaddr_map,
		(rmixlfw_mmap_t *)(intptr_t)rcp->rc_psb_info.avail_mem_map);
}

void
rmixlfw_mmap_print(const char *mapname, rmixlfw_mmap_t *map)
{
#ifdef MACHDEP_DEBUG
	for (size_t i=0; i < map->nmmaps; i++) {
		printf("%s[%zu]: %#"PRIx64", %#"PRIx64", %#x\n",
		    mapname, i, map->entry[i].start, map->entry[i].size,
		    map->entry[i].type);
	}
#endif
}

/*
 * mem_clusters_init
 *
 * initialize mem_clusters[] table based on memory address mapping
 * provided by boot firmware.
 *
 * prefer avail_mem_map if we can, otherwise use psb_physaddr_map.
 * these will be limited by MEMSIZE if it is configured.
 * if neither are available, just use MEMSIZE.
 */
static uint64_t
mem_clusters_init(
	rmixlfw_mmap_t *psb_physaddr_map,
	rmixlfw_mmap_t *avail_mem_map)
{
	rmixlfw_mmap_t *map = NULL;
	const char *mapname;
	uint64_t sz;
	uint64_t sum;
	u_int cnt;
#ifdef MEMSIZE
	uint64_t memsize = MEMSIZE;
#endif

#ifdef MACHDEP_DEBUG
	printf("psb_physaddr_map: %p\n", psb_physaddr_map);
#endif
	if (psb_physaddr_map != NULL) {
		map = psb_physaddr_map;
		mapname = "psb_physaddr_map";
		rmixlfw_mmap_print(mapname, map);
	}
#ifdef DIAGNOSTIC
	else {
		printf("WARNING: no psb_physaddr_map\n");
	}
#endif

#ifdef MACHDEP_DEBUG
	printf("avail_mem_map: %p\n", avail_mem_map);
#endif
	if (avail_mem_map != NULL) {
		map = avail_mem_map;
		mapname = "avail_mem_map";
		rmixlfw_mmap_print(mapname, map);
	}
#ifdef DIAGNOSTIC
	else {
		printf("WARNING: no avail_mem_map\n");
	}
#endif

	if (map == NULL) {
#ifndef MEMSIZE
		printf("panic: no firmware memory map, "
			"must configure MEMSIZE\r\n");
		for(;;);	/* XXX */
#else
#ifdef DIAGNOSTIC
		printf("WARNING: no avail_mem_map, using MEMSIZE\n");
#endif

		mem_clusters[0].start = 0;
		mem_clusters[0].size = MEMSIZE;
		mem_cluster_cnt = 1;
		return MEMSIZE;
#endif	/* MEMSIZE */
	}

#ifdef DIAGNOSTIC
	printf("using %s\n", mapname);
#endif
#ifdef MACHDEP_DEBUG
	printf("memory clusters:\n");
#endif
	sum = 0;
	cnt = 0;
	for (uint32_t i=0; i < map->nmmaps; i++) {
		if (map->entry[i].type != RMIXLFW_MMAP_TYPE_RAM)
			continue;
		mem_clusters[cnt].start = map->entry[i].start;
		sz = map->entry[i].size;
		sum += sz;
		mem_clusters[cnt].size = sz;
#ifdef MACHDEP_DEBUG
		printf("[%u]: %#"PRIx64", %#"PRIx64", %#"PRIx64"\n",
		    i, mem_clusters[cnt].start, sz, sum);
#endif
#ifdef MEMSIZE
		/*
		 * configurably limit memsize
		 */
		if (sum == memsize)
			break;
		if (sum > memsize) {
			uint64_t tmp;

			tmp = sum - memsize;
			sz -= tmp;
			sum -= tmp;
			mem_clusters[cnt].size = sz;
			cnt++;
			break;
		}
#endif
		cnt++;
	}
	mem_cluster_cnt = cnt;
	return sum;
}

#ifdef MULTIPROCESSOR
/*
 * RMI firmware passes wakeup info structure in CP0 OS Scratch reg #7
 * they do not explicitly give us the size of the wakeup area.
 * we "know" that firmware loader sets wip->gp thusly:
 *   gp = stack_start[vcpu] = round_page(wakeup_end) + (vcpu * (PAGE_SIZE * 2))
 * so
 *   round_page(wakeup_end) == gp - (vcpu * (PAGE_SIZE * 2))
 * Only the "master" cpu runs this function, so
 *   vcpu = wip->master_cpu
 */
void
rmixl_get_wakeup_info(struct rmixl_config *rcp)
{
	volatile rmixlfw_cpu_wakeup_info_t *wip;
	int32_t scratch_7;
	intptr_t end;

	__asm__ volatile(
		".set push"				"\n"
		".set noreorder"			"\n"
		".set mips64"				"\n"
		"dmfc0	%0, $22, 7"			"\n"
		".set pop"				"\n"
			: "=r"(scratch_7));

	wip = (volatile rmixlfw_cpu_wakeup_info_t *)
			(intptr_t)scratch_7;
	end = wip->entry.gp - (wip->master_cpu & (PAGE_SIZE * 2));;

	if (wip->valid == 1) {
		rcp->rc_cpu_wakeup_end = (const void *)end;
		rcp->rc_cpu_wakeup_info = wip;
	}
};

#ifdef MACHDEP_DEBUG
static void
rmixl_wakeup_info_print(volatile rmixlfw_cpu_wakeup_info_t *wip)
{
	int i;

	printf("%s: wip %p, size %lu\n", __func__, wip, sizeof(*wip));

	printf("cpu_status %#x\n",  wip->cpu_status);
	printf("valid: %d\n", wip->valid);
	printf("entry: addr %#x, args %#x, sp %#"PRIx64", gp %#"PRIx64"\n",
		wip->entry.addr,
		wip->entry.args,
		wip->entry.sp,
		wip->entry.gp);
	printf("master_cpu %d\n", wip->master_cpu);
	printf("master_cpu_mask %#x\n", wip->master_cpu_mask);
	printf("buddy_cpu_mask %#x\n", wip->buddy_cpu_mask);
	printf("psb_os_cpu_map %#x\n", wip->psb_os_cpu_map);
	printf("argc %d\n", wip->argc);
	printf("argv:");
	for (i=0; i < wip->argc; i++)
		printf(" %#x", wip->argv[i]);
	printf("\n");
	printf("valid_tlb_entries %d\n", wip->valid_tlb_entries);
	printf("tlb_map:\n");
	for (i=0; i < wip->valid_tlb_entries; i++) {
		volatile const struct lib_cpu_tlb_mapping *m =
			&wip->tlb_map[i];
		printf(" %d", m->page_size);
		printf(", %d", m->asid);
		printf(", %d", m->coherency);
		printf(", %d", m->coherency);
		printf(", %d", m->attr);
		printf(", %#x", m->virt);
		printf(", %#"PRIx64"\n", m->phys);
	}
	printf("elf segs:\n");
	for (i=0; i < MAX_ELF_SEGMENTS; i++) {
		volatile const struct core_segment_info *e =
			&wip->seg_info[i];
		printf(" %#"PRIx64"", e->vaddr);
		printf(", %#"PRIx64"", e->memsz);
		printf(", %#x\n", e->flags);
	}
	printf("envc %d\n", wip->envc);
	for (i=0; i < wip->envc; i++)
		printf(" %#x \"%s\"", wip->envs[i],
			(char *)(intptr_t)(int32_t)(wip->envs[i]));
	printf("\n");
	printf("app_mode %d\n", wip->app_mode);
	printf("printk_lock %#x\n", wip->printk_lock);
	printf("kseg_master %d\n", wip->kseg_master);
	printf("kuseg_reentry_function %#x\n", wip->kuseg_reentry_function);
	printf("kuseg_reentry_args %#x\n", wip->kuseg_reentry_args);
	printf("app_shared_mem_addr %#"PRIx64"\n", wip->app_shared_mem_addr);
	printf("app_shared_mem_size %#"PRIx64"\n", wip->app_shared_mem_size);
	printf("app_shared_mem_orig %#"PRIx64"\n", wip->app_shared_mem_orig);
	printf("loader_lock %#x\n", wip->loader_lock);
	printf("global_wakeup_mask %#x\n", wip->global_wakeup_mask);
	printf("unused_0 %#x\n", wip->unused_0);
}
#endif	/* MACHDEP_DEBUG */
#endif 	/* MULTIPROCESSOR */

void
consinit(void)
{

	/*
	 * Everything related to console initialization is done
	 * in mach_init().
	 */
}

/*
 * Allocate memory for variable-sized tables,
 */
void
cpu_startup()
{
	vaddr_t minaddr, maxaddr;
	char pbuf[9];

	/*
	 * Good {morning,afternoon,evening,night}.
	 */
	printf("%s%s", copyright, version);
	format_bytes(pbuf, sizeof(pbuf), ctob((uint64_t)physmem));
	printf("total memory = %s\n", pbuf);

	/*
	 * Virtual memory is bootstrapped -- notify the bus spaces
	 * that memory allocation is now safe.
	 */
	rmixl_configuration.rc_mallocsafe = 1;

	minaddr = 0;
	/*
	 * Allocate a submap for physio.
	 */
	phys_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
				    VM_PHYS_SIZE, 0, FALSE, NULL);

	/*
	 * (No need to allocate an mbuf cluster submap.  Mbuf clusters
	 * are allocated via the pool allocator, and we use XKSEG to
	 * map those pages.)
	 */

	format_bytes(pbuf, sizeof(pbuf), ptoa(uvmexp.free));
	printf("avail memory = %s\n", pbuf);
}

int	waittime = -1;

void
cpu_reboot(int howto, char *bootstr)
{

	/* Take a snapshot before clobbering any registers. */
	savectx(lwp_getpcb(curlwp));

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

	rmixl_reset();
}

/*
 * goodbye world
 */
void __attribute__((__noreturn__))
rmixl_reset(void)
{
	uint32_t r;

	if (MIPSNN_GET(CFG_AR, mips3_cp0_config_read()) == MIPSNN_CFG_AR_REV2) {
		rmixlp_write_4(RMIXLP_SM_PCITAG, RMIXLP_SM_CHIP_RESET, 1);
		DELAY(1000000);
		printf("%s: resorting to plan b!", __func__);
		*(volatile uint32_t *)MIPS_PHYS_TO_KSEG1(0x18035100) = 1;
		__asm __volatile("sync");
	} else {
		r = RMIXL_IOREG_READ(RMIXL_IO_DEV_GPIO + RMIXL_GPIO_RESET);
		r |= RMIXL_GPIO_RESET_RESET;
		RMIXL_IOREG_WRITE(RMIXL_IO_DEV_GPIO + RMIXL_GPIO_RESET, r);
	}

	printf("soft reset failed, spinning...\n");
	for (;;);
}

#ifdef __HAVE_PCI_CONF_HOOK
int
rmixl_pci_conf_hook(void *v, int bus, int device, int function, pcireg_t id)
{
	return PCI_CONF_MAP_MEM | PCI_CONF_ENABLE_MEM | PCI_CONF_ENABLE_BM;
}
#endif
