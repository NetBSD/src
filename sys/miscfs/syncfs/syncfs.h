/*	$NetBSD: syncfs.h,v 1.3.4.1 2000/07/27 02:46:52 mycroft Exp $	*/

/*
 * Copyright 1997 Marshall Kirk McKusick. All Rights Reserved.
 *
 * This code is derived from work done by Greg Ganger at the
 * University of Michigan.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. None of the names of McKusick, Ganger, or the University of Michigan
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY MARSHALL KIRK MCKUSICK ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL MARSHALL KIRK MCKUSICK BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Routines to create and manage a filesystem syncer vnode.
 */
#define sync_close	genfs_nullop
int	sync_fsync 	__P((void *));
int	sync_inactive 	__P((void *));
int	sync_reclaim 	__P((void *));
#define sync_lock	genfs_nolock
#define sync_unlock	genfs_nounlock
int	sync_print	__P((void *));
#define sync_islocked	genfs_noislocked

void sched_sync __P((void *));
void vn_initialize_syncerd __P((void));
int vfs_allocate_syncvnode __P((struct mount *));
void vfs_deallocate_syncvnode __P((struct mount *));

extern int (**sync_vnodeop_p) __P((void *));

#define SYNCER_MAXDELAY       32

extern int syncer_maxdelay;	/* maximum delay time */ 
extern struct lock syncer_lock;	/* lock to freeze syncer during unmount */
LIST_HEAD(synclist, vnode);
