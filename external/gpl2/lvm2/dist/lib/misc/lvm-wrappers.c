/*	$NetBSD: lvm-wrappers.c,v 1.1.1.1.2.1 2009/05/13 18:52:43 jym Exp $	*/

/*
 * Copyright (C) 2006 Red Hat, Inc. All rights reserved.
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

#include <unistd.h>
#include <fcntl.h>

int lvm_getpagesize(void)
{
	return getpagesize();
}

int read_urandom(void *buf, size_t len)
{
	int fd;

	/* FIXME: we should stat here, and handle other cases */
	/* FIXME: use common _io() routine's open/read/close */
	if ((fd = open("/dev/urandom", O_RDONLY)) < 0) {
		log_sys_error("open", "read_urandom: /dev/urandom");
		return 0;
	}

	if (read(fd, buf, len) != (ssize_t) len) {
		log_sys_error("read", "read_urandom: /dev/urandom");
		if (close(fd))
			stack;
		return 0;
	}

	if (close(fd))
		stack;

	return 1;
}

