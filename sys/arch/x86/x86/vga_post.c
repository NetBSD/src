/* $NetBSD: vga_post.c,v 1.17.2.1 2011/06/06 09:07:09 jruoho Exp $ */

/*-
 * Copyright (c) 2007 Joerg Sonnenberger <joerg@NetBSD.org>.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: vga_post.c,v 1.17.2.1 2011/06/06 09:07:09 jruoho Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <uvm/uvm.h>
#include <uvm/uvm_page.h>

#include <machine/pio.h>

#include <x86/vga_post.h>

#include <x86emu/x86emu.h>
#include <x86emu/x86emu_i8254.h>
#include <x86emu/x86emu_regs.h>

#include "opt_ddb.h"

#define	BASE_MEMORY	65536	/* How much memory to allocate in Real Mode */

struct vga_post {
	struct X86EMU emu;
	vaddr_t sys_image;
	uint32_t initial_eax;
	struct x86emu_i8254 i8254;
	uint8_t bios_data[PAGE_SIZE];
	struct pglist ram_backing;
};

#ifdef DDB
static struct vga_post *ddb_vgapostp;
void ddb_vgapost(void);
#endif

static uint8_t
vm86_emu_inb(struct X86EMU *emu, uint16_t port)
{
	struct vga_post *sc = emu->sys_private;

	if (port == 0xb2) /* APM scratch register */
		return 0;

	if (port >= 0x80 && port < 0x88) /* POST status register */
		return 0;

	if (x86emu_i8254_claim_port(&sc->i8254, port) && 0) {
		return x86emu_i8254_inb(&sc->i8254, port);
	} else
		return inb(port);
}

static uint16_t
vm86_emu_inw(struct X86EMU *emu, uint16_t port)
{
	if (port >= 0x80 && port < 0x88) /* POST status register */
		return 0;

	return inw(port);
}

static uint32_t
vm86_emu_inl(struct X86EMU *emu, uint16_t port)
{
	if (port >= 0x80 && port < 0x88) /* POST status register */
		return 0;

	return inl(port);
}

static void
vm86_emu_outb(struct X86EMU *emu, uint16_t port, uint8_t val)
{
	struct vga_post *sc = emu->sys_private;

	if (port == 0xb2) /* APM scratch register */
		return;

	if (port >= 0x80 && port < 0x88) /* POST status register */
		return;

	if (x86emu_i8254_claim_port(&sc->i8254, port) && 0) {
		x86emu_i8254_outb(&sc->i8254, port, val);
	} else
		outb(port, val);
}

static void
vm86_emu_outw(struct X86EMU *emu, uint16_t port, uint16_t val)
{
	if (port >= 0x80 && port < 0x88) /* POST status register */
		return;

	outw(port, val);
}

static void
vm86_emu_outl(struct X86EMU *emu, uint16_t port, uint32_t val)
{
	if (port >= 0x80 && port < 0x88) /* POST status register */
		return;

	outl(port, val);
}

