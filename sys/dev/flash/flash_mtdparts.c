/* $NetBSD: flash_mtdparts.c,v 1.1.2.2 2017/12/03 11:37:01 jdolecek Exp $ */

/*-
 * Copyright (c) 2017 Jared McNeill <jmcneill@invisible.ca>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: flash_mtdparts.c,v 1.1.2.2 2017/12/03 11:37:01 jdolecek Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/kmem.h>

#include <dev/flash/flash.h>

extern int flash_print(void *, const char *);

static void
flash_attach_partition(struct flash_interface *flash_if, device_t parent,
    struct flash_partition *part)
{
	struct flash_attach_args faa;	

	faa.flash_if = flash_if;
	faa.partinfo = *part;

	config_found_ia(parent, "flashbus", &faa, flash_print);
}

static flash_size_t
flash_parse_size(char *partdef, char **ep)
{
	flash_size_t size;

	/* Parse the size parameter */
	size = strtoul(partdef, ep, 10);
	if (partdef == *ep)
		return 0;

	switch (**ep) {
	case 'G':
	case 'g':
		size <<= 10;
		/* FALLTHROUGH */
	case 'M':
	case 'm':
		size <<= 10;
		/* FALLTHROUGH */
	case 'K':
	case 'k':
		size <<= 10;
		(*ep)++;
		break;
	}

	return size;
}

static int
flash_parse_partdef(flash_size_t flash_size, char *partdef, flash_off_t *offset,
    struct flash_partition *ppart)
{
	struct flash_partition part;

	/* Get the partition size */
	if (*partdef == '-') {
		/* Use the remaining space */
		part.part_size = 0;
		partdef++;
	} else {
		part.part_size = flash_parse_size(partdef, &partdef);
		if (part.part_size == 0)
			return EINVAL;
	}

	if (*partdef == '@') {
		/* Explicit offset */
		partdef++;
		part.part_offset = flash_parse_size(partdef, &partdef);
	} else {
		/* Offset is the end of the previous partition */
		part.part_offset = *offset;
	}

	/* Calculate partition size for "all remaining space" parts */
	if (part.part_size == 0)
		part.part_size = flash_size - part.part_offset;

	if (*partdef == '(') {
		/* Partition name */
		partdef++;
		part.part_name = partdef;
		partdef = strchr(partdef, ')');
		if (partdef == NULL)
			return EINVAL;
		*partdef = '\0';
		partdef++;
	}

	part.part_flags = 0;
	if (strncmp(partdef, "ro", 2) == 0) {
		part.part_flags |= FLASH_PART_READONLY;
		partdef += 2;
	}

	*ppart = part;

	*offset = part.part_offset + part.part_size;

	return 0;
}

static void
flash_parse_mtddef(struct flash_interface *flash_if, device_t parent,
    flash_size_t flash_size, char *mtddef)
{
	struct flash_partition part;
	char *partdef = mtddef, *nextdef;
	flash_off_t offset = 0;
	int error;

	while (partdef && offset < flash_size) {
		/* Find the end */
		nextdef = strchr(partdef, ',');
		if (nextdef == NULL)
		    nextdef = strchr(partdef, ' ');
		if (nextdef)
			*nextdef++ = '\0';

		error = flash_parse_partdef(flash_size, partdef, &offset,
		    &part);
		if (error) {
			aprint_error_dev(parent, "bad partition def '%s'\n",
			    partdef);
			return;
		}

		flash_attach_partition(flash_if, parent, &part);

		partdef = nextdef;
	}
}

/*
 * Attach partitions to a given parent device node that match the supplied
 * device id. The cmdline follows the following format:
 *
 * mtdparts=<mtddef>[;<mtddef]
 * <mtddef>  := <mtd-id>:<partdef>[,<partdef>]
 * <partdef> := <size>[@offset][<name>][ro]
 * <mtd-id>  := unique id used in mapping driver/device (number of flash bank)
 * <size>    := memsize OR "-" to denote all remaining space
 * <name>    := '(' NAME ')'
 */
void
flash_attach_mtdparts(struct flash_interface *flash_if, device_t parent,
    flash_size_t flash_size, const char *mtd_id, const char *cmdline)
{
	char *mtddef;
	size_t mtddeflen;

	/* Find the definition for our mtd id */
	const char *s = strstr(cmdline, mtd_id);
	if (s == NULL || s[strlen(mtd_id)] != ':')
		return;

	mtddef = kmem_strdupsize(s + strlen(mtd_id) + 1, &mtddeflen, KM_SLEEP);

	flash_parse_mtddef(flash_if, parent, flash_size, mtddef);

	kmem_free(mtddef, mtddeflen);
}
