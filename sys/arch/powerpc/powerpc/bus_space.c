/*	$NetBSD: bus_space.c,v 1.34.2.2 2014/08/20 00:03:20 tls Exp $	*/

/*-
 * Copyright (c) 1996, 1997, 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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
__KERNEL_RCSID(0, "$NetBSD: bus_space.c,v 1.34.2.2 2014/08/20 00:03:20 tls Exp $");

#define _POWERPC_BUS_SPACE_PRIVATE

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/endian.h>
#include <sys/extent.h>
#include <sys/bus.h>

#include <uvm/uvm.h>

#if defined (PPC_OEA) || defined(PPC_OEA64) || defined (PPC_OEA64_BRIDGE)
#include <powerpc/spr.h>
#include <powerpc/oea/bat.h>
#include <powerpc/oea/cpufeat.h>
#include <powerpc/oea/pte.h>
#include <powerpc/oea/spr.h>
#include <powerpc/oea/sr_601.h>
#endif

/* read_N */
u_int8_t bsr1(bus_space_tag_t, bus_space_handle_t, bus_size_t);
u_int16_t bsr2(bus_space_tag_t, bus_space_handle_t, bus_size_t);
u_int32_t bsr4(bus_space_tag_t, bus_space_handle_t, bus_size_t);
u_int64_t bsr8(bus_space_tag_t, bus_space_handle_t, bus_size_t);

/* write_N */
void bsw1(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int8_t);
void bsw2(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int16_t);
void bsw4(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int32_t);
void bsw8(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int64_t);

static const struct powerpc_bus_space_scalar scalar_ops = {
	bsr1, bsr2, bsr4, bsr8,
	bsw1, bsw2, bsw4, bsw8
};

/* read_N byte reverse */
u_int16_t bsr2rb(bus_space_tag_t, bus_space_handle_t, bus_size_t);
u_int32_t bsr4rb(bus_space_tag_t, bus_space_handle_t, bus_size_t);
u_int64_t bsr8rb(bus_space_tag_t, bus_space_handle_t, bus_size_t);

/* write_N byte reverse */
void bsw2rb(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int16_t);
void bsw4rb(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int32_t);
void bsw8rb(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int64_t);

static const struct powerpc_bus_space_scalar scalar_rb_ops = {
	bsr1, bsr2rb, bsr4rb, bsr8rb,
	bsw1, bsw2rb, bsw4rb, bsw8rb
};

/* read_multi_N */
void bsrm1(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int8_t *,
	size_t);
void bsrm2(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int16_t *,
	size_t);
void bsrm4(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int32_t *,
	size_t);
void bsrm8(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int64_t *,
	size_t);

/* write_multi_N */
void bswm1(bus_space_tag_t, bus_space_handle_t, bus_size_t,
	const u_int8_t *, size_t);
void bswm2(bus_space_tag_t, bus_space_handle_t, bus_size_t,
	const u_int16_t *, size_t);
void bswm4(bus_space_tag_t, bus_space_handle_t, bus_size_t,
	const u_int32_t *, size_t);
void bswm8(bus_space_tag_t, bus_space_handle_t, bus_size_t,
	const u_int64_t *, size_t);

static const struct powerpc_bus_space_group multi_ops = {
	bsrm1, bsrm2, bsrm4, bsrm8,
	bswm1, bswm2, bswm4, bswm8
};

/* read_multi_N byte reversed */
void bsrm2rb(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int16_t *,
	size_t);
void bsrm4rb(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int32_t *,
	size_t);
void bsrm8rb(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int64_t *,
	size_t);

/* write_multi_N byte reversed */
void bswm2rb(bus_space_tag_t, bus_space_handle_t, bus_size_t,
	const u_int16_t *, size_t);
void bswm4rb(bus_space_tag_t, bus_space_handle_t, bus_size_t,
	const u_int32_t *, size_t);
void bswm8rb(bus_space_tag_t, bus_space_handle_t, bus_size_t,
	const u_int64_t *, size_t);

static const struct powerpc_bus_space_group multi_rb_ops = {
	bsrm1, bsrm2rb, bsrm4rb, bsrm8rb,
	bswm1, bswm2rb, bswm4rb, bswm8rb
};