struct vga_post *
vga_post_init(int bus, int device, int function)
{
	struct vga_post *sc;
	vaddr_t iter;
	struct vm_page *pg;
	vaddr_t sys_image, sys_bios_data;
	int err;

	sys_image = uvm_km_alloc(kernel_map, 1024 * 1024, 0, UVM_KMF_VAONLY);
	if (sys_image == 0) {
		return NULL;
	}
	sc = kmem_alloc(sizeof(*sc), KM_SLEEP);
	sc->sys_image = sys_image;
	sc->emu.sys_private = sc;

	err = uvm_pglistalloc(BASE_MEMORY, 0, (paddr_t)-1, 0, 0,
	    &sc->ram_backing, BASE_MEMORY/PAGE_SIZE, 1);
	if (err) {
		uvm_km_free(kernel_map, sc->sys_image,
		    1024 * 1024, UVM_KMF_VAONLY);
		kmem_free(sc, sizeof(*sc));
		return NULL;
	}

	/*
	 * Map and copy BIOS data.
	 */
	sys_bios_data = uvm_km_alloc(kernel_map, PAGE_SIZE, 0, UVM_KMF_VAONLY);
	if (sys_bios_data == 0) {
		return NULL;
	}
	pmap_kenter_pa(sys_bios_data, 0, VM_PROT_READ, 0);
	pmap_update(pmap_kernel());

	memcpy((void *)sc->bios_data, (void *)sys_bios_data, PAGE_SIZE);

	pmap_kremove(sys_bios_data, PAGE_SIZE);
	pmap_update(pmap_kernel());
	uvm_km_free(kernel_map, sys_bios_data, PAGE_SIZE, UVM_KMF_VAONLY);

	/*
	 * Map 0 .. 64KB and 640KB .. 1MB ranges.
	 */
	iter = 0;
	TAILQ_FOREACH(pg, &sc->ram_backing, pageq.queue) {
		pmap_kenter_pa(sc->sys_image + iter, VM_PAGE_TO_PHYS(pg),
		    VM_PROT_READ | VM_PROT_WRITE, 0);
		iter += PAGE_SIZE;
	}
	KASSERT(iter == BASE_MEMORY);

	for (iter = 640 * 1024; iter < 1024 * 1024; iter += PAGE_SIZE) {
		pmap_kenter_pa(sc->sys_image + iter, iter,
		    VM_PROT_READ | VM_PROT_WRITE, 0);
	}
	pmap_update(pmap_kernel());

	memset(&sc->emu, 0, sizeof(sc->emu));
	X86EMU_init_default(&sc->emu);
	sc->emu.emu_inb = vm86_emu_inb;
	sc->emu.emu_inw = vm86_emu_inw;
	sc->emu.emu_inl = vm86_emu_inl;
	sc->emu.emu_outb = vm86_emu_outb;
	sc->emu.emu_outw = vm86_emu_outw;
	sc->emu.emu_outl = vm86_emu_outl;

	sc->emu.mem_base = (char *)sc->sys_image;
	sc->emu.mem_size = 1024 * 1024;

	sc->initial_eax = bus * 256 + device * 8 + function;
#ifdef DDB
	ddb_vgapostp = sc;
#endif
	return sc;
}

void
vga_post_call(struct vga_post *sc)
{
	sc->emu.x86.R_EAX = sc->initial_eax;
	sc->emu.x86.R_EDX = 0x00000080;
	sc->emu.x86.R_DS = 0x0040;
	sc->emu.x86.register_flags = 0x3200;

	memcpy((void *)sc->sys_image, sc->bios_data, PAGE_SIZE);

	/* stack is at the end of the first 64KB */
	sc->emu.x86.R_SS = 0;
	sc->emu.x86.R_ESP = 0;

	x86emu_i8254_init(&sc->i8254, nanotime);

	/* Jump straight into the VGA BIOS POST code */
	X86EMU_exec_call(&sc->emu, 0xc000, 0x0003);
}

void
vga_post_set_vbe(struct vga_post *sc, uint16_t vbemode)
{
	sc->emu.x86.R_EAX = sc->initial_eax;
	sc->emu.x86.R_EDX = 0x00000080;
	sc->emu.x86.R_DS = 0x0040;
	sc->emu.x86.register_flags = 0x3200;

	memcpy((void *)sc->sys_image, sc->bios_data, PAGE_SIZE);

	/* stack is at the end of the first 64KB */
	sc->emu.x86.R_SS = 0;
	sc->emu.x86.R_ESP = 0;

	x86emu_i8254_init(&sc->i8254, nanotime);

	sc->emu.x86.R_EBX = vbemode | 0x4000;
	sc->emu.x86.R_EAX = 0x4f02;
	X86EMU_exec_intr(&sc->emu, 0x10);
}

void
vga_post_free(struct vga_post *sc)
{

	uvm_pglistfree(&sc->ram_backing);
	pmap_kremove(sc->sys_image, 1024 * 1024);
	pmap_update(pmap_kernel());
	uvm_km_free(kernel_map, sc->sys_image, 1024 * 1024, UVM_KMF_VAONLY);
	kmem_free(sc, sizeof(*sc));
}

#ifdef DDB
void
ddb_vgapost(void)
{

	if (ddb_vgapostp)
		vga_post_call(ddb_vgapostp);
	else
		printf("ddb_vgapost: vga_post not initialized\n");
}
#endif
