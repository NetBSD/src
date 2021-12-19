/*	$NetBSD: io-mapping.h,v 1.13 2021/12/19 12:28:04 riastradh Exp $	*/

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
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

#ifndef _LINUX_IO_MAPPING_H_
#define _LINUX_IO_MAPPING_H_

#include <sys/types.h>

#include <sys/bus.h>

#define	bus_space_io_mapping_init_wc	linux_bus_space_io_mapping_init_wc
#define	bus_space_io_mapping_create_wc	linux_bus_space_io_mapping_create_wc
#define	io_mapping_fini			linux_io_mapping_fini
#define	io_mapping_free			linux_io_mapping_free
#define	io_mapping_map_wc		linux_io_mapping_map_wc
#define	io_mapping_unmap		linux_io_mapping_unmap
#define	io_mapping_map_atomic_wc	linux_io_mapping_map_atomic_wc
#define	io_mapping_unmap_atomic		linux_io_mapping_unmap_atomic

struct io_mapping {
	bus_space_tag_t		diom_bst;
	bus_addr_t		base; /* Linux API */
	bus_size_t		size; /* Linux API */
	vaddr_t			diom_va;
	bool			diom_atomic;
};

bool bus_space_io_mapping_init_wc(bus_space_tag_t, struct io_mapping *,
    bus_addr_t, bus_size_t);
void io_mapping_fini(struct io_mapping *);

struct io_mapping *bus_space_io_mapping_create_wc(bus_space_tag_t, bus_addr_t,
    bus_size_t);
void io_mapping_free(struct io_mapping *);

void *io_mapping_map_wc(struct io_mapping *, bus_addr_t, bus_size_t);
void io_mapping_unmap(struct io_mapping *, void *, bus_size_t);

void *io_mapping_map_atomic_wc(struct io_mapping *, bus_addr_t);
void io_mapping_unmap_atomic(struct io_mapping *, void *);

#endif  /* _LINUX_IO_MAPPING_H_ */
