/*	$NetBSD: dev-swap.c,v 1.1.1.1 2009/12/02 00:26:34 haad Exp $	*/

/*
 * Copyright (C) 2009 Red Hat, Inc. All rights reserved.
 *
 * This file is part of LVM2.
 *
 * This copyrighted material is made available to anyone wishing to use,
 * modify, copy, or redistribute it subject to the terms and conditions
 * of the GNU Lesser General Public License v.2.1.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "lib.h"
#include "metadata.h"
#include "xlate.h"
#include "filter.h"

#ifdef linux

#define MAX_PAGESIZE	(64 * 1024)
#define SIGNATURE_SIZE  10

static int
_swap_detect_signature(const char *buf)
{
	if (memcmp(buf, "SWAP-SPACE", 10) == 0 ||
            memcmp(buf, "SWAPSPACE2", 10) == 0)
		return 1;

	if (memcmp(buf, "S1SUSPEND", 9) == 0 ||
	    memcmp(buf, "S2SUSPEND", 9) == 0 ||
	    memcmp(buf, "ULSUSPEND", 9) == 0 ||
	    memcmp(buf, "\xed\xc3\x02\xe9\x98\x56\xe5\x0c", 8) == 0)
		return 1;

	return 0;
}

int dev_is_swap(struct device *dev, uint64_t *signature)
{
	char buf[10];
	uint64_t size;
	int page;

	if (!dev_get_size(dev, &size)) {
		stack;
		return -1;
	}

	if (!dev_open(dev)) {
		stack;
		return -1;
	}

	*signature = 0;

	for (page = 0x1000; page <= MAX_PAGESIZE; page <<= 1) {
		/*
		 * skip 32k pagesize since this does not seem to be supported
		 */
		if (page == 0x8000)
			continue;
		if (size < page)
			break;
		if (!dev_read(dev, page - SIGNATURE_SIZE,
			      SIGNATURE_SIZE, buf)) {
			stack;
			return -1;
		}
		if (_swap_detect_signature(buf)) {
			*signature = page - SIGNATURE_SIZE;
			break;
		}
	}

	if (!dev_close(dev))
		stack;

	if (*signature)
		return 1;

	return 0;
}

#endif
