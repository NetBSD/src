/*	$NetBSD: dklist.c,v 1.3 2001/09/29 17:04:10 jdolecek Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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
__RCSID("$NetBSD: dklist.c,v 1.3 2001/09/29 17:04:10 jdolecek Exp $");
#endif /* not lint */

#include <sys/types.h>
#include <sys/disk.h>
#include <sys/ioctl.h>

#include <dev/ic/mlxreg.h>
#include <dev/ic/mlxio.h>

#include <err.h>
#include <fcntl.h>
#include <kvm.h>
#include <limits.h>
#include <nlist.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#include "extern.h"

static SIMPLEQ_HEAD(, mlx_disk) mlx_disks;

static struct nlist namelist[] = {
#define X_DISK_COUNT	0
	{ "_disk_count" },	/* number of disks */
#define X_DISKLIST	1
	{ "_disklist" },	/* TAILQ of disks */
	{ NULL },
};

#define	KVM_ERROR(_string) {						\
	warnx("%s", (_string));						\
	errx(1, "%s", kvm_geterr(kd));					\
}

/*
 * Dereference the namelist pointer `v' and fill in the local copy 
 * 'p' which is of size 's'.
 */
#define deref_nl(kd, v, p, s)	\
    deref_kptr(kd, (void *)namelist[(v)].n_value, (p), (s));

static void	deref_kptr(kvm_t *, void *, void *, size_t);

void
mlx_disk_init(void)
{

	SIMPLEQ_INIT(&mlx_disks);
}

int
mlx_disk_empty(void)
{

	return (SIMPLEQ_FIRST(&mlx_disks) == NULL);
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

	if (name[0] != 'l' || name[1] != 'd' || !isdigit(name[2]))
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
	struct disklist_head disk_head;
	struct disk cur_disk, *dk;
        char errbuf[_POSIX2_LINE_MAX];
	char buf[12];
	int i, ndrives;
	kvm_t *kd;

	/* Open the kernel. */
        if ((kd = kvm_openfiles(nlistf, memf, NULL, O_RDONLY, errbuf)) == NULL)
		errx(1, "kvm_openfiles: %s", errbuf);

	/* Obtain the namelist symbols from the kernel. */
	if (kvm_nlist(kd, namelist))
		KVM_ERROR("kvm_nlist failed to read symbols.");

	/* Get the number of attached drives. */
	deref_nl(kd, X_DISK_COUNT, &ndrives, sizeof(ndrives));

	if (ndrives < 0)
		errx(EXIT_FAILURE, "invalid _disk_count %d.", ndrives);
	if (ndrives == 0)
		errx(EXIT_FAILURE, "no drives attached.");

	/* Get a pointer to the first disk. */
	deref_nl(kd, X_DISKLIST, &disk_head, sizeof(disk_head));
	dk = TAILQ_FIRST(&disk_head);

	/* Try to add each disk to the list. */
	for (i = 0; i < ndrives; i++) {
		deref_kptr(kd, dk, &cur_disk, sizeof(cur_disk));
		deref_kptr(kd, cur_disk.dk_name, buf, sizeof(buf));
		mlx_disk_add0(buf);
		dk = TAILQ_NEXT(&cur_disk, dk_link);
	}

	kvm_close(kd);
}

/*
 * Dereference the kernel pointer `kptr' and fill in the local copy pointed
 * to by `ptr'.  The storage space must be pre-allocated, and the size of
 * the copy passed in `len'.
 */
static void
deref_kptr(kvm_t *kd, void *kptr, void *ptr, size_t len)
{
	char buf[128];

	if (kvm_read(kd, (u_long)kptr, (char *)ptr, len) != len) {
		memset(buf, 0, sizeof(buf));
		snprintf(buf, sizeof buf, "can't dereference kptr 0x%lx",
		    (u_long)kptr);
		KVM_ERROR(buf);
	}
}
