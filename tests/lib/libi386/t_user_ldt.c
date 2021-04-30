/*	$NetBSD: t_user_ldt.c,v 1.6 2021/04/30 13:53:30 christos Exp $	*/

/*
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
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
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/mman.h>
#include <machine/segments.h>
#include <machine/sysarch.h>
#include <machine/vmparam.h>
#include <machine/gdt.h>

#include <atf-c.h>

static uint8_t *ldt_base;
static bool user_ldt_supported;

static void
user_ldt_detect(void)
{
	union descriptor desc;
	int ret;

	ret = i386_get_ldt(0, &desc, 1);
	user_ldt_supported = (ret != -1) || (errno != ENOSYS && errno != EPERM);
}

static void
init_ldt_base(void)
{
	ldt_base = (uint8_t *)mmap(NULL, 2 * PAGE_SIZE, PROT_READ | PROT_WRITE,
	    MAP_PRIVATE | MAP_ANON, -1, 0);
	if (ldt_base == MAP_FAILED)
		atf_tc_fail("mmap failed");
	munmap(ldt_base + PAGE_SIZE, PAGE_SIZE);
}

static void
build_desc(union descriptor *desc, void *basep, uint32_t limit, int type,
    int dpl, int def32, int gran)
{
	uintptr_t base = (uintptr_t)basep;

	limit--;

	desc->sd.sd_lolimit = limit & 0x0000ffff;
	desc->sd.sd_lobase  = base & 0x00ffffff;
	desc->sd.sd_type    = type & 0x1F;
	desc->sd.sd_dpl     = dpl & 0x3;
	desc->sd.sd_p       = 1;
	desc->sd.sd_hilimit = (limit & 0x00ff0000) >> 16;
	desc->sd.sd_xx      = 0;
	desc->sd.sd_def32   = def32 ? 1 : 0;
	desc->sd.sd_gran    = gran ? 1 : 0;
	desc->sd.sd_hibase  = (base & 0xff000000) >> 24;
}

static void
set_ds(unsigned int val)
{
	__asm volatile("mov %0,%%ds"::"r" ((unsigned short)val));
}

static void
set_fs(unsigned int val)
{
	__asm volatile("mov %0,%%fs"::"r" ((unsigned short)val));
}

static uint8_t __noinline
get_fs_byte(const char *addr)
{
	uint8_t val;
	__asm volatile (
		".globl fs_read_begin; fs_read_begin:\n"
		"movb %%fs:%1,%0\n"
		".globl fs_read_end; fs_read_end:\n"
		: "=q" (val) : "m" (*addr)
	);
	return val;
}

/* -------------------------------------------------------------------------- */

ATF_TC(filter_ops);
ATF_TC_HEAD(filter_ops, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Ensure that the kernel correctly filters the descriptors");
}
ATF_TC_BODY(filter_ops, tc)
{
	union descriptor desc;
	const int forbidden_types[] = {
		SDT_SYS286TSS,
		SDT_SYSLDT,
		SDT_SYS286BSY,
		SDT_SYS286CGT,
		SDT_SYSTASKGT,
		SDT_SYS286IGT,
		SDT_SYS286TGT,
		SDT_SYSNULL2,
		SDT_SYS386TSS,
		SDT_SYSNULL3,
		SDT_SYS386BSY,
		SDT_SYS386CGT,
		SDT_SYSNULL4,
		SDT_SYS386IGT,
		SDT_SYS386TGT
	};
	size_t i;

	if (!user_ldt_supported) {
		atf_tc_skip("USER_LDT disabled");
	}

	/* The first LDT slots should not be settable. */
	for (i = 0; i < 10; i++) {
		build_desc(&desc, ldt_base, PAGE_SIZE, SDT_MEMRW,
		    SEL_UPL, 1, 0);
		ATF_REQUIRE_EQ(i386_set_ldt(i, &desc, 1), -1);
		ATF_REQUIRE_EQ(errno, EINVAL);
	}

	/* SEL_KPL should not be allowed. */
	build_desc(&desc, ldt_base, PAGE_SIZE, SDT_MEMRW, SEL_KPL, 1, 0);
	ATF_REQUIRE_EQ(i386_set_ldt(256, &desc, 1), -1);
	ATF_REQUIRE_EQ(errno, EACCES);

	/* Long-mode segments should not be allowed. */
	build_desc(&desc, ldt_base, PAGE_SIZE, SDT_MEMRW, SEL_UPL, 1, 0);
	desc.sd.sd_xx = 0b11; /* sd_avl | sd_long */
	ATF_REQUIRE_EQ(i386_set_ldt(256, &desc, 1), -1);
	ATF_REQUIRE_EQ(errno, EACCES);

	/* No forbidden type should be allowed. */
	for (i = 0; i < __arraycount(forbidden_types); i++) {
		build_desc(&desc, ldt_base, PAGE_SIZE, forbidden_types[i],
		    SEL_UPL, 1, 0);
		ATF_REQUIRE_EQ(i386_set_ldt(256, &desc, 1), -1);
		ATF_REQUIRE_EQ(errno, EACCES);
	}

	/* Check the slot limit. */
	build_desc(&desc, ldt_base, PAGE_SIZE, SDT_MEMRW, SEL_UPL, 1, 0);
	ATF_REQUIRE_EQ(i386_set_ldt(MAX_USERLDT_SLOTS-1, &desc, 1),
	    MAX_USERLDT_SLOTS-1);
	ATF_REQUIRE_EQ(i386_set_ldt(MAX_USERLDT_SLOTS, &desc, 1), -1);
	ATF_REQUIRE_EQ(errno, EINVAL);
}

/* -------------------------------------------------------------------------- */

