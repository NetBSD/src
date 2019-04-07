/*	$NetBSD: t_ufetchstore.c,v 1.4 2019/04/07 15:50:12 thorpej Exp $	*/

/*
 * Copyright (c) 2019 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>
__COPYRIGHT("@(#) Copyright (c) 2019\
 The NetBSD Foundation, inc. All rights reserved.");
__RCSID("$NetBSD: t_ufetchstore.c,v 1.4 2019/04/07 15:50:12 thorpej Exp $");

#include <sys/types.h>
#include <sys/endian.h>
#include <sys/module.h>
#include <sys/sysctl.h>

#include <err.h>
#include <errno.h>
#include <limits.h>

#include <atf-c.h>

#include "common.h"

#define	mib_name	"kern.ufetchstore_test.test"

static bool module_loaded;

#define	MODULE_PATH	\
	"/usr/tests/modules/ufetchstore_tester/ufetchstore_tester.kmod"
#define	MODULE_NAME	"ufetchstore_tester"

#define	CHECK_MODULE()							\
do {									\
	load_module();							\
	if (! module_loaded) {						\
		atf_tc_skip("loading '%s' module failed.", MODULE_NAME);\
	}								\
} while (/*CONSTCOND*/0)

static void
load_module(void)
{
#ifndef SKIP_MODULE
	if (module_loaded)
		return;

	modctl_load_t params = {
		.ml_filename = MODULE_PATH,
		.ml_flags = MODCTL_NO_PROP,
	};

	if (modctl(MODCTL_LOAD, &params) != 0) {
		warn("failed to load module '%s'", MODULE_PATH);
	} else {
		module_loaded = true;
	}
#else
	module_loaded = true;
#endif /* ! SKIP_MODULE */
}

#define	UADDR(x)	((uintptr_t)(x))

static void
unload_module(void)
{
#ifndef SKIP_MODULE
	char module_name[] = MODULE_NAME;

	if (modctl(MODCTL_UNLOAD, module_name) != 0) {
		warn("failed to unload module '%s'", MODULE_NAME);
	} else {
		module_loaded = false;
	}
#endif /* ! SKIP_MODULE */
}

static unsigned long
vm_max_address_raw(void)
{
	static unsigned long max_addr = 0;
	int rv;

	if (max_addr == 0) {
		size_t max_addr_size = sizeof(max_addr);
		rv = sysctlbyname("vm.maxaddress", &max_addr, &max_addr_size,
				  NULL, 0);
		if (rv != 0)
	                err(1, "sysctlbyname('vm.maxaddress')");
        }
	return max_addr;
}

static void *
vm_max_address(void)
{
	return (void *)vm_max_address_raw();
}

static void *
vm_max_address_minus(unsigned int adj)
{
	return (void *)(vm_max_address_raw() - adj);
}

static int
do_sysctl(struct ufetchstore_test_args *args)
{
	uint64_t arg_addr64 = (uintptr_t)args;
	int rv;

	args->fetchstore_error = EBADF;	/* poison */
	args->pointer_size = (int)sizeof(void *);

	/*
	 * Yes, the intent is to provide the pointer, not the structure,
	 * to the kernel side of the test harness.
	 */
	rv = sysctlbyname(mib_name, NULL, NULL, &arg_addr64,
			  sizeof(arg_addr64));
	if (rv != 0) {
		rv = errno;
		warn("sysctlbyname('%s') -> %d", mib_name, rv);
		return rv;
	}
	return 0;
}

static int
do_ufetch_8(const uint8_t *uaddr, uint8_t *res)
{
	struct ufetchstore_test_args args = {
		.uaddr64 = UADDR(uaddr),
		.test_op = OP_LOAD,
		.size = 8,
	};

	ATF_REQUIRE_EQ(do_sysctl(&args), 0);
	*res = args.val8;
	return args.fetchstore_error;
}

static int
do_ufetch_16(const uint16_t *uaddr, uint16_t *res)
{
	struct ufetchstore_test_args args = {
		.uaddr64 = UADDR(uaddr),
		.test_op = OP_LOAD,
		.size = 16,
	};

	ATF_REQUIRE_EQ(do_sysctl(&args), 0);
	*res = args.val16;
	return args.fetchstore_error;
}

static int
do_ufetch_32(const uint32_t *uaddr, uint32_t *res)
{
	struct ufetchstore_test_args args = {
		.uaddr64 = UADDR(uaddr),
		.test_op = OP_LOAD,
		.size = 32,
	};

	ATF_REQUIRE_EQ(do_sysctl(&args), 0);
	*res = args.val32;
	return args.fetchstore_error;
}

#ifdef _LP64
static int
do_ufetch_64(const uint64_t *uaddr, uint64_t *res)
{
	struct ufetchstore_test_args args = {
		.uaddr64 = UADDR(uaddr),
		.test_op = OP_LOAD,
		.size = 64,
	};

	ATF_REQUIRE_EQ(do_sysctl(&args), 0);
	*res = args.val64;
	return args.fetchstore_error;
}
#endif /* _LP64 */

