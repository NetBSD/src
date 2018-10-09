/*	$NetBSD: fstrans.h,v 1.10.60.2 2018/10/09 09:58:09 martin Exp $	*/

/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Juergen Hannken-Illjes.
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
 * File system transaction operations.
 */

#ifndef _SYS_FSTRANS_H_
#define	_SYS_FSTRANS_H_

#include <sys/mount.h>

#define SUSPEND_SUSPEND	0x0001		/* VFS_SUSPENDCTL: suspend */
#define SUSPEND_RESUME	0x0002		/* VFS_SUSPENDCTL: resume */

enum fstrans_state {
	FSTRANS_NORMAL,
	FSTRANS_SUSPENDED,
	FSTRANS_SUSPENDING
};

void	fstrans_init(void);
void	fstrans_start(struct mount *);
int	fstrans_start_nowait(struct mount *);
void	fstrans_start_lazy(struct mount *);
void	fstrans_done(struct mount *);
int	fstrans_is_owner(struct mount *);
int	fstrans_mount(struct mount *);
void	fstrans_unmount(struct mount *);

int	fstrans_setstate(struct mount *, enum fstrans_state);
enum fstrans_state fstrans_getstate(struct mount *);

int	fscow_establish(struct mount *, int (*)(void *, struct buf *, bool),
	    void *);
int	fscow_disestablish(struct mount *, int (*)(void *, struct buf *, bool),
	    void *);
int	fscow_run(struct buf *, bool);

int	vfs_suspend(struct mount *, int);
void	vfs_resume(struct mount *);

#endif /* _SYS_FSTRANS_H_ */