/* read_region_N */
void bsrr1(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int8_t *,
	size_t);
void bsrr2(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int16_t *,
	size_t);
void bsrr4(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int32_t *,
	size_t);
void bsrr8(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int64_t *,
	size_t);

/* write_region_N */
void bswr1(bus_space_tag_t, bus_space_handle_t, bus_size_t,
	const u_int8_t *, size_t);
void bswr2(bus_space_tag_t, bus_space_handle_t, bus_size_t,
	const u_int16_t *, size_t);
void bswr4(bus_space_tag_t, bus_space_handle_t, bus_size_t,
	const u_int32_t *, size_t);
void bswr8(bus_space_tag_t, bus_space_handle_t, bus_size_t,
	const u_int64_t *, size_t);

static const struct powerpc_bus_space_group region_ops = {
	bsrr1, bsrr2, bsrr4, bsrr8,
	bswr1, bswr2, bswr4, bswr8
};

void bsrr2rb(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int16_t *,
	size_t);
void bsrr4rb(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int32_t *,
	size_t);
void bsrr8rb(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int64_t *,
	size_t);

void bswr2rb(bus_space_tag_t, bus_space_handle_t, bus_size_t,
	const u_int16_t *, size_t);
void bswr4rb(bus_space_tag_t, bus_space_handle_t, bus_size_t,
	const u_int32_t *, size_t);
void bswr8rb(bus_space_tag_t, bus_space_handle_t, bus_size_t,
	const u_int64_t *, size_t);

static const struct powerpc_bus_space_group region_rb_ops = {
	bsrr1, bsrr2rb, bsrr4rb, bsrr8rb,
	bswr1, bswr2rb, bswr4rb, bswr8rb
};

/* set_region_n */
void bssr1(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int8_t,
	size_t);
void bssr2(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int16_t,
	size_t);
void bssr4(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int32_t,
	size_t);
void bssr8(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int64_t,
	size_t);

static const struct powerpc_bus_space_set set_ops = {
	bssr1, bssr2, bssr4, bssr8,
};

void bssr2rb(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int16_t,
	size_t);
void bssr4rb(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int32_t,
	size_t);
void bssr8rb(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int64_t,
	size_t);

static const struct powerpc_bus_space_set set_rb_ops = {
	bssr1, bssr2rb, bssr4rb, bssr8rb,
};

/* copy_region_N */
void bscr1(bus_space_tag_t, bus_space_handle_t,
    bus_size_t, bus_space_handle_t, bus_size_t, size_t);
void bscr2(bus_space_tag_t, bus_space_handle_t,
    bus_size_t, bus_space_handle_t, bus_size_t, size_t);
void bscr4(bus_space_tag_t, bus_space_handle_t,
    bus_size_t, bus_space_handle_t, bus_size_t, size_t);
void bscr8(bus_space_tag_t, bus_space_handle_t,
    bus_size_t, bus_space_handle_t, bus_size_t, size_t);

static const struct powerpc_bus_space_copy copy_ops = {
	bscr1, bscr2, bscr4, bscr8
};

/*
 * Strided versions
 */
/* read_N */
u_int8_t bsr1_s(bus_space_tag_t, bus_space_handle_t, bus_size_t);
u_int16_t bsr2_s(bus_space_tag_t, bus_space_handle_t, bus_size_t);
u_int32_t bsr4_s(bus_space_tag_t, bus_space_handle_t, bus_size_t);
u_int64_t bsr8_s(bus_space_tag_t, bus_space_handle_t, bus_size_t);

/* write_N */
void bsw1_s(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int8_t);
void bsw2_s(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int16_t);
void bsw4_s(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int32_t);
void bsw8_s(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int64_t);

static const struct powerpc_bus_space_scalar scalar_strided_ops = {
	bsr1_s, bsr2_s, bsr4_s, bsr8_s,
	bsw1_s, bsw2_s, bsw4_s, bsw8_s
};