static int
do_ustore_8(uint8_t *uaddr, uint8_t val)
{
	struct ufetchstore_test_args args = {
		.uaddr64 = UADDR(uaddr),
		.test_op = OP_STORE,
		.size = 8,
		.val8 = val,
	};

	ATF_REQUIRE_EQ(do_sysctl(&args), 0);
	return args.fetchstore_error;
}

static int
do_ustore_16(uint16_t *uaddr, uint16_t val)
{
	struct ufetchstore_test_args args = {
		.uaddr64 = UADDR(uaddr),
		.test_op = OP_STORE,
		.size = 16,
		.val16 = val,
	};

	ATF_REQUIRE_EQ(do_sysctl(&args), 0);
	return args.fetchstore_error;
}

static int
do_ustore_32(uint32_t *uaddr, uint32_t val)
{
	struct ufetchstore_test_args args = {
		.uaddr64 = UADDR(uaddr),
		.test_op = OP_STORE,
		.size = 32,
		.val32 = val,
	};

	ATF_REQUIRE_EQ(do_sysctl(&args), 0);
	return args.fetchstore_error;
}

#ifdef _LP64
static int
do_ustore_64(uint64_t *uaddr, uint64_t val)
{
	struct ufetchstore_test_args args = {
		.uaddr64 = UADDR(uaddr),
		.test_op = OP_STORE,
		.size = 64,
		.val64 = val,
	};

	ATF_REQUIRE_EQ(do_sysctl(&args), 0);
	return args.fetchstore_error;
}
#endif /* _LP64 */

static int
do_ucas_32(uint32_t *uaddr, uint32_t expected, uint32_t new, uint32_t *actualp)
{
	struct ufetchstore_test_args args = {
		.uaddr64 = UADDR(uaddr),
		.test_op = OP_CAS,
		.size = 32,
		.val32 = new,
		.ea_val32 = expected,
	};

	ATF_REQUIRE_EQ(do_sysctl(&args), 0);
	*actualp = args.ea_val32;
	return args.fetchstore_error;
}

#ifdef _LP64
static int
do_ucas_64(uint64_t *uaddr, uint64_t expected, uint64_t new, uint64_t *actualp)
{
	struct ufetchstore_test_args args = {
		.uaddr64 = UADDR(uaddr),
		.test_op = OP_CAS,
		.size = 64,
		.val64 = new,
		.ea_val64 = expected,
	};

	ATF_REQUIRE_EQ(do_sysctl(&args), 0);
	*actualp = args.ea_val64;
	return args.fetchstore_error;
}
#endif /* _LP64 */

struct memory_cell {
	unsigned long guard0;
	union {
		unsigned long test_cell;
#ifdef _LP64
		uint64_t val64;
#endif
		uint32_t val32[sizeof(long) / 4];
		uint16_t val16[sizeof(long) / 2];
		uint8_t  val8 [sizeof(long)    ];
	};
	unsigned long guard1;
};

#define	index8		1
#define	index16		1
#define	index32		0

#define	test_pattern8	0xa5
#define	test_pattern16	0x5a6b
#define	test_pattern32	0xb01cafe1
#ifdef _LP64
#define	test_pattern64	0xcafedeadfeedbabe
#endif

#if _BYTE_ORDER == _LITTLE_ENDIAN
#define	test_cell_val8	((unsigned long)test_pattern8  << (index8  * NBBY))
#define	test_cell_val16	((unsigned long)test_pattern16 << (index16 * NBBY*2))
#define	test_cell_val32	((unsigned long)test_pattern32 << (index32 * NBBY*4))
#ifdef _LP64
#define	test_cell_val64	((unsigned long)test_pattern64)
#endif
#endif /* _BYTE_ORDER == _LITTLE_ENDIAN */

#if _BYTE_ORDER == _BIG_ENDIAN
#ifdef _LP64
#define	test_cell_val8	((unsigned long)test_pattern8  << (56-(index8  * NBBY)))
#define	test_cell_val16	((unsigned long)test_pattern16 << (48-(index16 * NBBY*2)))
#define	test_cell_val32	((unsigned long)test_pattern32 << (32-(index32 * NBBY*4)))
#define	test_cell_val64	((unsigned long)test_pattern64)
#else /* ! _LP64 */
#define	test_cell_val8	((unsigned long)test_pattern8  << (24-(index8  * NBBY)))
#define	test_cell_val16	((unsigned long)test_pattern16 << (16-(index16 * NBBY*2)))
#define	test_cell_val32	((unsigned long)test_pattern32)
#endif /* _LP64 */
#endif /* #if _BYTE_ORDER == _BIG_ENDIAN */

#define	read_test_cell(cell)		(cell)->test_cell
#define	write_test_cell(cell, v)	(cell)->test_cell = (v)

