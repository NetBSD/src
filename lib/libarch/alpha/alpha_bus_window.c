/*	$NetBSD: alpha_bus_window.c,v 1.2 2001/07/17 17:46:42 thorpej Exp $	*/

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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

/*
 * Support for mapping Alpha bus windows.  This is currently used to
 * provide bus space mapping support for XFree86.  In a perfect world,
 * this would go away in favor of a real bus space mapping framework.
 */

#include <sys/types.h>
#include <sys/mman.h>

#include <machine/sysarch.h>

#include <fcntl.h>
#include <paths.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int
alpha_bus_getwindows(int type, struct alpha_bus_window **abwp)
{
	struct alpha_bus_get_window_count_args count_args;
	struct alpha_bus_get_window_args window_args;
	struct alpha_bus_window *abw;
	int i;

	count_args.type = type;
	if (sysarch(ALPHA_BUS_GET_WINDOW_COUNT, &count_args) != 0)
		return (-1);

	abw = malloc(sizeof(*abw) * count_args.count);
	if (abw == NULL)
		return (-1);
	memset(abw, 0, sizeof(*abw) * count_args.count);

	for (i = 0; i < count_args.count; i++) {
		window_args.type = type;
		window_args.window = i;
		window_args.translation = &abw[i].abw_abst;
		if (sysarch(ALPHA_BUS_GET_WINDOW, &window_args) != 0) {
			free(abw);
			return (-1);
		}
	}

	*abwp = abw;
	return (count_args.count);
}

int
alpha_bus_mapwindow(struct alpha_bus_window *abw)
{
	struct alpha_bus_space_translation *abst = &abw->abw_abst;
	void *addr;
	size_t size;
	int fd;

	fd = open(_PATH_MEM, O_RDWR, 0600);
	if (fd == -1)
		return (-1);

	size = ((abst->abst_bus_end - abst->abst_bus_start) + 1) <<
	    abst->abst_addr_shift;

	addr = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_FILE|MAP_SHARED,
	    fd, (off_t) abst->abst_sys_start);

	(void) close(fd);

	if (addr == MAP_FAILED)
		return (-1);

	abw->abw_addr = addr;
	abw->abw_size = size;

	return (0);
}

void
alpha_bus_unmapwindow(struct alpha_bus_window *abw)
{

	(void) munmap(abw->abw_addr, abw->abw_size);
}