/* read_N */
u_int16_t bsr2rb_s(bus_space_tag_t, bus_space_handle_t, bus_size_t);
u_int32_t bsr4rb_s(bus_space_tag_t, bus_space_handle_t, bus_size_t);
u_int64_t bsr8rb_s(bus_space_tag_t, bus_space_handle_t, bus_size_t);

/* write_N */
void bsw2rb_s(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int16_t);
void bsw4rb_s(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int32_t);
void bsw8rb_s(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int64_t);

static const struct powerpc_bus_space_scalar scalar_rb_strided_ops = {
	bsr1_s, bsr2rb_s, bsr4rb_s, bsr8rb_s,
	bsw1_s, bsw2rb_s, bsw4rb_s, bsw8rb_s
};

/* read_multi_N */
void bsrm1_s(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int8_t *,
	size_t);
void bsrm2_s(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int16_t *,
	size_t);
void bsrm4_s(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int32_t *,
	size_t);
void bsrm8_s(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int64_t *,
	size_t);

/* write_multi_N */
void bswm1_s(bus_space_tag_t, bus_space_handle_t, bus_size_t,
	const u_int8_t *, size_t);
void bswm2_s(bus_space_tag_t, bus_space_handle_t, bus_size_t,
	const u_int16_t *, size_t);
void bswm4_s(bus_space_tag_t, bus_space_handle_t, bus_size_t,
	const u_int32_t *, size_t);
void bswm8_s(bus_space_tag_t, bus_space_handle_t, bus_size_t,
	const u_int64_t *, size_t);

static const struct powerpc_bus_space_group multi_strided_ops = {
	bsrm1_s, bsrm2_s, bsrm4_s, bsrm8_s,
	bswm1_s, bswm2_s, bswm4_s, bswm8_s
};

/* read_multi_N */
void bsrm2rb_s(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int16_t *,
	size_t);
void bsrm4rb_s(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int32_t *,
	size_t);
void bsrm8rb_s(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int64_t *,
	size_t);

/* write_multi_N */
void bswm2rb_s(bus_space_tag_t, bus_space_handle_t, bus_size_t,
	const u_int16_t *, size_t);
void bswm4rb_s(bus_space_tag_t, bus_space_handle_t, bus_size_t,
	const u_int32_t *, size_t);
void bswm8rb_s(bus_space_tag_t, bus_space_handle_t, bus_size_t,
	const u_int64_t *, size_t);

static const struct powerpc_bus_space_group multi_rb_strided_ops = {
	bsrm1_s, bsrm2rb_s, bsrm4rb_s, bsrm8rb_s,
	bswm1_s, bswm2rb_s, bswm4rb_s, bswm8rb_s
};

/* read_region_N */
void bsrr1_s(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int8_t *,
	size_t);
void bsrr2_s(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int16_t *,
	size_t);
void bsrr4_s(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int32_t *,
	size_t);
void bsrr8_s(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int64_t *,
	size_t);

/* write_region_N */
void bswr1_s(bus_space_tag_t, bus_space_handle_t, bus_size_t,
	const u_int8_t *, size_t);
void bswr2_s(bus_space_tag_t, bus_space_handle_t, bus_size_t,
	const u_int16_t *, size_t);
void bswr4_s(bus_space_tag_t, bus_space_handle_t, bus_size_t,
	const u_int32_t *, size_t);
void bswr8_s(bus_space_tag_t, bus_space_handle_t, bus_size_t,
	const u_int64_t *, size_t);

static const struct powerpc_bus_space_group region_strided_ops = {
	bsrr1_s, bsrr2_s, bsrr4_s, bsrr8_s,
	bswr1_s, bswr2_s, bswr4_s, bswr8_s
};

/* read_region_N */
void bsrr2rb_s(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int16_t *,
	size_t);
void bsrr4rb_s(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int32_t *,
	size_t);
void bsrr8rb_s(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int64_t *,
	size_t);

/* write_region_N */
void bswr2rb_s(bus_space_tag_t, bus_space_handle_t, bus_size_t,
	const u_int16_t *, size_t);
void bswr4rb_s(bus_space_tag_t, bus_space_handle_t, bus_size_t,
	const u_int32_t *, size_t);