#define	memory_cell_initializer		\
	{				\
		.guard0 = ULONG_MAX,	\
		.test_cell = 0,		\
		.guard1 = ULONG_MAX,	\
	}

static bool
memory_cell_check_guard(const struct memory_cell * const cell)
{
	return cell->guard0 == ULONG_MAX &&
	       cell->guard1 == ULONG_MAX;
}

ATF_TC_WITH_CLEANUP(ufetch_8);
ATF_TC_HEAD(ufetch_8, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "test for correct ufetch_8 behavior");
}
ATF_TC_BODY(ufetch_8, tc)
{
	struct memory_cell cell = memory_cell_initializer;
	uint8_t res;

	CHECK_MODULE();

	write_test_cell(&cell, test_cell_val8);
	ATF_REQUIRE_EQ(do_ufetch_8(&cell.val8[index8], &res), 0);
	ATF_REQUIRE(memory_cell_check_guard(&cell));
	ATF_REQUIRE(res == test_pattern8);
}
ATF_TC_CLEANUP(ufetch_8, tc)
{
	unload_module();
}

ATF_TC_WITH_CLEANUP(ufetch_16);
ATF_TC_HEAD(ufetch_16, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "test for correct ufetch_16 behavior");
}
ATF_TC_BODY(ufetch_16, tc)
{
	struct memory_cell cell = memory_cell_initializer;
	uint16_t res;

	CHECK_MODULE();

	write_test_cell(&cell, test_cell_val16);
	ATF_REQUIRE_EQ(do_ufetch_16(&cell.val16[index16], &res), 0);
	ATF_REQUIRE(memory_cell_check_guard(&cell));
	ATF_REQUIRE(res == test_pattern16);
}
ATF_TC_CLEANUP(ufetch_16, tc)
{
	unload_module();
}

ATF_TC_WITH_CLEANUP(ufetch_32);
ATF_TC_HEAD(ufetch_32, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "test for correct ufetch_32 behavior");
}
ATF_TC_BODY(ufetch_32, tc)
{
	struct memory_cell cell = memory_cell_initializer;
	uint32_t res;

	CHECK_MODULE();

	write_test_cell(&cell, test_cell_val32);
	ATF_REQUIRE_EQ(do_ufetch_32(&cell.val32[index32], &res), 0);
	ATF_REQUIRE(memory_cell_check_guard(&cell));
	ATF_REQUIRE(res == test_pattern32);
}
ATF_TC_CLEANUP(ufetch_32, tc)
{
	unload_module();
}

#ifdef _LP64
ATF_TC_WITH_CLEANUP(ufetch_64);
ATF_TC_HEAD(ufetch_64, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "test for correct ufetch_64 behavior");
}
ATF_TC_BODY(ufetch_64, tc)
{
	struct memory_cell cell = memory_cell_initializer;
	uint64_t res;

	CHECK_MODULE();

	write_test_cell(&cell, test_cell_val64);
	ATF_REQUIRE_EQ(do_ufetch_64(&cell.val64, &res), 0);
	ATF_REQUIRE(memory_cell_check_guard(&cell));
	ATF_REQUIRE(res == test_pattern64);
}
ATF_TC_CLEANUP(ufetch_64, tc)
{
	unload_module();
}
#endif /* _LP64 */

ATF_TC_WITH_CLEANUP(ufetch_8_null);
ATF_TC_HEAD(ufetch_8_null, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "test for correct ufetch_8 NULL pointer behavior");
}
ATF_TC_BODY(ufetch_8_null, tc)
{
	uint8_t res;

	CHECK_MODULE();

	ATF_REQUIRE_EQ(do_ufetch_8(NULL, &res), EFAULT);
}
ATF_TC_CLEANUP(ufetch_8_null, tc)
{
	unload_module();
}

ATF_TC_WITH_CLEANUP(ufetch_16_null);
ATF_TC_HEAD(ufetch_16_null, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "test for correct ufetch_16 NULL pointer behavior");
}
ATF_TC_BODY(ufetch_16_null, tc)
{
	uint16_t res;

	CHECK_MODULE();

	ATF_REQUIRE_EQ(do_ufetch_16(NULL, &res), EFAULT);
}
ATF_TC_CLEANUP(ufetch_16_null, tc)
{
	unload_module();
}

ATF_TC_WITH_CLEANUP(ufetch_32_null);
ATF_TC_HEAD(ufetch_32_null, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "test for correct ufetch_32 NULL pointer behavior");
}
ATF_TC_BODY(ufetch_32_null, tc)
{
	uint32_t res;

	CHECK_MODULE();

	ATF_REQUIRE_EQ(do_ufetch_32(NULL, &res), EFAULT);
}
ATF_TC_CLEANUP(ufetch_32_null, tc)
{
	unload_module();
}

