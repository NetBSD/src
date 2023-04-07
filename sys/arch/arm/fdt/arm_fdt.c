/* $NetBSD: arm_fdt.c,v 1.21 2023/04/07 08:55:30 skrll Exp $ */

/*-
 * Copyright (c) 2017 Jared D. McNeill <jmcneill@invisible.ca>
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "opt_arm_timer.h"
#include "opt_efi.h"
#include "opt_modular.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: arm_fdt.c,v 1.21 2023/04/07 08:55:30 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/bus.h>
#include <sys/module.h>

#include <uvm/uvm_extern.h>

#include <dev/fdt/fdtvar.h>

#include <dev/ofw/openfirm.h>

#include <arm/fdt/arm_fdtvar.h>

#include <arm/locore.h>

#ifdef EFI_RUNTIME
#include <arm/arm/efi_runtime.h>
#include <dev/clock_subr.h>
#endif

static int	arm_fdt_match(device_t, cfdata_t, void *);
static void	arm_fdt_attach(device_t, device_t, void *);

static void	arm_fdt_irq_default_handler(void *);
static void	arm_fdt_fiq_default_handler(void *);

#ifdef EFI_RUNTIME
static void	arm_fdt_efi_init(device_t);
static int	arm_fdt_efi_rtc_gettime(todr_chip_handle_t, struct clock_ymdhms *);
static int	arm_fdt_efi_rtc_settime(todr_chip_handle_t, struct clock_ymdhms *);

static struct todr_chip_handle efi_todr;
#endif

CFATTACH_DECL_NEW(arm_fdt, 0,
    arm_fdt_match, arm_fdt_attach, NULL, NULL);

struct arm_fdt_cpu_hatch_cb {
	TAILQ_ENTRY(arm_fdt_cpu_hatch_cb) next;
	void (*cb)(void *, struct cpu_info *);
	void *priv;
};

static TAILQ_HEAD(, arm_fdt_cpu_hatch_cb) arm_fdt_cpu_hatch_cbs =
    TAILQ_HEAD_INITIALIZER(arm_fdt_cpu_hatch_cbs);

static void (*_arm_fdt_irq_handler)(void *) = arm_fdt_irq_default_handler;
static void (*_arm_fdt_fiq_handler)(void *) = arm_fdt_fiq_default_handler;
static void (*_arm_fdt_timer_init)(void) = NULL;

int
arm_fdt_match(device_t parent, cfdata_t cf, void *aux)
{
	return 1;
}

void
arm_fdt_attach(device_t parent, device_t self, void *aux)
{
	const struct fdt_platform *plat = fdt_platform_find();
	struct fdt_attach_args faa;

	aprint_naive("\n");
	aprint_normal("\n");

	DISABLE_INTERRUPT();

#ifdef EFI_RUNTIME
	arm_fdt_efi_init(self);
#endif

	plat->fp_init_attach_args(&faa);
	faa.faa_name = "";
	faa.faa_phandle = OF_peer(0);

	config_found(self, &faa, NULL, CFARGS_NONE);
}
void
arm_fdt_cpu_hatch_register(void *priv, void (*cb)(void *, struct cpu_info *))
{
	struct arm_fdt_cpu_hatch_cb *c;

	c = kmem_alloc(sizeof(*c), KM_SLEEP);
	c->priv = priv;
	c->cb = cb;
	TAILQ_INSERT_TAIL(&arm_fdt_cpu_hatch_cbs, c, next);
}

void
arm_fdt_cpu_hatch(struct cpu_info *ci)
{
	struct arm_fdt_cpu_hatch_cb *c;

	TAILQ_FOREACH(c, &arm_fdt_cpu_hatch_cbs, next)
		c->cb(c->priv, ci);
}

static void
arm_fdt_irq_default_handler(void *frame)
{
	panic("No IRQ handler installed");
}

static void
arm_fdt_fiq_default_handler(void *frame)
{
	panic("No FIQ handler installed");
}

void
arm_fdt_irq_set_handler(void (*irq_handler)(void *))
{
	KASSERT(_arm_fdt_irq_handler == arm_fdt_irq_default_handler);
	_arm_fdt_irq_handler = irq_handler;
}

void
arm_fdt_fiq_set_handler(void (*fiq_handler)(void *))
{
	KASSERT(_arm_fdt_fiq_handler == arm_fdt_fiq_default_handler);
	_arm_fdt_fiq_handler = fiq_handler;
}

void
arm_fdt_irq_handler(void *tf)
{
	_arm_fdt_irq_handler(tf);
}

void
arm_fdt_fiq_handler(void *tf)
{
	_arm_fdt_fiq_handler(tf);
}

void
arm_fdt_timer_register(void (*timerfn)(void))
{
	if (_arm_fdt_timer_init != NULL) {
#ifdef DIAGNOSTIC
		aprint_verbose("%s: timer already registered\n", __func__);
#endif
		return;
	}
	_arm_fdt_timer_init = timerfn;
}

#ifdef __HAVE_GENERIC_CPU_INITCLOCKS
void
cpu_initclocks(void)
{
	if (_arm_fdt_timer_init == NULL)
		panic("cpu_initclocks: no timer registered");
	_arm_fdt_timer_init();
	ENABLE_INTERRUPT();
}
#endif

void
arm_fdt_module_init(void)
{
#ifdef MODULAR
	const int chosen = OF_finddevice("/chosen");
	const char *module_name;
	const uint64_t *data;
	u_int index;
	paddr_t pa;
	vaddr_t va;
	int len;

	if (chosen == -1)
		return;

	data = fdtbus_get_prop(chosen, "netbsd,modules", &len);
	if (data == NULL)
		return;

	for (index = 0; index < len / 16; index++, data += 2) {
		module_name = fdtbus_get_string_index(chosen,
		    "netbsd,module-names", index);
		if (module_name == NULL)
			break;

		const paddr_t startpa = (paddr_t)be64dec(data + 0);
		const size_t size = (size_t)be64dec(data + 1);
		const paddr_t endpa = round_page(startpa + size);

		const vaddr_t startva = uvm_km_alloc(kernel_map, endpa - startpa,
		    0, UVM_KMF_VAONLY | UVM_KMF_NOWAIT);
		if (startva == 0) {
			printf("ERROR: Cannot allocate VA for module %s\n",
			    module_name);
			continue;
		}

		for (pa = startpa, va = startva;
		     pa < endpa;
		     pa += PAGE_SIZE, va += PAGE_SIZE) {
			pmap_kenter_pa(va, pa, VM_PROT_ALL, 0);
		}
		pmap_update(pmap_kernel());

		module_prime(module_name, (void *)(uintptr_t)startva, size);
	}
#endif /* !MODULAR */
}

