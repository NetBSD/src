/*	$NetBSD: bus_space_hpcmips.h,v 1.5 2005/12/11 12:17:33 christos Exp $	*/

/*-
 * Copyright (c) 2001 TAKEMRUA Shin. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#ifndef _BUS_SPACE_HPCMIPS_H_
#define _BUS_SPACE_HPCMIPS_H_

/*
 *	bus_space_tag
 *
 *	bus space tag structure
 */
struct bus_space_tag_hpcmips {
	struct bus_space_tag	bst;
	char			name[16];	/* bus name */
	u_int32_t		base;		/* extent base */
	u_int32_t		size;		/* extent size */
	void			*extent;	/* pointer for extent structure */
};

/*
 * Hpcmips unique methods
 */
bus_space_tag_t hpcmips_system_bus_space(void);
struct bus_space_tag_hpcmips *hpcmips_system_bus_space_hpcmips(void);
void hpcmips_init_bus_space(struct bus_space_tag_hpcmips *,
    struct bus_space_tag_hpcmips *, const char *, u_int32_t, u_int32_t);
struct bus_space_tag_hpcmips *hpcmips_alloc_bus_space_tag(void);

#endif /* _BUS_SPACE_HPCMIPS_H_ */