#ifdef _LP64
ATF_TC_WITH_CLEANUP(ufetch_64_null);
ATF_TC_HEAD(ufetch_64_null, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "test for correct ufetch_64 NULL pointer behavior");
}
ATF_TC_BODY(ufetch_64_null, tc)
{
	uint64_t res;

	CHECK_MODULE();

	ATF_REQUIRE_EQ(do_ufetch_64(NULL, &res), EFAULT);
}
ATF_TC_CLEANUP(ufetch_64_null, tc)
{
	unload_module();
}
#endif /* _LP64 */

ATF_TC_WITH_CLEANUP(ufetch_8_max);
ATF_TC_HEAD(ufetch_8_max, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "test for correct ufetch_8 VM_MAX_ADDRESS pointer behavior");
}
ATF_TC_BODY(ufetch_8_max, tc)
{
	uint8_t res;

	CHECK_MODULE();

	ATF_REQUIRE_EQ(do_ufetch_8(vm_max_address(), &res), EFAULT);
}
ATF_TC_CLEANUP(ufetch_8_max, tc)
{
	unload_module();
}

ATF_TC_WITH_CLEANUP(ufetch_16_max);
ATF_TC_HEAD(ufetch_16_max, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "test for correct ufetch_16 VM_MAX_ADDRESS pointer behavior");
}
ATF_TC_BODY(ufetch_16_max, tc)
{
	uint16_t res;

	CHECK_MODULE();

	ATF_REQUIRE_EQ(do_ufetch_16(vm_max_address(), &res), EFAULT);
}
ATF_TC_CLEANUP(ufetch_16_max, tc)
{
	unload_module();
}

ATF_TC_WITH_CLEANUP(ufetch_32_max);
ATF_TC_HEAD(ufetch_32_max, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "test for correct ufetch_32 VM_MAX_ADDRESS pointer behavior");
}
ATF_TC_BODY(ufetch_32_max, tc)
{
	uint32_t res;

	CHECK_MODULE();

	ATF_REQUIRE_EQ(do_ufetch_32(vm_max_address(), &res), EFAULT);
}
ATF_TC_CLEANUP(ufetch_32_max, tc)
{
	unload_module();
}

#ifdef _LP64
ATF_TC_WITH_CLEANUP(ufetch_64_max);
ATF_TC_HEAD(ufetch_64_max, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "test for correct ufetch_64 VM_MAX_ADDRESS pointer behavior");
}
ATF_TC_BODY(ufetch_64_max, tc)
{
	uint64_t res;

	CHECK_MODULE();

	ATF_REQUIRE_EQ(do_ufetch_64(vm_max_address(), &res), EFAULT);
}
ATF_TC_CLEANUP(ufetch_64_max, tc)
{
	unload_module();
}
#endif /* _LP64 */

ATF_TC_WITH_CLEANUP(ufetch_16_nearmax_overflow);
ATF_TC_HEAD(ufetch_16_nearmax_overflow, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "test for correct ufetch_16 near-VM_MAX_ADDRESS pointer behavior");
}
ATF_TC_BODY(ufetch_16_nearmax_overflow, tc)
{
	uint16_t res;

	CHECK_MODULE();

	/*
	 * For no-strict-alignment platforms: address checks must return
	 * EFAULT.
	 *
	 * For strict-alignment platforms: alignment checks must return
	 * EFAULT.
	 */
	ATF_REQUIRE_EQ(do_ufetch_16(vm_max_address_minus(1), &res), EFAULT);
}
ATF_TC_CLEANUP(ufetch_16_nearmax_overflow, tc)
{
	unload_module();
}

ATF_TC_WITH_CLEANUP(ufetch_32_nearmax_overflow);
ATF_TC_HEAD(ufetch_32_nearmax_overflow, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "test for correct ufetch_32 near-VM_MAX_ADDRESS pointer behavior");
}
ATF_TC_BODY(ufetch_32_nearmax_overflow, tc)
{
	uint32_t res;

	CHECK_MODULE();

	/*
	 * For no-strict-alignment platforms: address checks must return
	 * EFAULT.
	 *
	 * For strict-alignment platforms: alignment checks must return
	 * EFAULT.
	 */
	ATF_REQUIRE_EQ(do_ufetch_32(vm_max_address_minus(3), &res), EFAULT);
}
ATF_TC_CLEANUP(ufetch_32_nearmax_overflow, tc)
{
	unload_module();
}

#ifdef _LP64
ATF_TC_WITH_CLEANUP(ufetch_64_nearmax_overflow);
ATF_TC_HEAD(ufetch_64_nearmax_overflow, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "test for correct ufetch_64 near-VM_MAX_ADDRESS pointer behavior");
}
ATF_TC_BODY(ufetch_64_nearmax_overflow, tc)
{
	uint64_t res;

	CHECK_MODULE();

	/*
	 * For no-strict-alignment platforms: address checks must return
	 * EFAULT.
	 *
	 * For strict-alignment platforms: alignment checks must return
	 * EFAULT.
	 */
	ATF_REQUIRE_EQ(do_ufetch_64(vm_max_address_minus(7), &res), EFAULT);
}
ATF_TC_CLEANUP(ufetch_64_nearmax_overflow, tc)
{
	unload_module();
}
#endif /* _LP64 */


