/*	$NetBSD: vfs_hooks.c,v 1.3 2008/04/28 20:24:05 martin Exp $	*/

/*-
 * Copyright (c) 2005 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Julio M. Merino Vidal.
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
 * VFS hooks.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: vfs_hooks.c,v 1.3 2008/04/28 20:24:05 martin Exp $");

#include <sys/param.h>
#include <sys/mount.h>

/*
 * Static list of file system specific hook sets supported by the kernel.
 */
__link_set_decl(vfs_hooks, struct vfs_hooks);

/*
 * Initialize a VFS hook set that does nothing.  This is to ensure that
 * we have, at the very least, one item in the link set.  Otherwise,
 * ld(1) will complain.
 */
static struct vfs_hooks null_hooks = {
	NULL		/* vh_unmount */
};
VFS_HOOKS_ATTACH(null_hooks);

/*
 * Macro to be used in one of the vfs_hooks_* function for hooks that
 * return an error code.  Calls will stop as soon as one of the hooks
 * fails.
 */
#define VFS_HOOKS_W_ERROR(func, args)					\
	int error;							\
	struct vfs_hooks * const *hp;					\
 									\
	error = 0;							\
 									\
	__link_set_foreach(hp, vfs_hooks) {				\
		if ((*hp)-> func != NULL) {				\
			error = (*hp)-> func args;			\
			if (error != 0)					\
				break;					\
		}							\
	}								\
 									\
	return error;

/*
 * Macro to be used in one of the vfs_hooks_* function for hooks that
 * do not return any error code.  All hooks will be executed
 * unconditionally.
 */
#define VFS_HOOKS_WO_ERROR(func, fargs, hook, hargs)			\
void									\
func fargs								\
{									\
	struct vfs_hooks * const *hp;					\
 									\
	__link_set_foreach(hp, vfs_hooks) {				\
		if ((*hp)-> hook != NULL)				\
			(*hp)-> hook hargs;				\
	}								\
}

/*
 * Routines to iterate over VFS hooks lists and execute them.
 */

VFS_HOOKS_WO_ERROR(vfs_hooks_unmount, (struct mount *mp), vh_unmount, (mp));

/*
void
vfs_hooks_unmount(struct mount *mp)
{

	VFS_HOOKS_WO_ERROR(vh_unmount, (mp));
}
*/
