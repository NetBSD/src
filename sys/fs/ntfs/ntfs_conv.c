/*	$NetBSD: ntfs_conv.c,v 1.5.4.2 2008/02/04 09:23:47 yamt Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
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
 * File name recode stuff.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ntfs_conv.c,v 1.5.4.2 2008/02/04 09:23:47 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/proc.h>
#include <sys/kernel.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/buf.h>
#include <sys/file.h>
#include <sys/malloc.h>
#include <sys/lock.h>

#include <miscfs/specfs/specdev.h>

#include <fs/ntfs/ntfs.h>
#include <fs/ntfs/ntfsmount.h>
#include <fs/ntfs/ntfs_inode.h>
#include <fs/ntfs/ntfs_vfsops.h>
#include <fs/ntfs/ntfs_subr.h>
#include <fs/ntfs/ntfs_compr.h>
#include <fs/ntfs/ntfs_ihash.h>

/* UTF-8 encoding stuff */
#include <fs/unicode.h>

/*
 * Read one wide character off the string, shift the string pointer
 * and return the character.
 */
wchar
ntfs_utf8_wget(const char **str, size_t *sz)
{
	return (wchar) wget_utf8(str, sz);
}

/*
 * Encode wide character and write it to the string. 'n' specifies
 * how much space there is in the string. Returns number of bytes written
 * to the target string.
 */
int
ntfs_utf8_wput(char *s, size_t n, wchar wc)
{
	return wput_utf8(s, n, (u_int16_t) wc);
}

/*
 * Compare two wide characters, returning 1, 0, -1 if the first is
 * bigger, equal or lower than the second.
 */
int
ntfs_utf8_wcmp(wchar wc1, wchar wc2)
{
	/* no conversions needed for utf8 */

	if (wc1 == wc2)
		return 0;
	else
		return (int) wc1 - (int) wc2;
}
