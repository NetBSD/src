/*	$NetBSD: prekern.c,v 1.7 2017/11/26 11:01:09 maxv Exp $	*/

/*
 * Copyright (c) 2017 The NetBSD Foundation, Inc. All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Maxime Villard.
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

#include "prekern.h"

#include <machine/reg.h>
#include <machine/specialreg.h>
#include <machine/frame.h>

#define _KERNEL
#include <machine/bootinfo.h>
#undef _KERNEL

#include <machine/tss.h>
#include <machine/segments.h>

int boothowto;
struct bootinfo bootinfo;

extern paddr_t kernpa_start, kernpa_end;

static uint8_t idtstore[PAGE_SIZE];
static uint8_t faultstack[PAGE_SIZE];
static struct x86_64_tss prekern_tss;

/* GDT offsets */
#define PREKERN_GDT_NUL_OFF	(0 * 8)
#define PREKERN_GDT_CS_OFF	(1 * 8)
#define PREKERN_GDT_DS_OFF	(2 * 8)
#define PREKERN_GDT_TSS_OFF	(3 * 8)

#define IDTVEC(name) __CONCAT(X, name)
typedef void (vector)(void);
extern vector *IDTVEC(exceptions)[];

void fatal(char *msg)
{
	print("\n");
	print_ext(RED_ON_BLACK, "********** FATAL ***********\n");
	print_ext(RED_ON_BLACK, msg);
	print("\n");
	print_ext(RED_ON_BLACK, "****************************\n");

	while (1);
}

/* -------------------------------------------------------------------------- */

struct smallframe {
	uint64_t sf_trapno;
	uint64_t sf_err;
	uint64_t sf_rip;
	uint64_t sf_cs;
	uint64_t sf_rflags;
	uint64_t sf_rsp;
	uint64_t sf_ss;
};

static void setregion(struct region_descriptor *, void *, uint16_t);
static void setgate(struct gate_descriptor *, void *, int, int, int, int);
static void set_sys_segment(struct sys_segment_descriptor *, void *,
    size_t, int, int, int);
static void set_sys_gdt(int, void *, size_t, int, int, int);
static void init_tss(void);
static void init_idt(void);

void trap(struct smallframe *);

static char *trap_type[] = {
	"privileged instruction fault",		/*  0 T_PRIVINFLT */
	"breakpoint trap",			/*  1 T_BPTFLT */
	"arithmetic trap",			/*  2 T_ARITHTRAP */
	"asynchronous system trap",		/*  3 T_ASTFLT */
	"protection fault",			/*  4 T_PROTFLT */
	"trace trap",				/*  5 T_TRCTRAP */
	"page fault",				/*  6 T_PAGEFLT */
	"alignment fault",			/*  7 T_ALIGNFLT */
	"integer divide fault",			/*  8 T_DIVIDE */
	"non-maskable interrupt",		/*  9 T_NMI */
	"overflow trap",			/* 10 T_OFLOW */
	"bounds check fault",			/* 11 T_BOUND */
	"FPU not available fault",		/* 12 T_DNA */
	"double fault",				/* 13 T_DOUBLEFLT */
	"FPU operand fetch fault",		/* 14 T_FPOPFLT */
	"invalid TSS fault",			/* 15 T_TSSFLT */
	"segment not present fault",		/* 16 T_SEGNPFLT */
	"stack fault",				/* 17 T_STKFLT */
	"machine check fault",			/* 18 T_MCA */
	"SSE FP exception",			/* 19 T_XMM */
	"reserved trap",			/* 20 T_RESERVED */
};
static int trap_types = __arraycount(trap_type);

/*
 * Trap handler.
 */
void
trap(struct smallframe *sf)
{
	uint64_t trapno = sf->sf_trapno;
	char *buf;

	if (trapno < trap_types) {
		buf = trap_type[trapno];
	} else {
		buf = "unknown trap";
	}

	print("\n");
	print_ext(RED_ON_BLACK, "****** FAULT OCCURRED ******\n");
	print_ext(RED_ON_BLACK, buf);
	print("\n");
	print_ext(RED_ON_BLACK, "****************************\n");

	while (1);
}

static void
setregion(struct region_descriptor *rd, void *base, uint16_t limit)
{
	rd->rd_limit = limit;
	rd->rd_base = (uint64_t)base;
}

static void
setgate(struct gate_descriptor *gd, void *func, int ist, int type, int dpl,
    int sel)
{
	gd->gd_looffset = (uint64_t)func & 0xffff;
	gd->gd_selector = sel;
	gd->gd_ist = ist;
	gd->gd_type = type;
	gd->gd_dpl = dpl;
	gd->gd_p = 1;
	gd->gd_hioffset = (uint64_t)func >> 16;
	gd->gd_zero = 0;
	gd->gd_xx1 = 0;
	gd->gd_xx2 = 0;
	gd->gd_xx3 = 0;
}

static void
set_sys_segment(struct sys_segment_descriptor *sd, void *base, size_t limit,
    int type, int dpl, int gran)
{
	memset(sd, 0, sizeof(*sd));
	sd->sd_lolimit = (unsigned)limit;
	sd->sd_lobase = (uint64_t)base;
	sd->sd_type = type;
	sd->sd_dpl = dpl;
	sd->sd_p = 1;
	sd->sd_hilimit = (unsigned)limit >> 16;
	sd->sd_gran = gran;
	sd->sd_hibase = (uint64_t)base >> 24;
}

