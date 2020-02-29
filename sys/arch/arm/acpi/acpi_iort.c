/* $NetBSD: acpi_iort.c,v 1.1.10.1 2020/02/29 20:18:17 ad Exp $ */

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: acpi_iort.c,v 1.1.10.1 2020/02/29 20:18:17 ad Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>

#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>

#include <arm/acpi/acpi_iort.h>

static ACPI_IORT_ID_MAPPING *
acpi_iort_id_map(ACPI_IORT_NODE *node, uint32_t *id)
{
	ACPI_IORT_ID_MAPPING *map;
	uint32_t offset, n;

	offset = node->MappingOffset;
	for (n = 0; n < node->MappingCount; n++) {
		map = ACPI_ADD_PTR(ACPI_IORT_ID_MAPPING, node, offset);
		if (map->Flags & ACPI_IORT_ID_SINGLE_MAPPING) {
			*id = map->OutputBase;
			return map;
		}
		if (*id >= map->InputBase && *id <= map->InputBase + map->IdCount) {
			*id = *id - map->InputBase + map->OutputBase;
			return map;
		}
		offset += sizeof(ACPI_IORT_ID_MAPPING);
	}

	return NULL;
}

static ACPI_IORT_NODE *
acpi_iort_find_ref(ACPI_TABLE_IORT *iort, ACPI_IORT_NODE *node, uint32_t *id)
{
	ACPI_IORT_ID_MAPPING *map;

	map = acpi_iort_id_map(node, id);
	if (map == NULL)
		return NULL;

	return ACPI_ADD_PTR(ACPI_IORT_NODE, iort, map->OutputReference);
}

uint32_t
acpi_iort_pci_root_map(u_int seg, uint32_t devid)
{
	ACPI_TABLE_IORT *iort;
	ACPI_IORT_NODE *node;
	ACPI_IORT_ROOT_COMPLEX *root;
	uint32_t offset, n;
	ACPI_STATUS rv;

	rv = AcpiGetTable(ACPI_SIG_IORT, 0, (ACPI_TABLE_HEADER **)&iort);
	if (ACPI_FAILURE(rv))
		return devid;

	offset = iort->NodeOffset;
	for (n = 0; n < iort->NodeCount; n++) {
		node = ACPI_ADD_PTR(ACPI_IORT_NODE, iort, offset);
		if (node->Type == ACPI_IORT_NODE_PCI_ROOT_COMPLEX) {
			root = (ACPI_IORT_ROOT_COMPLEX *)node->NodeData;
			if (root->PciSegmentNumber == seg) {
				const uint32_t odevid = devid;
				do {
					node = acpi_iort_find_ref(iort, node, &devid);
				} while (node != NULL);
				aprint_debug("ACPI: IORT mapped devid %#x -> devid %#x\n", odevid, devid);
				return devid;
			}
		}
		offset += node->Length;
	}

	return devid;
}

uint32_t
acpi_iort_its_id_map(u_int seg, uint32_t devid)
{
	ACPI_TABLE_IORT *iort;
	ACPI_IORT_NODE *node;
	ACPI_IORT_ROOT_COMPLEX *root;
	ACPI_IORT_ITS_GROUP *its_group;
	uint32_t offset, n;
	ACPI_STATUS rv;

	rv = AcpiGetTable(ACPI_SIG_IORT, 0, (ACPI_TABLE_HEADER **)&iort);
	if (ACPI_FAILURE(rv))
		return 0;

	offset = iort->NodeOffset;
	for (n = 0; n < iort->NodeCount; n++) {
		node = ACPI_ADD_PTR(ACPI_IORT_NODE, iort, offset);
		if (node->Type == ACPI_IORT_NODE_PCI_ROOT_COMPLEX) {
			root = (ACPI_IORT_ROOT_COMPLEX *)node->NodeData;
			if (root->PciSegmentNumber == seg) {
				const uint32_t odevid = devid;
				do {
					node = acpi_iort_find_ref(iort, node, &devid);
					if (node != NULL && node->Type == ACPI_IORT_NODE_ITS_GROUP) {
						its_group = (ACPI_IORT_ITS_GROUP *)node->NodeData;
						if (its_group->ItsCount == 0)
							return 0;
						aprint_debug("ACPI: IORT mapped devid %#x -> ITS %#x\n",
						    odevid, its_group->Identifiers[0]);
						return its_group->Identifiers[0];
					}
				} while (node != NULL);
				return 0;
			}
		}
		offset += node->Length;
	}

	return 0;
}
