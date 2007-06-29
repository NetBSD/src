/*	$NetBSD: efs_ihash.h,v 1.1 2007/06/29 23:30:29 rumble Exp $	*/

/*
 * Copyright (c) 2006 Stephen M. Rumble <rumble@ephemeral.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef _FS_EFS_EFS_IHASH_H_
#define _FS_EFS_EFS_IHASH_H_

void		efs_ihashinit(void);
void		efs_ihashreinit(void);
void		efs_ihashdone(void);
struct vnode   *efs_ihashget(dev_t, ino_t, int);
void		efs_ihashins(struct efs_inode *);
void		efs_ihashrem(struct efs_inode *);
void		efs_ihashlock(void);
void		efs_ihashunlock(void);

#endif	/* !_FS_EFS_EFS_IHASH_H_ */
