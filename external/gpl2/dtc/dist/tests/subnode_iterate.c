/*
 * libfdt - Flat Device Tree manipulation
 *	Tests that fdt_next_subnode() works as expected
 *
 * Copyright (C) 2013 Google, Inc
 *
 * Copyright (C) 2007 David Gibson, IBM Corporation.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <libfdt.h>

#include "tests.h"
#include "testdata.h"

static void test_node(void *fdt, int parent_offset)
{
	fdt32_t subnodes;
	const fdt32_t *prop;
	int offset;
	int count;
	int len;

	/* This property indicates the number of subnodes to expect */
	prop = fdt_getprop(fdt, parent_offset, "subnodes", &len);
	if (!prop || len != sizeof(fdt32_t)) {
		FAIL("Missing/invalid subnodes property at '%s'",
		     fdt_get_name(fdt, parent_offset, NULL));
	}
	subnodes = cpu_to_fdt32(*prop);

	count = 0;
	for (offset = fdt_first_subnode(fdt, parent_offset);
	     offset >= 0;
	     offset = fdt_next_subnode(fdt, offset))
		count++;

	if (count != subnodes) {
		FAIL("Node '%s': Expected %d subnodes, got %d\n",
		     fdt_get_name(fdt, parent_offset, NULL), subnodes,
		     count);
	}
}

static void check_fdt_next_subnode(void *fdt)
{
	int offset;
	int count = 0;

	for (offset = fdt_first_subnode(fdt, 0);
	     offset >= 0;
	     offset = fdt_next_subnode(fdt, offset)) {
		test_node(fdt, offset);
		count++;
	}

	if (count != 2)
		FAIL("Expected %d tests, got %d\n", 2, count);
}

int main(int argc, char *argv[])
{
	void *fdt;

	test_init(argc, argv);
	if (argc != 2)
		CONFIG("Usage: %s <dtb file>", argv[0]);

	fdt = load_blob(argv[1]);
	if (!fdt)
		FAIL("No device tree available");

	check_fdt_next_subnode(fdt);

	PASS();
}