ATF_TC_WITH_CLEANUP(ustore_8);
ATF_TC_HEAD(ustore_8, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "test for correct ustore_8 behavior");
}
ATF_TC_BODY(ustore_8, tc)
{
	struct memory_cell cell = memory_cell_initializer;

	CHECK_MODULE();

	ATF_REQUIRE_EQ(do_ustore_8(&cell.val8[index8], test_pattern8), 0);
	ATF_REQUIRE(memory_cell_check_guard(&cell));
	ATF_REQUIRE(read_test_cell(&cell) == test_cell_val8);
}
ATF_TC_CLEANUP(ustore_8, tc)
{
	unload_module();
}

ATF_TC_WITH_CLEANUP(ustore_16);
ATF_TC_HEAD(ustore_16, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "test for correct ustore_16 behavior");
}
ATF_TC_BODY(ustore_16, tc)
{
	struct memory_cell cell = memory_cell_initializer;

	CHECK_MODULE();

	ATF_REQUIRE_EQ(do_ustore_16(&cell.val16[index16], test_pattern16), 0);
	ATF_REQUIRE(memory_cell_check_guard(&cell));
	ATF_REQUIRE(read_test_cell(&cell) == test_cell_val16);
}
ATF_TC_CLEANUP(ustore_16, tc)
{
	unload_module();
}

ATF_TC_WITH_CLEANUP(ustore_32);
ATF_TC_HEAD(ustore_32, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "test for correct ustore_32 behavior");
}
ATF_TC_BODY(ustore_32, tc)
{
	struct memory_cell cell = memory_cell_initializer;

	CHECK_MODULE();

	ATF_REQUIRE_EQ(do_ustore_32(&cell.val32[index32], test_pattern32), 0);
	ATF_REQUIRE(memory_cell_check_guard(&cell));
	ATF_REQUIRE(read_test_cell(&cell) == test_cell_val32);
}
ATF_TC_CLEANUP(ustore_32, tc)
{
	unload_module();
}

#ifdef _LP64
ATF_TC_WITH_CLEANUP(ustore_64);
ATF_TC_HEAD(ustore_64, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "test for correct ustore_64 behavior");
}
ATF_TC_BODY(ustore_64, tc)
{
	struct memory_cell cell = memory_cell_initializer;

	CHECK_MODULE();

	ATF_REQUIRE_EQ(do_ustore_64(&cell.val64, test_pattern64), 0);
	ATF_REQUIRE(memory_cell_check_guard(&cell));
	ATF_REQUIRE(read_test_cell(&cell) == test_cell_val64);
}
ATF_TC_CLEANUP(ustore_64, tc)
{
	unload_module();
}
#endif /* _LP64 */

ATF_TC_WITH_CLEANUP(ustore_8_null);
ATF_TC_HEAD(ustore_8_null, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "test for correct ustore_8 NULL pointer behavior");
}
ATF_TC_BODY(ustore_8_null, tc)
{
	CHECK_MODULE();

	ATF_REQUIRE_EQ(do_ustore_8(NULL, 0), EFAULT);
}
ATF_TC_CLEANUP(ustore_8_null, tc)
{
	unload_module();
}

ATF_TC_WITH_CLEANUP(ustore_16_null);
ATF_TC_HEAD(ustore_16_null, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "test for correct ustore_16 NULL pointer behavior");
}
ATF_TC_BODY(ustore_16_null, tc)
{
	CHECK_MODULE();

	ATF_REQUIRE_EQ(do_ustore_16(NULL, 0), EFAULT);
}
ATF_TC_CLEANUP(ustore_16_null, tc)
{
	unload_module();
}

ATF_TC_WITH_CLEANUP(ustore_32_null);
ATF_TC_HEAD(ustore_32_null, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "test for correct ustore_32 NULL pointer behavior");
}
ATF_TC_BODY(ustore_32_null, tc)
{
	CHECK_MODULE();

	ATF_REQUIRE_EQ(do_ustore_32(NULL, 0), EFAULT);
}
ATF_TC_CLEANUP(ustore_32_null, tc)
{
	unload_module();
}

#ifdef _LP64
ATF_TC_WITH_CLEANUP(ustore_64_null);
ATF_TC_HEAD(ustore_64_null, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "test for correct ustore_64 NULL pointer behavior");
}
ATF_TC_BODY(ustore_64_null, tc)
{
	CHECK_MODULE();

	ATF_REQUIRE_EQ(do_ustore_64(NULL, 0), EFAULT);
}
ATF_TC_CLEANUP(ustore_64_null, tc)
{
	unload_module();
}
#endif /* _LP64 */