#ifdef EFI_RUNTIME
static void
arm_fdt_efi_init(device_t dev)
{
	uint64_t efi_system_table;
	struct efi_tm tm;
	int error;

	const int chosen = OF_finddevice("/chosen");
	if (chosen < 0)
		return;

	if (of_getprop_uint64(chosen, "netbsd,uefi-system-table", &efi_system_table) != 0)
		return;

	error = arm_efirt_init(efi_system_table);
	if (error)
		return;

	aprint_debug_dev(dev, "EFI system table at %#" PRIx64 "\n", efi_system_table);

	if (arm_efirt_gettime(&tm, NULL) == 0) {
		aprint_normal_dev(dev, "using EFI runtime services for RTC\n");
		efi_todr.cookie = NULL;
		efi_todr.todr_gettime_ymdhms = arm_fdt_efi_rtc_gettime;
		efi_todr.todr_settime_ymdhms = arm_fdt_efi_rtc_settime;
		todr_attach(&efi_todr);
	}
}

static int
arm_fdt_efi_rtc_gettime(todr_chip_handle_t tch, struct clock_ymdhms *dt)
{
	struct efi_tm tm;
	efi_status status;

	status = arm_efirt_gettime(&tm, NULL);
	if (status != 0)
		return EIO;

	dt->dt_year = tm.tm_year;
	dt->dt_mon = tm.tm_mon;
	dt->dt_day = tm.tm_mday;
	dt->dt_wday = 0;
	dt->dt_hour = tm.tm_hour;
	dt->dt_min = tm.tm_min;
	dt->dt_sec = tm.tm_sec;

	return 0;
}

static int
arm_fdt_efi_rtc_settime(todr_chip_handle_t tch, struct clock_ymdhms *dt)
{
	struct efi_tm tm;
	efi_status status;

	memset(&tm, 0, sizeof(tm));
	tm.tm_year = dt->dt_year;
	tm.tm_mon = dt->dt_mon;
	tm.tm_mday = dt->dt_day;
	tm.tm_hour = dt->dt_hour;
	tm.tm_min = dt->dt_min;
	tm.tm_sec = dt->dt_sec;

	status = arm_efirt_settime(&tm);
	if (status != 0)
		return EIO;

	return 0;
}
#endif
