/*	$NetBSD: platform.c,v 1.1 2001/06/13 15:08:05 soda Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by SODA Noriyuki.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kcore.h>

#include <machine/platform.h>
#include <arc/arc/arcbios.h>

struct platform *platform = NULL;

void print_platform __P((struct platform *));

void
print_platform(p)
	struct platform *p;
{
	printf("\"%s %s%s\" (%s, %s)",
	    p->vendor, p->model, p->variant,
	    p->vendor_id ? p->vendor_id : "NULL",
	    p->system_id);
}

int
ident_platform()
{
	int i, rv, matched = -1, match = 0, ambiguous_match = 0;

	for (i = 0; i < nplattab; i++) {
		rv = (*plattab[i]->match)(plattab[i]);
		if (rv > match) {
			match = rv;
			matched = i;
			ambiguous_match = 0;
		} else if (rv == match) {
			++ambiguous_match;
		}
	}
	if (ambiguous_match) {
		/* assumes that ARC Firmware printf() can be used. */
		printf("ambiguous platform detection between ");
		print_platform(plattab[matched]);
		for (i = 0; i < nplattab; i++) {
			if (i == matched)
				continue;
			rv = (*plattab[i]->match)(plattab[i]);
			if (rv < match)
				continue;
			if (--ambiguous_match)
				printf(", ");
			else
				printf(" and ");
			print_platform(plattab[i]);
		}
		printf(". ");
		print_platform(plattab[matched]);
		printf(" is choosed.\n");
	}
	if (match)
		platform = plattab[matched];
	return (match);
}

int
platform_generic_match(p)
	struct platform *p;
{
	if (strcmp(arc_id, p->system_id) == 0 &&
	    (p->vendor_id == NULL || strcmp(arc_vendor_id, p->vendor_id) == 0))
		return (1);

	return (0);
}

void
platform_nop()
{
}