ATF_TC_WITH_CLEANUP(ustore_8_max);
ATF_TC_HEAD(ustore_8_max, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "test for correct ustore_8 VM_MAX_ADDRESS pointer behavior");
}
ATF_TC_BODY(ustore_8_max, tc)
{
	CHECK_MODULE();

	ATF_REQUIRE_EQ(do_ustore_8(vm_max_address(), 0), EFAULT);
}
ATF_TC_CLEANUP(ustore_8_max, tc)
{
	unload_module();
}

ATF_TC_WITH_CLEANUP(ustore_16_max);
ATF_TC_HEAD(ustore_16_max, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "test for correct ustore_16 VM_MAX_ADDRESS pointer behavior");
}
ATF_TC_BODY(ustore_16_max, tc)
{
	CHECK_MODULE();

	ATF_REQUIRE_EQ(do_ustore_16(vm_max_address(), 0), EFAULT);
}
ATF_TC_CLEANUP(ustore_16_max, tc)
{
	unload_module();
}

ATF_TC_WITH_CLEANUP(ustore_32_max);
ATF_TC_HEAD(ustore_32_max, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "test for correct ustore_32 VM_MAX_ADDRESS pointer behavior");
}
ATF_TC_BODY(ustore_32_max, tc)
{
	CHECK_MODULE();

	ATF_REQUIRE_EQ(do_ustore_32(vm_max_address(), 0), EFAULT);
}
ATF_TC_CLEANUP(ustore_32_max, tc)
{
	unload_module();
}

#ifdef _LP64
ATF_TC_WITH_CLEANUP(ustore_64_max);
ATF_TC_HEAD(ustore_64_max, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "test for correct ustore_64 VM_MAX_ADDRESS pointer behavior");
}
ATF_TC_BODY(ustore_64_max, tc)
{
	CHECK_MODULE();

	ATF_REQUIRE_EQ(do_ustore_64(vm_max_address(), 0), EFAULT);
}
ATF_TC_CLEANUP(ustore_64_max, tc)
{
	unload_module();
}
#endif /* _LP64 */

ATF_TC_WITH_CLEANUP(ustore_16_nearmax_overflow);
ATF_TC_HEAD(ustore_16_nearmax_overflow, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "test for correct ustore_16 VM_MAX_ADDRESS pointer behavior");
}
ATF_TC_BODY(ustore_16_nearmax_overflow, tc)
{
	CHECK_MODULE();

	/*
	 * For no-strict-alignment platforms: address checks must return
	 * EFAULT.
	 *
	 * For strict-alignment platforms: alignment checks must return
	 * EFAULT.
	 */
	ATF_REQUIRE_EQ(do_ustore_16(vm_max_address_minus(1), 0), EFAULT);
}
ATF_TC_CLEANUP(ustore_16_nearmax_overflow, tc)
{
	unload_module();
}

ATF_TC_WITH_CLEANUP(ustore_32_nearmax_overflow);
ATF_TC_HEAD(ustore_32_nearmax_overflow, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "test for correct ustore_32 VM_MAX_ADDRESS pointer behavior");
}
ATF_TC_BODY(ustore_32_nearmax_overflow, tc)
{
	CHECK_MODULE();

	/*
	 * For no-strict-alignment platforms: address checks must return
	 * EFAULT.
	 *
	 * For strict-alignment platforms: alignment checks must return
	 * EFAULT.
	 */
	ATF_REQUIRE_EQ(do_ustore_32(vm_max_address_minus(3), 0), EFAULT);
}
ATF_TC_CLEANUP(ustore_32_nearmax_overflow, tc)
{
	unload_module();
}

#ifdef _LP64
ATF_TC_WITH_CLEANUP(ustore_64_nearmax_overflow);
ATF_TC_HEAD(ustore_64_nearmax_overflow, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "test for correct ustore_64 VM_MAX_ADDRESS pointer behavior");
}
ATF_TC_BODY(ustore_64_nearmax_overflow, tc)
{
	CHECK_MODULE();

	/*
	 * For no-strict-alignment platforms: address checks must return
	 * EFAULT.
	 *
	 * For strict-alignment platforms: alignment checks must return
	 * EFAULT.
	 */
	ATF_REQUIRE_EQ(do_ustore_64(vm_max_address_minus(7), 0), EFAULT);
}
ATF_TC_CLEANUP(ustore_64_nearmax_overflow, tc)
{
	unload_module();
}
#endif /* _LP64 */


ATF_TC_WITH_CLEANUP(ucas_32);
ATF_TC_HEAD(ucas_32, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "test for correct ucas_32 behavior");
}
ATF_TC_BODY(ucas_32, tc)
{
	uint32_t cell = 0xdeadbeef;
	uint32_t actual = 0;

	CHECK_MODULE();

	ATF_REQUIRE_EQ(do_ucas_32(&cell, 0xdeadbeef, 0xbeefdead, &actual), 0);
	ATF_REQUIRE(actual == 0xdeadbeef);
	ATF_REQUIRE(cell == 0xbeefdead);
}
ATF_TC_CLEANUP(ucas_32, tc)
{
	unload_module();
}

