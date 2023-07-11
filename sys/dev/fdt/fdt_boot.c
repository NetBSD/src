/*	$NetBSD: fdt_boot.c,v 1.4 2023/07/11 05:57:44 skrll Exp $	*/

/*-
 * Copyright (c) 2015-2017 Jared McNeill <jmcneill@invisible.ca>
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

/*-
 * Copyright (c) 2022 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nick Hudson
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: fdt_boot.c,v 1.4 2023/07/11 05:57:44 skrll Exp $");

#include "opt_efi.h"
#include "opt_md.h"

#include <sys/param.h>

#include <sys/disk.h>
#include <sys/disklabel.h>
#include <sys/fcntl.h>
#include <sys/md5.h>
#include <sys/optstr.h>
#include <sys/rnd.h>
#include <sys/rndsource.h>
#include <sys/uuid.h>
#include <sys/vnode.h>

#include <net/if.h>
#include <net/if_dl.h>

#include <uvm/uvm_extern.h>

#include <libfdt.h>

#include <dev/fdt/fdtvar.h>
#include <dev/fdt/fdt_boot.h>
#include <dev/fdt/fdt_memory.h>

#ifndef FDT_MAX_BOOT_STRING
#define	FDT_MAX_BOOT_STRING	1024
#endif
static char bootargs[FDT_MAX_BOOT_STRING] = "";

#ifdef EFI_RUNTIME
#include <machine/efirt.h>

void fdt_map_efi_runtime(const char *, enum cpu_efirt_mem_type);

#endif

#ifdef MEMORY_DISK_DYNAMIC
#include <dev/md.h>

static uint64_t initrd_start, initrd_end;
#endif

static uint64_t rndseed_start, rndseed_end; /* our on-disk seed */
static uint64_t efirng_start, efirng_end;   /* firmware's EFI RNG output */
static struct krndsource efirng_source;


static void
fdt_probe_range(const char *startname, const char *endname,
    uint64_t *pstart, uint64_t *pend)
{
	int chosen, len;
	const void *start_data, *end_data;

	*pstart = *pend = 0;

	chosen = OF_finddevice("/chosen");
	if (chosen < 0)
		return;

	start_data = fdtbus_get_prop(chosen, startname, &len);
	end_data = fdtbus_get_prop(chosen, endname, NULL);
	if (start_data == NULL || end_data == NULL)
		return;

	switch (len) {
	case 4:
		*pstart = be32dec(start_data);
		*pend = be32dec(end_data);
		break;
	case 8:
		*pstart = be64dec(start_data);
		*pend = be64dec(end_data);
		break;
	default:
		printf("Unsupported len %d for /chosen `%s'\n",
		    len, startname);
		return;
	}
}


static void *
fdt_map_range(uint64_t start, uint64_t end, uint64_t *psize,
    const char *purpose)
{
	const paddr_t startpa = trunc_page(start);
	const paddr_t endpa = round_page(end);
	paddr_t pa;
	vaddr_t va;
	void *ptr;

	*psize = end - start;
	if (*psize == 0)
		return NULL;

	const vaddr_t voff = start & PAGE_MASK;

	// XXX NH add an align so map_chunk works betterer?
	va = uvm_km_alloc(kernel_map, *psize, 0, UVM_KMF_VAONLY | UVM_KMF_NOWAIT);
	if (va == 0) {
		printf("Failed to allocate VA for %s\n", purpose);
		return NULL;
	}
	ptr = (void *)(va + voff);

	// XXX NH map chunk
	for (pa = startpa; pa < endpa; pa += PAGE_SIZE, va += PAGE_SIZE)
		pmap_kenter_pa(va, pa, VM_PROT_READ | VM_PROT_WRITE, 0);
	pmap_update(pmap_kernel());

	return ptr;
}

static void
fdt_unmap_range(void *ptr, uint64_t size)
{
	const char *start = ptr, *end = start + size;
	const vaddr_t startva = trunc_page((vaddr_t)(uintptr_t)start);
	const vaddr_t endva = round_page((vaddr_t)(uintptr_t)end);
	const vsize_t sz = endva - startva;

	pmap_kremove(startva, sz);
	pmap_update(pmap_kernel());

	uvm_km_free(kernel_map, startva, sz, UVM_KMF_VAONLY);
}

char *
fdt_get_bootargs(void)
{
	const int chosen = OF_finddevice("/chosen");

	if (chosen >= 0)
		OF_getprop(chosen, "bootargs", bootargs, sizeof(bootargs));
	return bootargs;
}

void
fdt_probe_initrd(void)
{

#ifdef MEMORY_DISK_DYNAMIC
	fdt_probe_range("linux,initrd-start", "linux,initrd-end",
	    &initrd_start, &initrd_end);
#endif
}

void
fdt_setup_initrd(void)
{
#ifdef MEMORY_DISK_DYNAMIC
	void *md_start;
	uint64_t initrd_size;

	md_start = fdt_map_range(initrd_start, initrd_end, &initrd_size,
	    "initrd");
	if (md_start == NULL)
		return;
	md_root_setconf(md_start, initrd_size);
#endif
}

void
fdt_reserve_initrd(void)
{
#ifdef MEMORY_DISK_DYNAMIC
	const uint64_t initrd_size =
	    round_page(initrd_end) - trunc_page(initrd_start);

	if (initrd_size > 0)
		fdt_memory_remove_range(trunc_page(initrd_start), initrd_size);
#endif
}

