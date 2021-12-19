/*	$NetBSD: ioport.h,v 1.7 2021/12/19 11:55:38 riastradh Exp $	*/

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

#ifndef _LINUX_IOPORT_H_
#define _LINUX_IOPORT_H_

#include <sys/types.h>
#include <sys/bus.h>

#define	IORESOURCE_IO		__BIT(0)
#define	IORESOURCE_MEM		__BIT(1)
#define	IORESOURCE_IRQ		__BIT(2)
#define	IORESOURCE_UNSET	__BIT(3)

struct resource {
	bus_addr_t start;
	bus_addr_t end;		/* WARNING: Inclusive! */
	const char *name;
	unsigned int flags;
	bus_space_tag_t r_bst;	/* This milk is not organic.  */
	bus_space_handle_t r_bsh;
};

#define	DEFINE_RES_MEM(START, SIZE)					      \
	{ .start = (START), .end = (START) + ((SIZE) - 1) }

static inline bus_size_t
resource_size(struct resource *resource)
{
	return resource->end - resource->start + 1;
}

static inline bool
resource_contains(struct resource *r1, struct resource *r2)
{
	if (r1->r_bst != r2->r_bst)
		return false;
	return r1->start <= r2->start && r2->end <= r1->end;
}

static inline void
release_resource(struct resource *resource)
{
	bus_space_free(resource->r_bst, resource->r_bsh,
	    resource_size(resource));
}

#endif  /* _LINUX_IOPORT_H_ */