#ifdef _LP64
ATF_TC_WITH_CLEANUP(ucas_64);
ATF_TC_HEAD(ucas_64, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "test for correct ucas_64 behavior");
}
ATF_TC_BODY(ucas_64, tc)
{
	uint64_t cell = 0xdeadbeef;
	uint64_t actual = 0;

	CHECK_MODULE();

	ATF_REQUIRE_EQ(do_ucas_64(&cell, 0xdeadbeef, 0xbeefdead, &actual), 0);
	ATF_REQUIRE(actual == 0xdeadbeef);
	ATF_REQUIRE(cell == 0xbeefdead);
}
ATF_TC_CLEANUP(ucas_64, tc)
{
	unload_module();
}
#endif /* _LP64 */

ATF_TC_WITH_CLEANUP(ucas_32_miscompare);
ATF_TC_HEAD(ucas_32_miscompare, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "test for correct ucas_32 behavior with miscompare");
}
ATF_TC_BODY(ucas_32_miscompare, tc)
{
	uint32_t cell = 0xa5a5a5a5;
	uint32_t actual = 0;

	CHECK_MODULE();

	ATF_REQUIRE_EQ(do_ucas_32(&cell, 0xdeadbeef, 0xbeefdead, &actual), 0);
	ATF_REQUIRE(actual == 0xa5a5a5a5);
	ATF_REQUIRE(cell == 0xa5a5a5a5);
}
ATF_TC_CLEANUP(ucas_32_miscompare, tc)
{
	unload_module();
}

#ifdef _LP64
ATF_TC_WITH_CLEANUP(ucas_64_miscompare);
ATF_TC_HEAD(ucas_64_miscompare, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "test for correct ucas_64 behavior with miscompare");
}
ATF_TC_BODY(ucas_64_miscompare, tc)
{
	uint64_t cell = 0xa5a5a5a5;
	uint64_t actual = 0;

	CHECK_MODULE();

	ATF_REQUIRE_EQ(do_ucas_64(&cell, 0xdeadbeef, 0xbeefdead, &actual), 0);
	ATF_REQUIRE(actual == 0xa5a5a5a5);
	ATF_REQUIRE(cell == 0xa5a5a5a5);
}
ATF_TC_CLEANUP(ucas_64_miscompare, tc)
{
	unload_module();
}
#endif /* _LP64 */

ATF_TC_WITH_CLEANUP(ucas_32_null);
ATF_TC_HEAD(ucas_32_null, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "test for correct ucas_32 NULL pointer behavior");
}
ATF_TC_BODY(ucas_32_null, tc)
{
	uint32_t actual = 0;

	CHECK_MODULE();

	ATF_REQUIRE_EQ(do_ucas_32(NULL, 0xdeadbeef, 0xbeefdead, &actual),
	    EFAULT);
}
ATF_TC_CLEANUP(ucas_32_null, tc)
{
	unload_module();
}

#ifdef _LP64
ATF_TC_WITH_CLEANUP(ucas_64_null);
ATF_TC_HEAD(ucas_64_null, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "test for correct ucas_64 NULL pointer behavior");
}
ATF_TC_BODY(ucas_64_null, tc)
{
	uint64_t actual = 0;

	CHECK_MODULE();

	ATF_REQUIRE_EQ(do_ucas_64(NULL, 0xdeadbeef, 0xbeefdead, &actual),
	    EFAULT);
}
ATF_TC_CLEANUP(ucas_64_null, tc)
{
	unload_module();
}
#endif /* _LP64 */

ATF_TC_WITH_CLEANUP(ucas_32_max);
ATF_TC_HEAD(ucas_32_max, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "test for correct ucas_32 VM_MAX_ADDRESS pointer behavior");
}
ATF_TC_BODY(ucas_32_max, tc)
{
	uint32_t actual = 0;

	CHECK_MODULE();

	ATF_REQUIRE_EQ(do_ucas_32(vm_max_address(), 0xdeadbeef, 0xbeefdead,
	    &actual), EFAULT);
}
ATF_TC_CLEANUP(ucas_32_max, tc)
{
	unload_module();
}

#ifdef _LP64
ATF_TC_WITH_CLEANUP(ucas_64_max);
ATF_TC_HEAD(ucas_64_max, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "test for correct ucas_64 VM_MAX_ADDRESS pointer behavior");
}
ATF_TC_BODY(ucas_64_max, tc)
{
	uint64_t actual = 0;

	CHECK_MODULE();

	ATF_REQUIRE_EQ(do_ucas_64(vm_max_address(), 0xdeadbeef, 0xbeefdead,
	    &actual), EFAULT);
}
ATF_TC_CLEANUP(ucas_64_max, tc)
{
	unload_module();
}
#endif /* _LP64 */

