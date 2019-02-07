/*
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
 * All rights reserved.
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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <err.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <machine/segments.h>
#include <machine/psl.h>
#include <machine/pte.h>
#include <x86/specialreg.h>

#include <nvmm.h>

#define PAGE_SIZE 4096

static uint8_t mmiobuf[PAGE_SIZE];
static uint8_t *instbuf;

static void
init_seg(struct nvmm_x64_state_seg *seg, int type, int sel)
{
	seg->selector = sel;
	seg->attrib.type = type;
	seg->attrib.dpl = 0;
	seg->attrib.p = 1;
	seg->attrib.avl = 1;
	seg->attrib.lng = 1;
	seg->attrib.def32 = 0;
	seg->attrib.gran = 1;
	seg->limit = 0x0000FFFF;
	seg->base = 0x00000000;
}

static void
reset_machine(struct nvmm_machine *mach)
{
	struct nvmm_x64_state state;

	memset(&state, 0, sizeof(state));

	/* Default. */
	state.gprs[NVMM_X64_GPR_RFLAGS] = PSL_MBO;
	init_seg(&state.segs[NVMM_X64_SEG_CS], SDT_MEMERA, GSEL(GCODE_SEL, SEL_KPL));
	init_seg(&state.segs[NVMM_X64_SEG_SS], SDT_MEMRWA, GSEL(GDATA_SEL, SEL_KPL));
	init_seg(&state.segs[NVMM_X64_SEG_DS], SDT_MEMRWA, GSEL(GDATA_SEL, SEL_KPL));
	init_seg(&state.segs[NVMM_X64_SEG_ES], SDT_MEMRWA, GSEL(GDATA_SEL, SEL_KPL));
	init_seg(&state.segs[NVMM_X64_SEG_FS], SDT_MEMRWA, GSEL(GDATA_SEL, SEL_KPL));
	init_seg(&state.segs[NVMM_X64_SEG_GS], SDT_MEMRWA, GSEL(GDATA_SEL, SEL_KPL));

	/* Blank. */
	init_seg(&state.segs[NVMM_X64_SEG_GDT], 0, 0);
	init_seg(&state.segs[NVMM_X64_SEG_IDT], 0, 0);
	init_seg(&state.segs[NVMM_X64_SEG_LDT], SDT_SYSLDT, 0);
	init_seg(&state.segs[NVMM_X64_SEG_TR], SDT_SYS386BSY, 0);

	/* Protected mode enabled. */
	state.crs[NVMM_X64_CR_CR0] = CR0_PG|CR0_PE|CR0_NE|CR0_TS|CR0_MP|CR0_WP|CR0_AM;

	/* 64bit mode enabled. */
	state.crs[NVMM_X64_CR_CR4] = CR4_PAE;
	state.msrs[NVMM_X64_MSR_EFER] = EFER_LME | EFER_SCE | EFER_LMA;

	/* Stolen from x86/pmap.c */
#define	PATENTRY(n, type)	(type << ((n) * 8))
#define	PAT_UC		0x0ULL
#define	PAT_WC		0x1ULL
#define	PAT_WT		0x4ULL
#define	PAT_WP		0x5ULL
#define	PAT_WB		0x6ULL
#define	PAT_UCMINUS	0x7ULL
	state.msrs[NVMM_X64_MSR_PAT] =
	    PATENTRY(0, PAT_WB) | PATENTRY(1, PAT_WT) |
	    PATENTRY(2, PAT_UCMINUS) | PATENTRY(3, PAT_UC) |
	    PATENTRY(4, PAT_WB) | PATENTRY(5, PAT_WT) |
	    PATENTRY(6, PAT_UCMINUS) | PATENTRY(7, PAT_UC);

	/* Page tables. */
	state.crs[NVMM_X64_CR_CR3] = 0x3000;

	state.gprs[NVMM_X64_GPR_RIP] = 0x2000;

	if (nvmm_vcpu_setstate(mach, 0, &state, NVMM_X64_STATE_ALL) == -1)
		err(errno, "nvmm_vcpu_setstate");
}

