/*	$NetBSD: fdt_ddb.c,v 1.2 2021/03/06 13:21:26 skrll Exp $	*/

/*-
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nick Hudson
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
__KERNEL_RCSID(0, "$NetBSD: fdt_ddb.c,v 1.2 2021/03/06 13:21:26 skrll Exp $");

#include <sys/param.h>

#include <libfdt.h>
#include <dev/fdt/fdt_ddb.h>
#include <dev/fdt/fdtvar.h>

#define FDT_MAX_DEPTH	16

static bool
fdt_isprint(const void *data, int len)
{
	const uint8_t *c = (const uint8_t *)data;

	if (len == 0)
		return false;

	/* Count consecutive zeroes */
	int cz = 0;
	for (size_t j = 0; j < len; j++) {
		if (c[j] == '\0')
			cz++;
		else if (isprint(c[j]))
			cz = 0;
		else
			return false;
		if (cz > 1)
			return false;
	}
	return true;
}

static void
fdt_print_properties(const void *fdt, int node,
    void (*pr)(const char *, ...) __printflike(1, 2))
{
	int property;

	fdt_for_each_property_offset(property, fdt, node) {
		int len;
		const struct fdt_property *prop =
		    fdt_get_property_by_offset(fdt, property, &len);
		const char *name = fdt_string(fdt, fdt32_to_cpu(prop->nameoff));

		pr("    %s", name);
		if (len == 0) {
			pr("\n");
			continue;
		}
		if (fdt_isprint(prop->data, len)) {
			const uint8_t *c = (const uint8_t *)prop->data;

			pr(" = \"");
			for (size_t j = 0; j < len; j++) {
				if (c[j] == '\0') {
					if (j + 1 != len)
						pr("\", \"");
				} else
					pr("%c", c[j]);
			}
			pr("\"\n");
			continue;
		}
		if ((len % 4) == 0) {
			const uint32_t *cell = (const uint32_t *)prop->data;
			size_t count = len / sizeof(uint32_t);

			pr(" = <");
			for (size_t j = 0; j < count; j++) {
				pr("%#" PRIx32 "%s", fdt32_to_cpu(cell[j]),
				    (j != count - 1) ? " " : "");
			}
			pr(">\n");
		} else {
			const uint8_t *byte = (const uint8_t *)prop->data;

			pr(" = [");
			for (size_t j = 0; j < len; j++) {
				pr("%02x%s", byte[j],
				   (j != len - 1) ? " " : "");
			}
			pr("]\n");
		}
	}
}


void
fdt_print(const void *addr, bool full,
    void (*pr)(const char *, ...) __printflike(1, 2))
{
	const void *fdt = addr;
	const char *pname[FDT_MAX_DEPTH] = { NULL };

	int error = fdt_check_header(fdt);
	if (error) {
		pr("Invalid FDT at %p\n", fdt);
		return;
	}

	int depth = 0;
	for (int node = fdt_path_offset(fdt, "/");
	     node >= 0 && depth >= 0;
	     node = fdt_next_node(fdt, node, &depth)) {
		const char *name = fdt_get_name(fdt, node, NULL);

		if (depth > FDT_MAX_DEPTH) {
			pr("max depth exceeded: %d\n", depth);
			continue;
		}
		pname[depth] = name;
		/*
		 * change conditional for when alternative root nodes
		 * can be specified
		 */
		if (depth == 0)
			pr("/");
		for (size_t i = 1; i <= depth; i++) {
			if (pname[i] == NULL)
				break;
			pr("/%s", pname[i]);
		}
		pr("\n");
		if (!full)
			continue;
		fdt_print_properties(fdt, node, pr);
	}
}
