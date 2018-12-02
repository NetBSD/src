/*	$NetBSD: subr_kleak.c,v 1.1 2018/12/02 21:00:13 maxv Exp $	*/

/*
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Maxime Villard. Based on an idea developed by Maxime Villard and
 * Thomas Barabosch of Fraunhofer FKIE.
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
__KERNEL_RCSID(0, "$NetBSD: subr_kleak.c,v 1.1 2018/12/02 21:00:13 maxv Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/param.h>
#include <sys/conf.h>
#include <sys/systm.h>
#include <sys/kmem.h>
#include <sys/sysctl.h>
#include <sys/types.h>

#include <sys/ksyms.h>
#include <sys/callout.h>

#define __RET_ADDR	__builtin_return_address(0)

#undef copyout
#undef copyoutstr

static uintptr_t kleak_kernel_text __read_mostly;

static bool kleak_enabled = false;
static int kleak_nrounds = 1;	/* tunable [1:8] */

static bool dummy1, dummy2, dummy3;

static kmutex_t kleak_mtx __cacheline_aligned;

static uint8_t *kleak_buf;
static size_t kleak_buf_size;

static uint8_t kleak_pattern_list[8] = {
	154, 157, 221, 218, 159, 169, 167, 181
};
static size_t kleak_index;
static volatile uint8_t kleak_pattern_byte;

/* -------------------------------------------------------------------------- */

#define KLEAK_HIT_MAXPC		16
#define KLEAK_RESULT_MAXHIT	32

struct kleak_hit {
	size_t len;
	size_t leaked;
	size_t npc;
	uintptr_t pc[KLEAK_HIT_MAXPC];
};

struct kleak_result {
	size_t nhits;
	size_t nmiss;
	struct kleak_hit hits[KLEAK_RESULT_MAXHIT];
};

static struct kleak_result result;

/* The MD code. */
#include <machine/kleak.h>

/* -------------------------------------------------------------------------- */

static void
kleak_note(const void *pc, size_t len, size_t leaked)
{
	struct kleak_hit *hit;
	uint8_t *byte;
	size_t i, off;

	if ((uintptr_t)pc < kleak_kernel_text) {
		return;
	}
	off = ((uintptr_t)pc - kleak_kernel_text);
	if (__predict_false(off >= kleak_buf_size)) {
		return;
	}

	byte = kleak_buf + off;

	mutex_enter(&kleak_mtx);

	*byte |= __BIT(kleak_index);
	for (i = 0; i < kleak_nrounds; i++) {
		if (__predict_true((*byte & __BIT(i)) == 0))
			goto out;
	}

	*byte = 0;

	if (__predict_false(result.nhits == KLEAK_RESULT_MAXHIT)) {
		result.nmiss++;
		goto out;
	}

	hit = &result.hits[result.nhits++];
	hit->len = len;
	hit->leaked = leaked;
	kleak_md_unwind(hit);

out:
	mutex_exit(&kleak_mtx);
}

int
kleak_copyout(const void *kaddr, void *uaddr, size_t len)
{
	const uint8_t *ptr = (const uint8_t *)kaddr;
	size_t remain = len;
	size_t cnt = 0;

	if (!kleak_enabled) {
		goto out;
	}

	while (remain-- > 0) {
		if (__predict_false(*ptr == kleak_pattern_byte)) {
			cnt++;
		}
		ptr++;
	}

	if (__predict_false(cnt > 0)) {
		kleak_note(__RET_ADDR, len, cnt);
	}

out:
	return copyout(kaddr, uaddr, len);
}

int
kleak_copyoutstr(const void *kaddr, void *uaddr, size_t len, size_t *done)
{
	const uint8_t *ptr = (const uint8_t *)kaddr;
	size_t remain = len;
	size_t cnt = 0;

	if (!kleak_enabled) {
		goto out;
	}

	while (remain-- > 0) {
		if (*ptr == '\0') {
			break;
		}
		if (__predict_false(*ptr == kleak_pattern_byte)) {
			cnt++;
		}
		ptr++;
	}

	if (__predict_false(cnt > 0)) {
		kleak_note(__RET_ADDR, len, cnt);
	}

out:
	return copyoutstr(kaddr, uaddr, len, done);
}

void
kleak_fill_area(void *ptr, size_t len)
{
	memset(ptr, kleak_pattern_byte, len);
}

void
kleak_fill_stack(void)
{
	char buf[USPACE-(2*PAGE_SIZE)];
	explicit_memset(buf, kleak_pattern_byte, sizeof(buf));
}

void __sanitizer_cov_trace_pc(void);

/*
 * We want an explicit memset, but inlined. So use a builtin with optimization
 * disabled.
 */
void __attribute__((optimize("O0")))
__sanitizer_cov_trace_pc(void)
{
	char buf[512];
	__builtin_memset(buf, kleak_pattern_byte, sizeof(buf));
}

/* -------------------------------------------------------------------------- */

