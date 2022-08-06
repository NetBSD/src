/* $NetBSD: fdt_memory.c,v 1.6 2022/08/06 20:16:42 ryo Exp $ */

/*-
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jared McNeill <jmcneill@invisible.ca>.
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

#include "opt_fdt.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: fdt_memory.c,v 1.6 2022/08/06 20:16:42 ryo Exp $");

#include <sys/param.h>
#include <sys/queue.h>

#include <libfdt.h>
#include <dev/fdt/fdtvar.h>
#include <dev/fdt/fdt_memory.h>

#ifndef FDT_MEMORY_RANGES
#define FDT_MEMORY_RANGES	256
#endif

struct fdt_memory_range {
	struct fdt_memory               mr_mem;
	bool                            mr_used;
	TAILQ_ENTRY(fdt_memory_range)   mr_list;
};

static TAILQ_HEAD(fdt_memory_rangehead, fdt_memory_range) fdt_memory_ranges =
    TAILQ_HEAD_INITIALIZER(fdt_memory_ranges);

static struct fdt_memory_range fdt_memory_range_pool[FDT_MEMORY_RANGES];

static struct fdt_memory_range *
fdt_memory_range_alloc(void)
{
	for (size_t n = 0; n < FDT_MEMORY_RANGES; n++)
		if (!fdt_memory_range_pool[n].mr_used) {
			fdt_memory_range_pool[n].mr_used = true;
			return &fdt_memory_range_pool[n];
		}

	printf("%s: no free memory ranges, increase FDT_MEMORY_RANGES!\n", __func__);
	return NULL;
}

static void
fdt_memory_range_free(struct fdt_memory_range *mr)
{
	mr->mr_used = false;
}

/*
 * Get all of physical memory, including holes.
 */
void
fdt_memory_get(uint64_t *pstart, uint64_t *pend)
{
	const int memory = OF_finddevice("/memory");
	uint64_t cur_addr, cur_size;
	int index, nadd;

	for (index = 0, nadd = 0;
	     fdtbus_get_reg64(memory, index, &cur_addr, &cur_size) == 0;
	     index++) {
		if (cur_size == 0)
			continue;
		fdt_memory_add_range(cur_addr, cur_size);

		if (nadd++ == 0) {
			*pstart = cur_addr;
			*pend = cur_addr + cur_size;
			continue;
		}
		if (cur_addr < *pstart)
			*pstart = cur_addr;
		if (cur_addr + cur_size > *pend)
			*pend = cur_addr + cur_size;
	}
	if (nadd == 0)
		panic("Cannot determine memory size");
}

/*
 * Exclude memory ranges from memory config from the device tree
 */
void
fdt_memory_remove_reserved(uint64_t min_addr, uint64_t max_addr)
{
	uint64_t lstart = 0, lend = 0;
	int index, error, phandle, child;

	const int num = fdt_num_mem_rsv(fdtbus_get_data());
	for (index = 0; index <= num; index++) {
		uint64_t addr, size;

		error = fdt_get_mem_rsv(fdtbus_get_data(), index,
		    &addr, &size);
		if (error != 0)
			continue;
		if (lstart <= addr && addr <= lend) {
			size -= (lend - addr);
			addr = lend;
		}
		if (size == 0)
			continue;
		if (addr + size <= min_addr)
			continue;
		if (addr >= max_addr)
			continue;
		if (addr < min_addr) {
			size -= (min_addr - addr);
			addr = min_addr;
		}
		if (addr + size > max_addr)
			size = max_addr - addr;
		fdt_memory_remove_range(addr, size);
		lstart = addr;
		lend = addr + size;
	}

	/*
	 * "no-map" ranges defined in the /reserved-memory node
	 * must also be excluded.
	 */
	phandle = OF_finddevice("/reserved-memory");
	if (phandle != -1) {
		for (child = OF_child(phandle); child; child = OF_peer(child)) {
			bus_addr_t addr;
			bus_size_t size;

			if (!of_hasprop(child, "no-map"))
				continue;

			if (fdtbus_get_reg(child, 0, &addr, &size) != 0)
				continue;
			if (size == 0)
				continue;
			fdt_memory_remove_range(addr, size);
		}
	}
}