void
fdt_probe_rndseed(void)
{

	fdt_probe_range("netbsd,rndseed-start", "netbsd,rndseed-end",
	    &rndseed_start, &rndseed_end);
}

void
fdt_setup_rndseed(void)
{
	uint64_t rndseed_size;
	void *rndseed;

	rndseed = fdt_map_range(rndseed_start, rndseed_end, &rndseed_size,
	    "rndseed");
	if (rndseed == NULL)
		return;
	rnd_seed(rndseed, rndseed_size);
	fdt_unmap_range(rndseed, rndseed_size);
}

void
fdt_reserve_rndseed(void)
{
	const uint64_t rndseed_size =
	    round_page(rndseed_end) - trunc_page(rndseed_start);

	if (rndseed_size > 0)
		fdt_memory_remove_range(trunc_page(rndseed_start),
		    rndseed_size);
}

void
fdt_probe_efirng(void)
{

	fdt_probe_range("netbsd,efirng-start", "netbsd,efirng-end",
	    &efirng_start, &efirng_end);
}

void
fdt_setup_efirng(void)
{
	uint64_t efirng_size;
	void *efirng;

	efirng = fdt_map_range(efirng_start, efirng_end, &efirng_size,
	    "efirng");
	if (efirng == NULL)
		return;

	rnd_attach_source(&efirng_source, "efirng", RND_TYPE_RNG,
	    RND_FLAG_DEFAULT);

	/*
	 * We don't really have specific information about the physical
	 * process underlying the data provided by the firmware via the
	 * EFI RNG API, so the entropy estimate here is heuristic.
	 * What efiboot provides us is up to 4096 bytes of data from
	 * the EFI RNG API, although in principle it may return short.
	 *
	 * The UEFI Specification (2.8 Errata A, February 2020[1]) says
	 *
	 *	When a Deterministic Random Bit Generator (DRBG) is
	 *	used on the output of a (raw) entropy source, its
	 *	security level must be at least 256 bits.
	 *
	 * It's not entirely clear whether `it' refers to the DRBG or
	 * the entropy source; if it refers to the DRBG, it's not
	 * entirely clear how ANSI X9.31 3DES, one of the options for
	 * DRBG in the UEFI spec, can provide a `256-bit security
	 * level' because it has only 232 bits of inputs (three 56-bit
	 * keys and one 64-bit block).  That said, even if it provides
	 * only 232 bits of entropy, that's enough to prevent all
	 * attacks and we probably get a few more bits from sampling
	 * the clock anyway.
	 *
	 * In the event we get raw samples, e.g. the bits sampled by a
	 * ring oscillator, we hope that the samples have at least half
	 * a bit of entropy per bit of data -- and efiboot tries to
	 * draw 4096 bytes to provide plenty of slop.  Hence we divide
	 * the total number of bits by two and clamp at 256.  There are
	 * ways this could go wrong, but on most machines it should
	 * behave reasonably.
	 *
	 * [1] https://uefi.org/sites/default/files/resources/UEFI_Spec_2_8_A_Feb14.pdf
	 */
	rnd_add_data(&efirng_source, efirng, efirng_size,
	    MIN(256, efirng_size*NBBY/2));

	explicit_memset(efirng, 0, efirng_size);
	fdt_unmap_range(efirng, efirng_size);
}

void
fdt_reserve_efirng(void)
{
	const uint64_t efirng_size =
	    round_page(efirng_end) - trunc_page(efirng_start);

	if (efirng_size > 0)
		fdt_memory_remove_range(trunc_page(efirng_start), efirng_size);
}

#ifdef EFI_RUNTIME
void
fdt_map_efi_runtime(const char *prop, enum cpu_efirt_mem_type type)
{
	int len;

	const int chosen_off = fdt_path_offset(fdtbus_get_data(), "/chosen");
	if (chosen_off < 0)
		return;

	const uint64_t *map = fdt_getprop(fdtbus_get_data(), chosen_off, prop, &len);
	if (map == NULL)
		return;

	while (len >= 24) {
		const paddr_t pa = be64toh(map[0]);
		const vaddr_t va = be64toh(map[1]);
		const size_t sz = be64toh(map[2]);
#if 0
		VPRINTF("%s: %s %#" PRIxPADDR "-%#" PRIxVADDR " (%#" PRIxVADDR
		    "-%#" PRIxVSIZE ")\n", __func__, prop, pa, pa + sz - 1,
		    va, va + sz - 1);
#endif
		cpu_efirt_map_range(va, pa, sz, type);
		map += 3;
		len -= 24;
	}
}
#endif

void
fdt_update_stdout_path(void *fdt, const char *boot_args)
{
	const char *stdout_path;
	char buf[256];

	const int chosen_off = fdt_path_offset(fdt, "/chosen");
	if (chosen_off == -1)
		return;

	if (optstr_get_string(boot_args, "stdout-path", &stdout_path) == false)
		return;

	const char *ep = strchr(stdout_path, ' ');
	size_t stdout_path_len = ep ? (ep - stdout_path) : strlen(stdout_path);
	if (stdout_path_len >= sizeof(buf))
		return;

	strncpy(buf, stdout_path, stdout_path_len);
	buf[stdout_path_len] = '\0';
	fdt_setprop(fdt, chosen_off, "stdout-path",
	    buf, stdout_path_len + 1);
}
