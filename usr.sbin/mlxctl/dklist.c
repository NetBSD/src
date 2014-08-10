/*	$NetBSD: dklist.c,v 1.9.22.1 2014/08/10 06:59:51 tls Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
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

/*
 * Copyright (c) 1996 John M. Vinopal
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed for the NetBSD Project
 *      by John M. Vinopal.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

#ifndef lint
#include <sys/cdefs.h>
__RCSID("$NetBSD: dklist.c,v 1.9.22.1 2014/08/10 06:59:51 tls Exp $");
#endif /* not lint */

#include <sys/types.h>
#include <sys/iostat.h>
#include <sys/ioctl.h>
#include <sys/sysctl.h>

#include <dev/ic/mlxreg.h>
#include <dev/ic/mlxio.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <util.h>

#include "extern.h"

static SIMPLEQ_HEAD(, mlx_disk) mlx_disks;

void
mlx_disk_init(void)
{

	SIMPLEQ_INIT(&mlx_disks);
}

int
mlx_disk_empty(void)
{

	return (SIMPLEQ_EMPTY(&mlx_disks));
}

void
mlx_disk_iterate(void (*func)(struct mlx_disk *))
{
	struct mlx_disk *md;

	SIMPLEQ_FOREACH(md, &mlx_disks, chain)
		(*func)(md);
}

static int
mlx_disk_add0(const char *name)
{
	struct mlx_disk *md;
	int unit;

	if (name[0] != 'l' || name[1] != 'd' || !isdigit((unsigned char)name[2]))
		return (-1);

	SIMPLEQ_FOREACH(md, &mlx_disks, chain)
		if (strcmp(md->name, name) == 0)
			return (0);

	unit = atoi(name + 2);
	if (ioctl(mlxfd, MLX_GET_SYSDRIVE, &unit) < 0)
		return (-1);

	if ((md = malloc(sizeof(*md))) == NULL)
		err(EXIT_FAILURE, "mlx_disk_add()");

	strlcpy(md->name, name, sizeof(md->name));
	md->hwunit = unit;
	SIMPLEQ_INSERT_TAIL(&mlx_disks, md, chain);
	return (0);
}

void
mlx_disk_add(const char *name)
{

	if (mlx_disk_add0(name) != 0)
		errx(EXIT_FAILURE, "%s is not attached to %s", name, mlxname);
}

void
mlx_disk_add_all(void)
{
	struct io_sysctl *data;
	size_t i, len;
	static const int mib[3] = { CTL_HW, HW_IOSTATS, sizeof(*data) };

	data = asysctl(mib, __arraycount(mib), &len);
	len /= sizeof(*data);

	if (data == NULL || len == 0)
		errx(EXIT_FAILURE, "no drives attached.");

	for (i = 0; i < len; ++i) {
		if (data[i].type == IOSTAT_DISK)
			mlx_disk_add0(data[i].name);
	}

	free(data);
}
