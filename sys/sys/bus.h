/*	$NetBSD: bus.h,v 1.2.22.1 2010/05/30 05:18:08 rmind Exp $	*/

/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
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

#ifndef _SYS_BUS_H_
#define	_SYS_BUS_H_

#include <machine/bus.h>

struct bus_space_reservation;

typedef struct bus_space_reservation /* {
	bus_addr_t	sr_addr;
	bus_size_t	sr_size;
} */ bus_space_reservation_t;

enum bus_space_override_idx {
	  BUS_SPACE_OVERRIDE_SPACE_MAP		= __BIT(0)
	, BUS_SPACE_OVERRIDE_SPACE_UNMAP	= __BIT(1)
	, BUS_SPACE_OVERRIDE_SPACE_ALLOC	= __BIT(2)
	, BUS_SPACE_OVERRIDE_SPACE_FREE		= __BIT(3)
	, BUS_SPACE_OVERRIDE_SPACE_EXTEND	= __BIT(4)
	, BUS_SPACE_OVERRIDE_SPACE_TRIM		= __BIT(5)
};

/* Only add new members at the end of this struct! */
struct bus_space_overrides {
	int (*bs_space_map)(void *, bus_space_tag_t, bus_addr_t, bus_size_t,
	    int, bus_space_handle_t *);

	void (*bs_space_unmap)(void *, bus_space_tag_t, bus_space_handle_t,
	    bus_size_t);

	int (*bs_space_alloc)(void *, bus_space_tag_t, bus_addr_t, bus_addr_t,
	    bus_size_t, bus_size_t, bus_size_t, int, bus_addr_t *,
	    bus_space_handle_t *);

	void (*bs_space_free)(void *, bus_space_tag_t, bus_space_handle_t,
	    bus_size_t);

	int (*bs_space_reserve)(void *, bus_space_tag_t, bus_addr_t, bus_size_t,
	    bus_space_reservation_t *);

	void (*bus_space_release)(void *, bus_space_tag_t,
	    bus_space_reservation_t);

	int (*bs_space_extend)(void *, bus_space_tag_t, bus_space_reservation_t,
	    bus_size_t, bus_size_t);

	void (*bs_space_trim)(void *, bus_space_tag_t, bus_space_reservation_t,
	    bus_size_t, bus_size_t);
};

bool	bus_space_is_equal(bus_space_tag_t, bus_space_tag_t);
int	bus_space_tag_create(bus_space_tag_t, uint64_t,
	                     const struct bus_space_overrides *, void *,
	                     bus_space_tag_t *);
void	bus_space_tag_destroy(bus_space_tag_t);

/* Reserve a region of bus space.  Reserved bus space cannot be allocated
 * with bus_space_alloc().  Reserved space has not been bus_space_map()'d.
 */
int	bus_space_reserve(bus_space_tag_t, bus_addr_t, bus_size_t,
	                  bus_space_reservation_t *);

/* Cancel a reservation. */
void	bus_space_release(bus_space_tag_t, bus_space_reservation_t);

/* Extend a reservation to the left and/or to the right.  The extension
 * has not been bus_space_map()'d.
 */
int	bus_space_extend(bus_space_tag_t, bus_space_reservation_t, bus_size_t,
	                 bus_size_t);

/* Trim bus space from a reservation on the left and/or on the right. */
void	bus_space_trim(bus_space_tag_t, bus_space_reservation_t, bus_size_t,
	               bus_size_t);

#endif	/* _SYS_BUS_H_ */