static void
iretq_gp__gp_handler(int signo, siginfo_t *sig, void *ctx)
{
	ATF_REQUIRE(sig->si_signo == SIGSEGV);
	ATF_REQUIRE(sig->si_code == SEGV_ACCERR);
	atf_tc_pass();
	/* NOTREACHED */
}

ATF_TC(iretq_gp);
ATF_TC_HEAD(iretq_gp, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Ensure that the kernel correctly handles iretq #GP faults");
}
ATF_TC_BODY(iretq_gp, tc)
{
	union descriptor desc;
	struct sigaction act;

	if (!user_ldt_supported) {
		atf_tc_skip("USER_LDT disabled");
	}

	/* Build a desc, set %ds to it. */
	build_desc(&desc, 0, 0xFFFFFUL, SDT_MEMRWA, SEL_UPL, 1, 1);
	ATF_REQUIRE_EQ(i386_set_ldt(256, &desc, 1), 256);
	set_ds(LSEL(256, SEL_UPL));

	/* Register the fault handler. */
	memset(&act, 0, sizeof(act));
	act.sa_sigaction = iretq_gp__gp_handler;
	act.sa_flags = SA_SIGINFO;
	ATF_REQUIRE_EQ(sigaction(SIGSEGV, &act, NULL), 0);

	/* Set NULL on %ds, iretq should fault. */
	memset(&desc, 0, sizeof(desc));
	ATF_REQUIRE_EQ(i386_set_ldt(256, &desc, 1), 256);

	atf_tc_fail("test did not fault as expected");
}

/* -------------------------------------------------------------------------- */

static void
iretq_np__np_handler(int signo, siginfo_t *sig, void *ctx)
{
	ATF_REQUIRE(sig->si_signo == SIGBUS);
	ATF_REQUIRE(sig->si_code == BUS_ADRERR);
	atf_tc_pass();
	/* NOTREACHED */
}

ATF_TC(iretq_np);
ATF_TC_HEAD(iretq_np, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Ensure that the kernel correctly handles iretq #NP faults");
}
ATF_TC_BODY(iretq_np, tc)
{
	union descriptor desc;
	struct sigaction act;

	if (!user_ldt_supported) {
		atf_tc_skip("USER_LDT disabled");
	}

	/* Build a desc, set %ds to it. */
	build_desc(&desc, 0, 0xFFFFFFFFUL, SDT_MEMRWA, SEL_UPL, 1, 1);
	ATF_REQUIRE_EQ(i386_set_ldt(256, &desc, 1), 256);
	set_ds(LSEL(256, SEL_UPL));

	/* Register the fault handler. */
	memset(&act, 0, sizeof(act));
	act.sa_sigaction = iretq_np__np_handler;
	act.sa_flags = SA_SIGINFO;
	ATF_REQUIRE_EQ(sigaction(SIGBUS, &act, NULL), 0);

	/* Set non-present on %ds, iretq should fault. */
	desc.sd.sd_p = 0;
	ATF_REQUIRE_EQ(i386_set_ldt(256, &desc, 1), 256);

	atf_tc_fail("test did not fault as expected");
}

/* -------------------------------------------------------------------------- */

static volatile bool expect_crash;

static void
user_ldt__gp_handler(int signo, siginfo_t *sig, void *ctx)
{
	ucontext_t *uctx = ctx;
	extern uint8_t fs_read_begin;

	if (!expect_crash) {
		atf_tc_fail("unexpected #GP");
	}

	ATF_REQUIRE(sig->si_signo == SIGSEGV);
	ATF_REQUIRE(sig->si_code == SEGV_ACCERR);

	if (uctx->uc_mcontext.__gregs[_REG_EIP] != (intptr_t)&fs_read_begin) {
		atf_tc_fail("got #GP on the wrong instruction");
	}

	set_fs(GSEL(GUDATA_SEL, SEL_UPL));

	atf_tc_pass();
	/* NOTREACHED */
}

ATF_TC(user_ldt);
ATF_TC_HEAD(user_ldt, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Ensure that USER_LDT works as expected");
}
ATF_TC_BODY(user_ldt, tc)
{
	union descriptor desc;
	struct sigaction act;

	if (!user_ldt_supported) {
		atf_tc_skip("USER_LDT disabled");
	}

	memset(&act, 0, sizeof(act));
	act.sa_sigaction = user_ldt__gp_handler;
	act.sa_flags = SA_SIGINFO;
	ATF_REQUIRE_EQ(sigaction(SIGSEGV, &act, NULL), 0);

	build_desc(&desc, ldt_base, PAGE_SIZE, SDT_MEMRW, SEL_UPL, 1, 0);
	ATF_REQUIRE_EQ(i386_set_ldt(256, &desc, 1), 256);

	set_fs(LSEL(256, SEL_UPL));

	ldt_base[666] = 123;
	ldt_base[PAGE_SIZE-1] = 213;
	__insn_barrier();
	ATF_REQUIRE_EQ(get_fs_byte((char *)666), 123);
	ATF_REQUIRE_EQ(get_fs_byte((char *)PAGE_SIZE-1), 213);

	/* This one should fault, and it concludes our test case. */
	expect_crash = true;
	get_fs_byte((char *)PAGE_SIZE);

	atf_tc_fail("test did not fault as expected");
}

/* -------------------------------------------------------------------------- */

ATF_TP_ADD_TCS(tp)
{
	user_ldt_detect();
	init_ldt_base();

	ATF_TP_ADD_TC(tp, filter_ops);
	ATF_TP_ADD_TC(tp, iretq_gp);
	ATF_TP_ADD_TC(tp, iretq_np);
	ATF_TP_ADD_TC(tp, user_ldt);

	return atf_no_error();
}