static void
map_pages(struct nvmm_machine *mach)
{
	pt_entry_t *L4, *L3, *L2, *L1;

	instbuf = mmap(NULL, PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_ANON|MAP_PRIVATE,
	    -1, 0);
	if (instbuf == MAP_FAILED)
		err(errno, "mmap");

	if (nvmm_hva_map(mach, (uintptr_t)instbuf, PAGE_SIZE) == -1)
		err(errno, "nvmm_hva_map");
	if (nvmm_gpa_map(mach, (uintptr_t)instbuf, 0x2000, PAGE_SIZE, 0) == -1)
		err(errno, "nvmm_gpa_map");

	L4 = mmap(NULL, PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_ANON|MAP_PRIVATE,
	    -1, 0);
	if (L4 == MAP_FAILED)
		err(errno, "mmap");
	L3 = mmap(NULL, PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_ANON|MAP_PRIVATE,
	    -1, 0);
	if (L3 == MAP_FAILED)
		err(errno, "mmap");
	L2 = mmap(NULL, PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_ANON|MAP_PRIVATE,
	    -1, 0);
	if (L2 == MAP_FAILED)
		err(errno, "mmap");
	L1 = mmap(NULL, PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_ANON|MAP_PRIVATE,
	    -1, 0);
	if (L1 == MAP_FAILED)
		err(errno, "mmap");

	if (nvmm_hva_map(mach, (uintptr_t)L4, PAGE_SIZE) == -1)
		err(errno, "nvmm_hva_map");
	if (nvmm_hva_map(mach, (uintptr_t)L3, PAGE_SIZE) == -1)
		err(errno, "nvmm_hva_map");
	if (nvmm_hva_map(mach, (uintptr_t)L2, PAGE_SIZE) == -1)
		err(errno, "nvmm_hva_map");
	if (nvmm_hva_map(mach, (uintptr_t)L1, PAGE_SIZE) == -1)
		err(errno, "nvmm_hva_map");

	if (nvmm_gpa_map(mach, (uintptr_t)L4, 0x3000, PAGE_SIZE, 0) == -1)
		err(errno, "nvmm_gpa_map");
	if (nvmm_gpa_map(mach, (uintptr_t)L3, 0x4000, PAGE_SIZE, 0) == -1)
		err(errno, "nvmm_gpa_map");
	if (nvmm_gpa_map(mach, (uintptr_t)L2, 0x5000, PAGE_SIZE, 0) == -1)
		err(errno, "nvmm_gpa_map");
	if (nvmm_gpa_map(mach, (uintptr_t)L1, 0x6000, PAGE_SIZE, 0) == -1)
		err(errno, "nvmm_gpa_map");

	memset(L4, 0, PAGE_SIZE);
	memset(L3, 0, PAGE_SIZE);
	memset(L2, 0, PAGE_SIZE);
	memset(L1, 0, PAGE_SIZE);

	L4[0] = PG_V | PG_RW | 0x4000;
	L3[0] = PG_V | PG_RW | 0x5000;
	L2[0] = PG_V | PG_RW | 0x6000;
	L1[0x2000 / PAGE_SIZE] = PG_V | PG_RW | 0x2000;
	L1[0x1000 / PAGE_SIZE] = PG_V | PG_RW | 0x1000;
}

/* -------------------------------------------------------------------------- */

static void
mem_callback(struct nvmm_mem *mem)
{
	size_t off;

	if (mem->gpa < 0x1000 || mem->gpa + mem->size > 0x1000 + PAGE_SIZE) {
		printf("Out of page\n");
		exit(-1);
	}

	off = mem->gpa - 0x1000;

	printf("-> gpa = %p\n", (void *)mem->gpa);

	if (mem->write) {
		memcpy(mmiobuf + off, mem->data, mem->size);
	} else {
		memcpy(mem->data, mmiobuf + off, mem->size);
	}
}

static int
handle_memory(struct nvmm_machine *mach, struct nvmm_exit *exit)
{
	int ret;

	ret = nvmm_assist_mem(mach, 0, exit);
	if (ret == -1) {
		err(errno, "nvmm_assist_mem");
	}

	return 0;
}