void
fdt_memory_add_range(uint64_t start, uint64_t size)
{
	struct fdt_memory_range *mr, *prev, *cur, *tmp;
	bool inserted = false;

	mr = fdt_memory_range_alloc();
	if (mr == NULL)
		return;

	mr->mr_mem.start = start;
	mr->mr_mem.end = start + size;

	/*
	 * Add the new range to the list of sorted ranges.
	 */
	TAILQ_FOREACH(cur, &fdt_memory_ranges, mr_list)
		if (mr->mr_mem.start <= cur->mr_mem.start) {
			TAILQ_INSERT_BEFORE(cur, mr, mr_list);
			inserted = true;
			break;
		}
	if (!inserted)
		TAILQ_INSERT_TAIL(&fdt_memory_ranges, mr, mr_list);

	/*
	 * Remove overlaps.
	 */
	TAILQ_FOREACH_SAFE(mr, &fdt_memory_ranges, mr_list, tmp) {
		prev = TAILQ_PREV(mr, fdt_memory_rangehead, mr_list);
		if (prev && prev->mr_mem.end > mr->mr_mem.start) {
			mr->mr_mem.start = prev->mr_mem.end;
			if (mr->mr_mem.start >= mr->mr_mem.end) {
				TAILQ_REMOVE(&fdt_memory_ranges, mr, mr_list);
				fdt_memory_range_free(mr);
			}
		}
	}

	/*
	 * Combine adjacent ranges.
	 */
	TAILQ_FOREACH_SAFE(mr, &fdt_memory_ranges, mr_list, tmp) {
		prev = TAILQ_PREV(mr, fdt_memory_rangehead, mr_list);
		if (prev && prev->mr_mem.end == mr->mr_mem.start) {
			prev->mr_mem.end = mr->mr_mem.end;
			TAILQ_REMOVE(&fdt_memory_ranges, mr, mr_list);
			fdt_memory_range_free(mr);
		}
	}
}

void
fdt_memory_remove_range(uint64_t start, uint64_t size)
{
	struct fdt_memory_range *mr, *next, *tmp;
	const uint64_t end = start + size;

	TAILQ_FOREACH_SAFE(mr, &fdt_memory_ranges, mr_list, tmp) {
		if (start <= mr->mr_mem.start && end >= mr->mr_mem.end) {
			/*
			 * Removed range completely covers this range,
			 * just remove it.
			 */
			TAILQ_REMOVE(&fdt_memory_ranges, mr, mr_list);
			fdt_memory_range_free(mr);
		} else if (start > mr->mr_mem.start && end < mr->mr_mem.end) {
			/*
			 * Removed range is completely contained by this range,
			 * split it.
			 */
			next = fdt_memory_range_alloc();
			if (next == NULL)
				panic("fdt_memory_remove_range");
			next->mr_mem.start = end;
			next->mr_mem.end = mr->mr_mem.end;
			mr->mr_mem.end = start;
			TAILQ_INSERT_AFTER(&fdt_memory_ranges, mr, next, mr_list);
		} else if (start <= mr->mr_mem.start && end > mr->mr_mem.start && end < mr->mr_mem.end) {
			/*
			 * Partial overlap at the beginning of the range.
			 */
			mr->mr_mem.start = end;
		} else if (start > mr->mr_mem.start && start < mr->mr_mem.end && end >= mr->mr_mem.end) {
			/*
			 * Partial overlap at the end of the range.
			 */
			mr->mr_mem.end = start;
		}
		KASSERT(mr->mr_mem.start < mr->mr_mem.end);
	}
}

void
fdt_memory_foreach(void (*fn)(const struct fdt_memory *, void *), void *arg)
{
	struct fdt_memory_range *mr;

	TAILQ_FOREACH(mr, &fdt_memory_ranges, mr_list)
		fn(&mr->mr_mem, arg);
}