void bswr8rb_s(bus_space_tag_t, bus_space_handle_t, bus_size_t,
	const u_int64_t *, size_t);

static const struct powerpc_bus_space_group region_rb_strided_ops = {
	bsrr1_s, bsrr2rb_s, bsrr4rb_s, bsrr8rb_s,
	bswr1_s, bswr2rb_s, bswr4rb_s, bswr8rb_s
};

/* set_region_N */
void bssr1_s(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int8_t,
	size_t);
void bssr2_s(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int16_t,
	size_t);
void bssr4_s(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int32_t,
	size_t);
void bssr8_s(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int64_t,
	size_t);

static const struct powerpc_bus_space_set set_strided_ops = {
	bssr1_s, bssr2_s, bssr4_s, bssr8_s,
};

/* set_region_N */
void bssr2rb_s(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int16_t,
	size_t);
void bssr4rb_s(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int32_t,
	size_t);
void bssr8rb_s(bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int64_t,
	size_t);

static const struct powerpc_bus_space_set set_rb_strided_ops = {
	bssr1_s, bssr2rb_s, bssr4rb_s, bssr8rb_s,
};

/* copy_region_N */
void bscr1_s(bus_space_tag_t, bus_space_handle_t, bus_size_t,
	bus_space_handle_t, bus_size_t, size_t);
void bscr2_s(bus_space_tag_t, bus_space_handle_t, bus_size_t,
	bus_space_handle_t, bus_size_t, size_t);
void bscr4_s(bus_space_tag_t, bus_space_handle_t, bus_size_t,
	bus_space_handle_t, bus_size_t, size_t);
void bscr8_s(bus_space_tag_t, bus_space_handle_t, bus_size_t,
	bus_space_handle_t, bus_size_t, size_t);

static const struct powerpc_bus_space_copy copy_strided_ops = {
	bscr1_s, bscr2_s, bscr4_s, bscr8_s
};

static paddr_t memio_mmap(bus_space_tag_t, bus_addr_t, off_t, int, int);
static int memio_map(bus_space_tag_t, bus_addr_t, bus_size_t, int,
	bus_space_handle_t *);
static int memio_subregion(bus_space_tag_t, bus_space_handle_t, bus_size_t,
	bus_size_t, bus_space_handle_t *);
static void memio_unmap(bus_space_tag_t, bus_space_handle_t, bus_size_t);
static int memio_alloc(bus_space_tag_t, bus_addr_t, bus_addr_t, bus_size_t,
	bus_size_t, bus_size_t, int, bus_addr_t *, bus_space_handle_t *);
static void memio_free(bus_space_tag_t, bus_space_handle_t, bus_size_t);

static int extent_flags;

