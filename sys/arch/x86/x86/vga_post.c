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

#include <sys/param.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <uvm/uvm_extern.h>
#include <uvm/uvm_page.h>

#include <machine/pio.h>

#include <x86/vga_post.h>

#include <x86emu/x86emu.h>
#include <x86emu/x86emu_i8254.h>
#include <x86emu/x86emu_regs.h>

struct vga_post {
	struct X86EMU emu;
	vaddr_t sys_image;
	uint32_t initial_eax;
	struct x86emu_i8254 i8254;
};

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
	vaddr_t rom;
	vaddr_t sys_image;

	sys_image = uvm_km_alloc(kernel_map, 1024 * 1024, 0, UVM_KMF_VAONLY);
	if (sys_image == 0)
		return NULL;
	sc = kmem_alloc(sizeof(*sc), KM_SLEEP);

	sc->sys_image = sys_image;
	sc->emu.sys_private = sc;

	pmap_kenter_pa(sc->sys_image, 0, VM_PROT_READ | VM_PROT_WRITE);
	for (rom = 640 * 1024; rom < 1024 * 1024; rom += 4096)
		pmap_kenter_pa(sc->sys_image + rom, rom, VM_PROT_READ | VM_PROT_WRITE);
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

	return sc;
}

void
vga_post_call(struct vga_post *sc)
{
	sc->emu.x86.R_EAX = sc->initial_eax;
	sc->emu.x86.R_EDX = 0x00000080;
	sc->emu.x86.R_DS = 0x0040;
	sc->emu.x86.register_flags = 0x3200;

	/* stack is at the end of the first 4KB */
	sc->emu.x86.R_SS = 0;
	sc->emu.x86.R_ESP = 4096;

	x86emu_i8254_init(&sc->i8254, nanotime);

	/* Jump straight into the VGA BIOS POST code */
	X86EMU_exec_call(&sc->emu, 0xc000, 0x0003);
}

void
vga_post_free(struct vga_post *sc)
{
	pmap_kremove(sc->sys_image, 1024 * 1024);
	pmap_update(pmap_kernel());
	kmem_free(sc, sizeof(*sc));
}
