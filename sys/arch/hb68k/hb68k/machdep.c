/*	$NetBSD: machdep.c,v 1.1 2026/07/19 01:48:21 thorpej Exp $	*/

/*-
 * Copyright (c) 2026 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
 * from: Utah $Hdr: machdep.c 1.74 92/12/20$
 *
 *	@(#)machdep.c	8.10 (Berkeley) 4/20/94
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: machdep.c,v 1.1 2026/07/19 01:48:21 thorpej Exp $");

#include "opt_ddb.h"
#include "opt_modular.h"
#include "opt_m68k_arch.h"
#include "opt_hb68k_config.h"

#include "ksyms.h"

#define	_M68K_BUS_SPACE_PRIVATE
#define	_M68K_BUS_DMA_PRIVATE

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/reboot.h>
#include <sys/conf.h>
#include <sys/ksyms.h>
#include <sys/module.h>
#include <sys/device.h>
#include <sys/cpu.h>
#include <sys/boot_flag.h>
#include <sys/bus.h>

#include <dev/fdt/fdtvar.h>
#include <dev/fdt/fdt_boot.h>
#include <dev/fdt/fdt_console.h>
#include <dev/fdt/fdt_platform.h>

#include <dev/ofw/openfirm.h>

#include <uvm/uvm_extern.h>

#include <sys/sysctl.h>

#include <dev/cons.h>

#if defined(CONFIG_LINUX_BOOTINFO)
#include <m68k/linux_bootinfo.h>
#endif

#ifdef DDB
#include <machine/db_machdep.h>
#include <ddb/db_extern.h>
#include <ddb/db_output.h>
#endif

/* Default to the fastest CPU we could have in this configuration. */
#if defined(M68060)
#define	DELAY_DIVISOR_DEFAULT	delay_divisor_est60(80)
#elif defined(M68040)
#define	DELAY_DIVISOR_DEFAULT	delay_divisor_est40(33)
#elif defined(M68030)
#define	DELAY_DIVISOR_DEFAULT	delay_divisor_est(50)
#elif defined(M68020)
#define	DELAY_DIVISOR_DEFAULT	delay_divisor_est(25)
#else
#define	DELAY_DIVISOR_DEFAULT	delay_divisor_est(12)
#endif

int	delay_divisor = DELAY_DIVISOR_DEFAULT;

/* Machine-dependent initialization routines. */
void	machine_init(paddr_t);

void
machine_fdt_init(void *addr)
{
	static bool fdt_initialized;

	if (fdt_initialized) {
		return;
	}

	fdtbus_init(addr);
	fdt_initialized = true;
}

const struct fdt_platform *
machine_platform(void)
{
	static const struct fdt_platform *this_plat;

	if (__predict_false(this_plat == NULL)) {
		this_plat = fdt_platform_find();
	}
	return this_plat;
}

static struct m68k_bus_dma_tag hb68k_simple_dma_tag = {
	NULL,
	0,
	_bus_dmamap_create,
	_bus_dmamap_destroy,
	_bus_dmamap_load_direct,
	_bus_dmamap_load_mbuf_direct,
	_bus_dmamap_load_uio_direct,
	_bus_dmamap_load_raw_direct,
	_bus_dmamap_unload,
	_bus_dmamap_sync,
	_bus_dmamem_alloc,
	_bus_dmamem_free,
	_bus_dmamem_map,
	_bus_dmamem_unmap,
	_bus_dmamem_mmap
};

bus_dma_tag_t
fdtbus_dma_tag_create(int phandle, const struct fdt_dma_range *ranges,
    u_int nranges)
{
	KASSERT(nranges == 0);
	return &hb68k_simple_dma_tag;
}

bus_space_tag_t
fdtbus_bus_tag_create(int phandle, uint32_t flags)
{
	return &m68k_simple_bus_space;
}