ATF_TC_WITH_CLEANUP(ucas_32_nearmax_overflow);
ATF_TC_HEAD(ucas_32_nearmax_overflow, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "test for correct ucas_32 near-VM_MAX_ADDRESS pointer behavior");
}
ATF_TC_BODY(ucas_32_nearmax_overflow, tc)
{
	uint32_t actual = 0;

	CHECK_MODULE();

	/*
	 * For no-strict-alignment platforms: address checks must return
	 * EFAULT.
	 *
	 * For strict-alignment platforms: alignment checks must return
	 * EFAULT.
	 */
	ATF_REQUIRE_EQ(do_ucas_32(vm_max_address_minus(3), 0xdeadbeef,
	    0xbeefdead, &actual), EFAULT);
}
ATF_TC_CLEANUP(ucas_32_nearmax_overflow, tc)
{
	unload_module();
}

#ifdef _LP64
ATF_TC_WITH_CLEANUP(ucas_64_nearmax_overflow);
ATF_TC_HEAD(ucas_64_nearmax_overflow, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "test for correct ucas_64 near-VM_MAX_ADDRESS pointer behavior");
}
ATF_TC_BODY(ucas_64_nearmax_overflow, tc)
{
	uint64_t actual = 0;

	CHECK_MODULE();

	/*
	 * For no-strict-alignment platforms: address checks must return
	 * EFAULT.
	 *
	 * For strict-alignment platforms: alignment checks must return
	 * EFAULT.
	 */
	ATF_REQUIRE_EQ(do_ucas_64(vm_max_address_minus(7), 0xdeadbeef,
	    0xbeefdead, &actual), EFAULT);
}
ATF_TC_CLEANUP(ucas_64_nearmax_overflow, tc)
{
	unload_module();
}
#endif /* _LP64 */

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, ufetch_8);
	ATF_TP_ADD_TC(tp, ufetch_16);
	ATF_TP_ADD_TC(tp, ufetch_32);
#ifdef _LP64
	ATF_TP_ADD_TC(tp, ufetch_64);
#endif

	ATF_TP_ADD_TC(tp, ufetch_8_null);
	ATF_TP_ADD_TC(tp, ufetch_16_null);
	ATF_TP_ADD_TC(tp, ufetch_32_null);
#ifdef _LP64
	ATF_TP_ADD_TC(tp, ufetch_64_null);
#endif

	ATF_TP_ADD_TC(tp, ufetch_8_max);
	ATF_TP_ADD_TC(tp, ufetch_16_max);
	ATF_TP_ADD_TC(tp, ufetch_32_max);
#ifdef _LP64
	ATF_TP_ADD_TC(tp, ufetch_64_max);
#endif

	ATF_TP_ADD_TC(tp, ufetch_16_nearmax_overflow);
	ATF_TP_ADD_TC(tp, ufetch_32_nearmax_overflow);
#ifdef _LP64
	ATF_TP_ADD_TC(tp, ufetch_64_nearmax_overflow);
#endif

	ATF_TP_ADD_TC(tp, ustore_8);
	ATF_TP_ADD_TC(tp, ustore_16);
	ATF_TP_ADD_TC(tp, ustore_32);
#ifdef _LP64
	ATF_TP_ADD_TC(tp, ustore_64);
#endif

	ATF_TP_ADD_TC(tp, ustore_8_null);
	ATF_TP_ADD_TC(tp, ustore_16_null);
	ATF_TP_ADD_TC(tp, ustore_32_null);
#ifdef _LP64
	ATF_TP_ADD_TC(tp, ustore_64_null);
#endif

	ATF_TP_ADD_TC(tp, ustore_8_max);
	ATF_TP_ADD_TC(tp, ustore_16_max);
	ATF_TP_ADD_TC(tp, ustore_32_max);
#ifdef _LP64
	ATF_TP_ADD_TC(tp, ustore_64_max);
#endif

	ATF_TP_ADD_TC(tp, ustore_16_nearmax_overflow);
	ATF_TP_ADD_TC(tp, ustore_32_nearmax_overflow);
#ifdef _LP64
	ATF_TP_ADD_TC(tp, ustore_64_nearmax_overflow);
#endif

	ATF_TP_ADD_TC(tp, ucas_32);
#ifdef _LP64
	ATF_TP_ADD_TC(tp, ucas_64);
#endif

	ATF_TP_ADD_TC(tp, ucas_32_miscompare);
#ifdef _LP64
	ATF_TP_ADD_TC(tp, ucas_64_miscompare);
#endif

	ATF_TP_ADD_TC(tp, ucas_32_null);
#ifdef _LP64
	ATF_TP_ADD_TC(tp, ucas_64_null);
#endif

	ATF_TP_ADD_TC(tp, ucas_32_max);
#ifdef _LP64
	ATF_TP_ADD_TC(tp, ucas_64_max);
#endif

	ATF_TP_ADD_TC(tp, ucas_32_nearmax_overflow);
#ifdef _LP64
	ATF_TP_ADD_TC(tp, ucas_64_nearmax_overflow);
#endif

	return atf_no_error();
}