int
bus_space_init(struct powerpc_bus_space *t, const char *extent_name,
	void *storage, size_t storage_size)
{
	if (t->pbs_extent == NULL && extent_name != NULL) {
		t->pbs_extent = extent_create(extent_name, t->pbs_base,
		    t->pbs_limit, storage, storage_size,
		    EX_NOCOALESCE|EX_NOWAIT);
		if (t->pbs_extent == NULL)
			return ENOMEM;
	}

	t->pbs_mmap = memio_mmap;
	t->pbs_map = memio_map;
	t->pbs_subregion = memio_subregion;
	t->pbs_unmap = memio_unmap;
	t->pbs_alloc = memio_alloc;
	t->pbs_free = memio_free;

	if (t->pbs_flags & _BUS_SPACE_STRIDE_MASK) {
		t->pbs_scalar_stream = scalar_strided_ops;
		t->pbs_multi_stream = &multi_strided_ops;
		t->pbs_region_stream = &region_strided_ops;
		t->pbs_set_stream = &set_strided_ops;
		t->pbs_copy = &copy_strided_ops;
	} else {
		t->pbs_scalar_stream = scalar_ops;
		t->pbs_multi_stream = &multi_ops;
		t->pbs_region_stream = &region_ops;
		t->pbs_set_stream = &set_ops;
		t->pbs_copy = &copy_ops;
	}

#if BYTE_ORDER == BIG_ENDIAN
	if (t->pbs_flags & _BUS_SPACE_BIG_ENDIAN) {
		if (t->pbs_flags & _BUS_SPACE_STRIDE_MASK) {
			t->pbs_scalar = scalar_strided_ops;
			t->pbs_multi = &multi_strided_ops;
			t->pbs_region = &region_strided_ops;
			t->pbs_set = &set_strided_ops;
		} else {
			t->pbs_scalar = scalar_ops;
			t->pbs_multi = &multi_ops;
			t->pbs_region = &region_ops;
			t->pbs_set = &set_ops;
		}
	} else {
		if (t->pbs_flags & _BUS_SPACE_STRIDE_MASK) {
			t->pbs_scalar = scalar_rb_strided_ops;
			t->pbs_multi = &multi_rb_strided_ops;
			t->pbs_region = &region_rb_strided_ops;
			t->pbs_set = &set_rb_strided_ops;
		} else {
			t->pbs_scalar = scalar_rb_ops;
			t->pbs_multi = &multi_rb_ops;
			t->pbs_region = &region_rb_ops;
			t->pbs_set = &set_rb_ops;
		}
	}
#else
	if (t->pbs_flags & _BUS_SPACE_LITTLE_ENDIAN) {
		if (t->pbs_flags & _BUS_SPACE_STRIDE_MASK) {
			t->pbs_scalar = scalar_strided_ops;
			t->pbs_multi = &multi_strided_ops;
			t->pbs_region = &region_strided_ops;
			t->pbs_set = &set_strided_ops;
		} else {
			t->pbs_scalar = scalar_ops;
			t->pbs_multi = &multi_ops;
			t->pbs_region = &region_ops;
			t->pbs_set = &set_ops;
		}
	} else {
		if (t->pbs_flags & _BUS_SPACE_STRIDE_MASK) {
			t->pbs_scalar = scalar_rb_strided_ops;
			t->pbs_multi = &multi_rb_strided_ops;
			t->pbs_region = &region_rb_strided_ops;
			t->pbs_set = &set_rb_strided_ops;
		} else {
			t->pbs_scalar = scalar_rb_ops;
			t->pbs_multi = &multi_rb_ops;
			t->pbs_region = &region_rb_ops;
			t->pbs_set = &set_rb_ops;
		}
	}
#endif
	return 0;
}

void
bus_space_mallocok(void)
{
	extent_flags = EX_MALLOCOK;
}

/* ARGSUSED */
paddr_t
memio_mmap(bus_space_tag_t t, bus_addr_t bpa, off_t offset, int prot, int flags)
{
	paddr_t ret;
	/* XXX what about stride? */
	ret = trunc_page(t->pbs_offset + bpa + offset);

#ifdef DEBUG
	if (ret == 0) {
		printf("%s: [%08x, %08x %08x] mmaps to 0?!\n", __func__,
		    (uint32_t)t->pbs_offset, (uint32_t)bpa, (uint32_t)offset);
		return -1;
	}
#endif

#ifdef POWERPC_MMAP_FLAG_MASK
	if (flags & BUS_SPACE_MAP_PREFETCHABLE)
		ret |= POWERPC_MMAP_FLAG_PREFETCHABLE;
	if (flags & BUS_SPACE_MAP_CACHEABLE)
		ret |= POWERPC_MMAP_FLAG_CACHEABLE;
#endif

	return ret;
}