void
machine_init_attach_args(struct fdt_attach_args *faa)
{
	memset(faa, 0, sizeof(*faa));

	faa->faa_bst = &m68k_simple_bus_space;
	faa->faa_dmat = &hb68k_simple_dma_tag;
}

#if NKSYMS || defined(DDB) || defined(MODULAR)
vaddr_t ksym_start, ksym_end;

static bool
machine_find_ksyms(void)
{
	/*
	 * If they've already been found, then no more work to do!
	 */
	if (ksym_end - ksym_start != 0) {
		return true;
	}

#if defined(CONFIG_LINUX_BOOTINFO)
	struct bootinfo_data *bid = bootinfo_data();
	if (bid->bootinfo_ksym_size != 0) {
		ksym_start = bid->bootinfo_ksym_start;
		ksym_end = ksym_start + bid->bootinfo_ksym_size;
		return true;
	}
#endif

	return false;
}
#endif /* NKSYMS || DDB || MODULAR */

static const char *
machine_parse_bootflags(const char *cp)
{
	while (*cp != '\0') {
		if (*cp == ' ' || *cp == '\t') {
			break;
		}
		BOOT_FLAG(*cp, boothowto);
		cp++;
	}
	return cp;
}

static void
machine_parse_cmdline(void)
{
	/*
	 * We are pretty flexible with what we accept.  Here, we're
	 * just interested in boot flags, so we can do:
	 *
	 *	flags=...
	 * -or-
	 *	-...
	 *
	 * Or any combination of that, really.
	 *
	 * We also notice root= and pass that to machine_set_bootspec().
	 */
	const char *cp = NULL;

#if defined(CONFIG_LINUX_BOOTINFO)
	if (cp == NULL) {
		struct bi_record *bi = bootinfo_find(BI_COMMAND_LINE);
		if (bi != NULL) {
			cp = bootinfo_dataptr(bi);
		}
	}
#endif
	if (cp == NULL) {
		cp = fdt_get_bootargs();
	}

	if (cp == NULL) {
		return;
	}

	while (*cp != '\0') {
		if (*cp == '-') {
			cp = machine_parse_bootflags(cp + 1);
			continue;
		}
		if (strncmp(cp, "flags=", 6) == 0) {
			cp = machine_parse_bootflags(cp + 6);
			continue;
		}
		if (strncmp(cp, "root=", 5) == 0) {
			cp = machine_set_bootspec(cp + 5);
			continue;
		}
		cp++;
	}
}

/*
 * Early initialization, right before main is called.
 */
void
machine_init(paddr_t nextpa)
{
	int phandle;
	uint32_t val;

#if defined(CONFIG_LINUX_BOOTINFO)
	/*
	 * For the FDT machtype, the device tree is passed as a
	 * blob directly in the bootinfo.
	 */
	struct bi_record *bi =
	    bootinfo_find_machdep(BI_MACH_FDT, BI_FDT_BLOB);
	if (bi != NULL) {
		struct bi_data *d = bootinfo_dataptr(bi);
		machine_fdt_init(&d->data_bytes[0]);
	}
#endif

	const struct fdt_platform *plat = machine_platform();
	if (plat->fp_machine_init != NULL) {
		nextpa = plat->fp_machine_init(nextpa);
	}

	phandle = OF_finddevice("/cpus/cpu@0");
	if (phandle >= 0 &&
	    of_getprop_uint32(phandle, "clock-frequency", &val) != -1) {
		cpuspeed_khz = val / 1000;

		/*
		 * Set initial delay divisor; this may get properly
		 * calibrated later.
		 */
		switch (cputype) {
#ifdef M68060
		case CPU_68060:
			delay_divisor =
			    delay_divisor_est60(cpuspeed_khz / 1000);
			break;
#endif
#ifdef M68040
		case CPU_68040:
			delay_divisor =
			    delay_divisor_est40(cpuspeed_khz / 1000);
			break;
#endif
		default:
			delay_divisor =
			    delay_divisor_est(cpuspeed_khz / 1000);
			break;
		}
	}

#if defined(CONFIG_LINUX_BOOTINFO)
	/*
	 * Pass 2 at parsing bootinfo now that the MMU is enabled.
	 */
	bootinfo_startup2(nextpa);
#endif

	/*
	 * Just use the default pager_map_size for now.  We may decide
	 * to make it larger for large memory configs.
	 */

	machine_init_common(nextpa);

	machine_parse_cmdline();
}