static void
kleak_init(void)
{
	uintptr_t sva, eva;

	kleak_md_init(&sva, &eva);
	kleak_kernel_text = sva;

	kleak_index = 0;
	kleak_pattern_byte = kleak_pattern_list[kleak_index];

	if (kleak_buf == NULL) {
		mutex_init(&kleak_mtx, MUTEX_DEFAULT, IPL_NONE);
		kleak_buf_size = (size_t)eva - (size_t)sva;
		kleak_buf = kmem_zalloc(kleak_buf_size, KM_SLEEP);
	} else {
		/* Already initialized, just reset. */
		mutex_enter(&kleak_mtx);
		memset(kleak_buf, 0, kleak_buf_size);
		mutex_exit(&kleak_mtx);
	}
}

static int
kleak_rotate(void)
{
	mutex_enter(&kleak_mtx);
	kleak_index++;
	mutex_exit(&kleak_mtx);

	/* XXX: Should be atomic. */
	kleak_pattern_byte = kleak_pattern_list[kleak_index];

	if (kleak_index >= kleak_nrounds) {
		return ENOENT;
	}

	return 0;
}

static int
sysctl_kleak_rounds(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	int error, val;

	val = *(int *)rnode->sysctl_data;

	node = *rnode;
	node.sysctl_data = &val;

	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error != 0 || newp == NULL)
		return error;
	if (val < 1 || val > 8)
		return EINVAL;
	if (kleak_enabled)
		return EINVAL;

	*(int *)rnode->sysctl_data = val;

	return 0;
}

static int
sysctl_kleak_patterns(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	uint8_t val[8];
	int error;

	memcpy(val, rnode->sysctl_data, sizeof(val));

	node = *rnode;
	node.sysctl_data = &val;

	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error != 0 || newp == NULL)
		return error;
	if (kleak_enabled)
		return EINVAL;

	memcpy(rnode->sysctl_data, val, 8 * sizeof(uint8_t));

	return 0;
}

static int
sysctl_kleak_start(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	int error;
	bool val;

	val = *(bool *)rnode->sysctl_data;

	node = *rnode;
	node.sysctl_data = &val;

	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error != 0 || newp == NULL)
		return error;
	if (kleak_enabled)
		return EEXIST;
	if (!val)
		return EINVAL;

	kleak_init();
	memset(&result, 0, sizeof(result));
	kleak_enabled = true;

	return 0;
}

static int
sysctl_kleak_rotate(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	int error;
	bool val;

	val = *(bool *)rnode->sysctl_data;

	node = *rnode;
	node.sysctl_data = &val;

	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error != 0 || newp == NULL)
		return error;
	if (!val)
		return EINVAL;

	return kleak_rotate();
}

static int
sysctl_kleak_stop(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	int error;
	bool val;

	val = *(bool *)rnode->sysctl_data;

	node = *rnode;
	node.sysctl_data = &val;

	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error != 0 || newp == NULL)
		return error;
	if (!val)
		return EINVAL;

	kleak_enabled = false;

	return 0;
}

static int
sysctl_kleak_result(SYSCTLFN_ARGS)
{
	struct sysctlnode node;

	node = *rnode;
	node.sysctl_data = &result;
	node.sysctl_size = sizeof(result);

	return sysctl_lookup(SYSCTLFN_CALL(&node));
}

SYSCTL_SETUP(sysctl_kleak_setup, "sysctl kleak subtree setup")
{
	const struct sysctlnode *kleak_rnode;

	kleak_rnode = NULL;

	sysctl_createv(clog, 0, NULL, &kleak_rnode,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "kleak", NULL,
		       NULL, 0, NULL, 0,
		       CTL_CREATE, CTL_EOL);

	sysctl_createv(clog, 0, &kleak_rnode, NULL,
		       CTLFLAG_READWRITE,
		       CTLTYPE_INT, "rounds",
		       SYSCTL_DESCR("Number of rounds"),
		       sysctl_kleak_rounds, 0, &kleak_nrounds, 0,
		       CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, &kleak_rnode, NULL,
		       CTLFLAG_READWRITE,
		       CTLTYPE_STRUCT, "patterns",
		       SYSCTL_DESCR("List of patterns"),
		       sysctl_kleak_patterns, 0, &kleak_pattern_list,
	               sizeof(kleak_pattern_list),
		       CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, &kleak_rnode, NULL,
		       CTLFLAG_READWRITE,
		       CTLTYPE_BOOL, "start",
		       SYSCTL_DESCR("Start KLEAK"),
		       sysctl_kleak_start, 0, &dummy1, 0,
		       CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, &kleak_rnode, NULL,
		       CTLFLAG_READWRITE,
		       CTLTYPE_BOOL, "rotate",
		       SYSCTL_DESCR("Rotate the pattern"),
		       sysctl_kleak_rotate, 0, &dummy2, 0,
		       CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, &kleak_rnode, NULL,
		       CTLFLAG_READWRITE,
		       CTLTYPE_BOOL, "stop",
		       SYSCTL_DESCR("Stop KLEAK"),
		       sysctl_kleak_stop, 0, &dummy3, 0,
		       CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, &kleak_rnode, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRUCT, "result",
		       SYSCTL_DESCR("Get the result"),
		       sysctl_kleak_result, 0, NULL, 0,
		       CTL_CREATE, CTL_EOL);
}