static void
run_machine(struct nvmm_machine *mach)
{
	struct nvmm_exit exit;

	while (1) {
		if (nvmm_vcpu_run(mach, 0, &exit) == -1)
			err(errno, "nvmm_vcpu_run");

		switch (exit.reason) {
		case NVMM_EXIT_NONE:
			break;

		case NVMM_EXIT_MSR:
			/* Stop here. */
			return;

		case NVMM_EXIT_MEMORY:
			handle_memory(mach, &exit);
			break;

		case NVMM_EXIT_SHUTDOWN:
			printf("Shutting down!\n");
			return;

		default:
			printf("Invalid!\n");
			return;
		}
	}
}

/* -------------------------------------------------------------------------- */

struct test {
	const char *name;
	uint8_t *code_begin;
	uint8_t *code_end;
	uint64_t wanted;
};

static void
run_test(struct nvmm_machine *mach, const struct test *test)
{
	uint64_t *res;
	size_t size;

	size = (size_t)test->code_end - (size_t)test->code_begin;

	reset_machine(mach);

	memset(mmiobuf, 0, PAGE_SIZE);
	memcpy(instbuf, test->code_begin, size);

	run_machine(mach);

	res = (uint64_t *)mmiobuf;
	if (*res == test->wanted) {
		printf("Test '%s' passed\n", test->name);
	} else {
		printf("Test '%s' failed, wanted 0x%lx, got 0x%lx\n", test->name,
		    test->wanted, *res);
	}
}

/* -------------------------------------------------------------------------- */

extern uint8_t test1_begin, test1_end;
extern uint8_t test2_begin, test2_end;
extern uint8_t test3_begin, test3_end;
extern uint8_t test4_begin, test4_end;
extern uint8_t test5_begin, test5_end;
extern uint8_t test6_begin, test6_end;
extern uint8_t test7_begin, test7_end;
extern uint8_t test8_begin, test8_end;
extern uint8_t test9_begin, test9_end;
extern uint8_t test10_begin, test10_end;
extern uint8_t test11_begin, test11_end;
extern uint8_t test12_begin, test12_end;
extern uint8_t test13_begin, test13_end;
extern uint8_t test14_begin, test14_end;

static const struct test tests[] = {
	{ "test1 - MOV", &test1_begin, &test1_end, 0x3004 },
	{ "test2 - OR",  &test2_begin, &test2_end, 0x16FF },
	{ "test3 - AND", &test3_begin, &test3_end, 0x1FC0 },
	{ "test4 - XOR", &test4_begin, &test4_end, 0x10CF },
	{ "test5 - Address Sizes", &test5_begin, &test5_end, 0x1F00 },
	{ "test6 - DMO", &test6_begin, &test6_end, 0xFFAB },
	{ "test7 - STOS", &test7_begin, &test7_end, 0x00123456 },
	{ "test8 - LODS", &test8_begin, &test8_end, 0x12345678 },
	{ "test9 - MOVS", &test9_begin, &test9_end, 0x12345678 },
	{ "test10 - MOVZXB", &test10_begin, &test10_end, 0x00000078 },
	{ "test11 - MOVZXW", &test11_begin, &test11_end, 0x00005678 },
	{ "test12 - CMP", &test12_begin, &test12_end, 0x00000001 },
	{ "test13 - SUB", &test13_begin, &test13_end, 0x0000000F0000A0FF },
	{ "test14 - TEST", &test14_begin, &test14_end, 0x00000001 },
	{ NULL, NULL, NULL, -1 }
};

static const struct nvmm_callbacks callbacks = {
	.io = NULL,
	.mem = mem_callback
};

/*
 * 0x1000: MMIO address, unmapped
 * 0x2000: Instructions, mapped
 * 0x3000: L4
 * 0x4000: L3
 * 0x5000: L2
 * 0x6000: L1
 */
int main(int argc, char *argv[])
{
	struct nvmm_machine mach;
	size_t i;

	if (nvmm_machine_create(&mach) == -1)
		err(errno, "nvmm_machine_create");
	if (nvmm_vcpu_create(&mach, 0) == -1)
		err(errno, "nvmm_vcpu_create");
	nvmm_callbacks_register(&callbacks);
	map_pages(&mach);

	for (i = 0; tests[i].name != NULL; i++) {
		run_test(&mach, &tests[i]);
	}

	return 0;
}