/*
 * Console initialization: called early on from main,
 * before vm init or startup.  Do enough configuration
 * to choose and initialize a console.
 */
void
consinit(void)
{
	static bool initialized = false;
	const struct fdt_platform *plat = machine_platform();
	const struct fdt_console *cons = fdtbus_get_console();
	const int phandle = fdtbus_get_stdout_phandle();
	struct fdt_attach_args faa;
	u_int uart_freq = 0;

	if (initialized || cons == NULL) {
		return;
	}

	machine_init_attach_args(&faa);

	if (of_getprop_uint32(phandle, "clock-frequency", &uart_freq)) {
		if (plat->fp_uart_freq != NULL) {
			uart_freq = plat->fp_uart_freq();
		}
	}

	faa.faa_phandle = phandle;
	cons->consinit(&faa, uart_freq);

	initialized = true;

#if NKSYMS || defined(DDB) || defined(MODULAR)
	/*
	 * Add kernel symbols now because traditionally consinit()
	 * is the boot-into-debugger hook.
	 */
	if (machine_find_ksyms()) {
		ksyms_addsyms_elf(ksym_end - ksym_start,
		    (void *)ksym_start, (void *)ksym_end);
	}
#endif
#ifdef DDB
	if (boothowto & RB_KDB)
		Debugger();
#endif
}

/*
 * cpu_startup: allocate memory for variable-sized tables,
 * initialize CPU, and do autoconfiguration.
 */
void
cpu_startup(void)
{

	cpu_startup_common();

#if defined(CONFIG_LINUX_BOOTINFO)
	struct bootinfo_data *bid = bootinfo_data();
	if (bid->bootinfo_mem_segments_ignored) {
		printf("WARNING: ignored %zd bytes of memory in %d segments.\n",
		    bid->bootinfo_mem_segments_ignored_bytes,
		    bid->bootinfo_mem_segments_ignored);
	}
#endif
}

void
machine_set_model(void)
{
	int phandle;
	char buf[40];

	phandle = OF_finddevice("/");
	if (phandle < 0 ||
	    OF_getprop(phandle, "model", buf, sizeof(buf)) < 0) {
		cpu_setmodel("Unknown Homebrew");
	} else {
		cpu_setmodel("%s", buf);
	}
}

/*
 * machine dependent system variables.
 */
SYSCTL_SETUP(sysctl_machdep_setup, "sysctl machdep subtree setup")
{
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "machdep", NULL,
		       NULL, 0, NULL, 0,
		       CTL_MACHDEP, CTL_EOL);

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRUCT, "console_device", NULL,
		       sysctl_consdev, 0, NULL, sizeof(dev_t),
		       CTL_MACHDEP, CPU_CONSDEV, CTL_EOL);
}

void
machine_powerdown(void)
{
	const struct fdt_platform *plat = machine_platform();

	(*plat->fp_powerdown)();
}

void
machine_halt(void)
{
	const struct fdt_platform *plat = machine_platform();

	(*plat->fp_halt)();
}

void
machine_reboot(int howto, char *bootstr)
{
	const struct fdt_platform *plat = machine_platform();

	(*plat->fp_reboot)(howto, bootstr);
}

/*
 * Level 7 interrupts are caused by e.g. the ABORT switch.
 *
 * If we have DDB, then break into DDB on ABORT.  In a production
 * environment, bumping the ABORT switch would be bad, so we enable
 * panic'ing on ABORT with the kernel option "PANICBUTTON".
 */
int
nmihand(void *arg)
{

	printf("NMI ignored.\n");

	return 1;
}