int
memio_map(bus_space_tag_t t, bus_addr_t bpa, bus_size_t size, int flags,
	bus_space_handle_t *bshp)
{
	int error;
	paddr_t pa;

	size = _BUS_SPACE_STRIDE(t, size);
	bpa = _BUS_SPACE_STRIDE(t, bpa);

	if (t->pbs_limit != 0 && bpa + size - 1 > t->pbs_limit) {
#ifdef DEBUG
		printf("bus_space_map(%p[%x:%x], %#x, %#x) failed: EINVAL\n",
		    t, t->pbs_base, t->pbs_limit, bpa, size);
#endif
		return (EINVAL);
	}

	/*
	 * Can't map I/O space as linear.
	 */
	if ((flags & BUS_SPACE_MAP_LINEAR) &&
	    (t->pbs_flags & _BUS_SPACE_IO_TYPE)) {
		return (EOPNOTSUPP);
	}

	if (t->pbs_extent != NULL) {
#ifdef PPC_IBM4XX
		/*
		 * XXX: Temporary kludge.
		 * Don't bother checking the extent during very early bootstrap.
		 */
		if (extent_flags) {
#endif
			/*
			 * Before we go any further, let's make sure that this
			 * region is available.
			 */
			error = extent_alloc_region(t->pbs_extent, bpa, size,
			    EX_NOWAIT | extent_flags);
			if (error) {
#ifdef DEBUG
				printf("bus_space_map(%p[%x:%x], %#x, %#x)"
				    " failed: %d\n",
				    t, t->pbs_base, t->pbs_limit,
				    bpa, size, error);
#endif
				return (error);
			}
#ifdef PPC_IBM4XX
		}
#endif
	}

	pa = t->pbs_offset + bpa;
#if defined (PPC_OEA) || defined(PPC_OEA601)
#ifdef PPC_OEA601
	if ((mfpvr() >> 16) == MPC601) {
		/*
		 * Map via the MPC601's I/O segments
		 */
		register_t sr = iosrtable[pa >> ADDR_SR_SHFT];
		if (SR601_VALID_P(sr) && ((pa >> ADDR_SR_SHFT) ==
		    ((pa + size - 1) >> ADDR_SR_SHFT))) {
			*bshp = pa;
			return (0);
		}
	} else
#endif /* PPC_OEA601 */
	if ((oeacpufeat & OEACPU_NOBAT) == 0) {
		/*
		 * Let's try to BAT map this address if possible
		 * (note this assumes 1:1 VA:PA)
		 */
		register_t batu = battable[BAT_VA2IDX(pa)].batu;
		if (BAT_VALID_P(batu, 0) && BAT_VA_MATCH_P(batu, pa) &&
		    BAT_VA_MATCH_P(batu, pa + size - 1)) {
			*bshp = pa;
			return (0);
		}
	}
#endif /* defined (PPC_OEA) || defined(PPC_OEA601) */

	/*
	 * Map this into the kernel pmap.
	 */
	*bshp = (bus_space_handle_t) mapiodev(pa, size,
	    (flags & BUS_SPACE_MAP_PREFETCHABLE) != 0);
	if (*bshp == 0) {
		if (t->pbs_extent != NULL) {
			extent_free(t->pbs_extent, bpa, size,
			    EX_NOWAIT | extent_flags);
		}
#ifdef DEBUG
		printf("bus_space_map(%p[%x:%x], %#x, %#x) failed: ENOMEM\n",
		    t, t->pbs_base, t->pbs_limit, bpa, size);
#endif
		return (ENOMEM);
	}

	return (0);
}

int
memio_subregion(bus_space_tag_t t, bus_space_handle_t bsh, bus_size_t offset,
	bus_size_t size, bus_space_handle_t *bshp)
{
	*bshp = bsh + _BUS_SPACE_STRIDE(t, offset);
	return (0);
}

