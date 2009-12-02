/*	$NetBSD: example_cmdlib.c,v 1.1.1.2 2009/12/02 00:26:03 haad Exp $	*/

/*
 * Copyright (C) 2004 Red Hat, Inc. All rights reserved.
 *
 * This file is part of LVM2.
 *
 * This copyrighted material is made available to anyone wishing to use,
 * modify, copy, or redistribute it subject to the terms and conditions
 * of the GNU General Public License v.2.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "lvm2cmd.h"

/* All output gets passed to this function line-by-line */
void test_log_fn(int level, int dm_errno, const char *file, int line,
		 const char *format)
{
	/* Extract and process output here rather than printing it */

	if (level != 4)
		return;

	printf("%s\n", format);
	return;
}

int main(int argc, char **argv)
{
	void *handle;
	int r;

	lvm2_log_fn(test_log_fn);

	handle = lvm2_init();

	lvm2_log_level(handle, 1);
	r = lvm2_run(handle, "vgs --noheadings vg1");

	/* More commands here */

	lvm2_exit(handle);

	return r;
}