static void
set_sys_gdt(int slotoff, void *base, size_t limit, int type, int dpl, int gran)
{
	struct sys_segment_descriptor sd;
	extern uint64_t *gdt64_start;

	set_sys_segment(&sd, base, limit, type, dpl, gran);

	memcpy(&gdt64_start + slotoff, &sd, sizeof(sd));
}

static void
init_tss(void)
{
	memset(&prekern_tss, 0, sizeof(prekern_tss));
	prekern_tss.tss_ist[0] = (uintptr_t)(&faultstack[PAGE_SIZE-1]) & ~0xf;

	set_sys_gdt(PREKERN_GDT_TSS_OFF, &prekern_tss,
	    sizeof(struct x86_64_tss) - 1, SDT_SYS386TSS, SEL_KPL, 0);
}

static void
init_idt(void)
{
	struct region_descriptor region;
	struct gate_descriptor *idt;
	size_t i;

	idt = (struct gate_descriptor *)&idtstore;
	for (i = 0; i < NCPUIDT; i++) {
		setgate(&idt[i], IDTVEC(exceptions)[i], 0, SDT_SYS386IGT,
		    SEL_KPL, GSEL(GCODE_SEL, SEL_KPL));
	}

	setregion(&region, &idtstore, PAGE_SIZE - 1);
	lidt(&region);
}

/* -------------------------------------------------------------------------- */

struct prekern_args {
	int boothowto;
	void *bootinfo;
	void *bootspace;
	int esym;
	int biosextmem;
	int biosbasemem;
	int cpuid_level;
	uint32_t nox_flag;
	uint64_t PDPpaddr;
	vaddr_t atdevbase;
	vaddr_t lwp0uarea;
	paddr_t first_avail;
};

struct prekern_args pkargs;

static void
init_prekern_args(void)
{
	extern struct bootspace bootspace;
	extern int esym;
	extern int biosextmem;
	extern int biosbasemem;
	extern int cpuid_level;
	extern uint32_t nox_flag;
	extern uint64_t PDPpaddr;
	extern vaddr_t iom_base;
	extern paddr_t stkpa;
	extern paddr_t pa_avail;

	memset(&pkargs, 0, sizeof(pkargs));
	pkargs.boothowto = boothowto;
	pkargs.bootinfo = (void *)&bootinfo;
	pkargs.bootspace = &bootspace;
	pkargs.esym = esym;
	pkargs.biosextmem = biosextmem;
	pkargs.biosbasemem = biosbasemem;
	pkargs.cpuid_level = cpuid_level;
	pkargs.nox_flag = nox_flag;
	pkargs.PDPpaddr = PDPpaddr;
	pkargs.atdevbase = iom_base;
	pkargs.lwp0uarea = bootspace.boot.va + (stkpa - bootspace.boot.pa);
	pkargs.first_avail = pa_avail;

	extern vaddr_t stkva;
	stkva = pkargs.lwp0uarea + (USPACE - FRAMESIZE);
}

void
exec_kernel(vaddr_t ent)
{
	int (*jumpfunc)(struct prekern_args *);
	int ret;

	/*
	 * Normally, the function does not return. If it does, it means the
	 * kernel had trouble processing the arguments, and we panic here. The
	 * return value is here for debug.
	 */
	jumpfunc = (void *)ent;
	ret = (*jumpfunc)(&pkargs);

	if (ret == -1) {
		fatal("kernel returned -1");
	} else {
		fatal("kernel returned unknown value");
	}
}

/*
 * Main entry point of the Prekern.
 */
void
init_prekern(paddr_t pa_start)
{
	vaddr_t ent;

	init_cons();
	print_banner();

	if (kernpa_start == 0 || kernpa_end == 0) {
		fatal("init_prekern: unable to locate the kernel");
	}
	if (kernpa_start != (1UL << 21)) {
		fatal("init_prekern: invalid kernpa_start");
	}
	if (kernpa_start % PAGE_SIZE != 0) {
		fatal("init_prekern: kernpa_start not aligned");
	}
	if (kernpa_end % PAGE_SIZE != 0) {
		fatal("init_prekern: kernpa_end not aligned");
	}
	if (kernpa_end <= kernpa_start) {
		fatal("init_prekern: kernpa_end >= kernpa_start");
	}

	/*
	 * Our physical space starts after the end of the kernel.
	 */
	if (pa_start < kernpa_end) {
		fatal("init_prekern: physical space inside kernel");
	}
	mm_init(pa_start);

	/*
	 * Init the TSS and IDT. We mostly don't care about this, they are just
	 * here to properly handle traps.
	 */
	init_tss();
	init_idt();

	print_state(true, "Prekern loaded");

	/*
	 * Init the PRNG.
	 */
	prng_init();

	/*
	 * Relocate the kernel.
	 */
	mm_map_kernel();
	ent = elf_kernel_reloc();
	mm_bootspace_mprotect();

	/*
	 * Build the arguments.
	 */
	init_prekern_args();

	/*
	 * Finally, jump into the kernel.
	 */
	print_state(true, "Jumping into the kernel");
	jump_kernel(ent);

	fatal("init_prekern: unreachable!");
}