void
memio_unmap(bus_space_tag_t t, bus_space_handle_t bsh, bus_size_t size)
{
	bus_addr_t bpa;
	vaddr_t va = bsh;
	paddr_t pa;

	size = _BUS_SPACE_STRIDE(t, size);

#if defined (PPC_OEA) || defined(PPC_OEA601)
#ifdef PPC_OEA601
	if ((mfpvr() >> 16) == MPC601) {
		register_t sr = iosrtable[va >> ADDR_SR_SHFT];
		if (SR601_VALID_P(sr) && ((va >> ADDR_SR_SHFT) ==
		    ((va + size - 1) >> ADDR_SR_SHFT))) {
			pa = va;
			va = 0;
		} else {
			pmap_extract(pmap_kernel(), va, &pa);
		}
	} else
#endif /* PPC_OEA601 */
	if ((oeacpufeat & OEACPU_NOBAT) == 0) {
		register_t batu = battable[BAT_VA2IDX(va)].batu;
		if (BAT_VALID_P(batu, 0) && BAT_VA_MATCH_P(batu, va) &&
		    BAT_VA_MATCH_P(batu, va + size - 1)) {
			pa = va;
			va = 0;
		} else { 
			pmap_extract(pmap_kernel(), va, &pa);
		}
	} else
		pmap_extract(pmap_kernel(), va, &pa);
#else
	pmap_extract(pmap_kernel(), va, &pa);
#endif /* defined (PPC_OEA) || defined(PPC_OEA601) */
	bpa = pa - t->pbs_offset;

	if (t->pbs_extent != NULL
#ifdef PPC_IBM4XX
	    && extent_flags
#endif
	    && extent_free(t->pbs_extent, bpa, size,
			   EX_NOWAIT | extent_flags)) {
		printf("memio_unmap: %s 0x%lx, size 0x%lx\n",
		    (t->pbs_flags & _BUS_SPACE_IO_TYPE) ? "port" : "mem",
		    (unsigned long)bpa, (unsigned long)size);
		printf("memio_unmap: can't free region\n");
	}

	if (va)
		unmapiodev(va, size);
}

int
memio_alloc(bus_space_tag_t t, bus_addr_t rstart, bus_addr_t rend,
	bus_size_t size, bus_size_t alignment, bus_size_t boundary,
	int flags, bus_addr_t *bpap, bus_space_handle_t *bshp)
{
	u_long bpa;
	paddr_t pa;
	int error;

	size = _BUS_SPACE_STRIDE(t, size);
	rstart = _BUS_SPACE_STRIDE(t, rstart);

	if (t->pbs_extent == NULL)
		return ENOMEM;

	if (t->pbs_limit != 0 && rstart + size - 1 > t->pbs_limit) {
#ifdef DEBUG
		printf("%s(%p[%x:%x], %#x, %#x) failed: EINVAL\n",
		   __func__, t, t->pbs_base, t->pbs_limit, rstart, size);
#endif
		return (EINVAL);
	}

	/*
	 * Can't map I/O space as linear.
	 */
	if ((flags & BUS_SPACE_MAP_LINEAR) &&
	    (t->pbs_flags & _BUS_SPACE_IO_TYPE))
		return (EOPNOTSUPP);

	if (rstart < t->pbs_extent->ex_start || rend > t->pbs_extent->ex_end)
		panic("memio_alloc: bad region start/end");

	error = extent_alloc_subregion(t->pbs_extent, rstart, rend, size,
	    alignment, boundary, EX_FAST | EX_NOWAIT | extent_flags, &bpa);

	if (error)
		return (error);

	*bpap = bpa;
	pa = t->pbs_offset + bpa;
#if defined (PPC_OEA) || defined(PPC_OEA601)
#ifdef PPC_OEA601
	if ((mfpvr() >> 16) == MPC601) {
		register_t sr = iosrtable[pa >> ADDR_SR_SHFT];
		if (SR601_VALID_P(sr) && SR601_PA_MATCH_P(sr, pa) &&
		    SR601_PA_MATCH_P(sr, pa + size - 1)) {
			*bshp = pa;
			return (0);
		}
	} else
#endif /* PPC_OEA601 */
	if ((oeacpufeat & OEACPU_NOBAT) == 0) {
		register_t batu = battable[BAT_VA2IDX(pa)].batu;
		if (BAT_VALID_P(batu, 0) && BAT_VA_MATCH_P(batu, pa) &&
		    BAT_VA_MATCH_P(batu, pa + size - 1)) {
			*bshp = pa;
			return (0);
		}
	}
#endif /* defined (PPC_OEA) || defined(PPC_OEA601) */
	*bshp = (bus_space_handle_t) mapiodev(pa, size, false);
	if (*bshp == 0) {
		extent_free(t->pbs_extent, bpa, size, EX_NOWAIT | extent_flags);
		return (ENOMEM);
	}

	return (0);
}

void
memio_free(bus_space_tag_t t, bus_space_handle_t bsh, bus_size_t size)
{
	if (t->pbs_extent == NULL)
		return;

	/* memio_unmap() does all that we need to do. */
	memio_unmap(t, bsh, size);
}
