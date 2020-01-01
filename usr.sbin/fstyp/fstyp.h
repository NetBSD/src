/*	$NetBSD: fstyp.h,v 1.7 2020/01/01 08:56:41 tkusumi Exp $	*/

/*-
 * Copyright (c) 2017 The NetBSD Foundation, Inc.
 * Copyright (c) 2016 The DragonFly Project
 * Copyright (c) 2014 The FreeBSD Foundation
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tomohiro Kusumi <kusumi.tomohiro@gmail.com>.
 *
 * This software was developed by Edward Tomasz Napierala under sponsorship
 * from the FreeBSD Foundation.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD$
 */

#ifndef FSTYP_H
#define	FSTYP_H

#include <stdbool.h>

/* Undefine this on FreeBSD and NetBSD. */
//#define HAS_DEVPATH

/* The spec doesn't seem to permit UTF-16 surrogates; definitely LE. */
#define	EXFAT_ENC	"UCS-2LE"
/*
 * NTFS itself is agnostic to encoding; it just stores 255 u16 wchars.  In
 * practice, UTF-16 seems expected for NTFS.  (Maybe also for exFAT.)
 */
#define	NTFS_ENC	"UTF-16LE"

extern bool	show_label;	/* -l flag */

void	*read_buf(FILE *, off_t, size_t);
char	*checked_strdup(const char *);
void	rtrim(char *, size_t);

int	fstyp_apfs(FILE *fp, char *label, size_t size);
int	fstyp_cd9660(FILE *, char *, size_t);
int	fstyp_exfat(FILE *fp, char *label, size_t size);
int	fstyp_ext2fs(FILE *, char *, size_t);
int	fstyp_hfsp(FILE *fp, char *label, size_t size);
int	fstyp_msdosfs(FILE *, char *, size_t);
int	fstyp_ntfs(FILE *, char *, size_t);
int	fstyp_ufs(FILE *, char *, size_t);
int	fstyp_hammer(FILE *, char *, size_t);
int	fstyp_hammer2(FILE *, char *, size_t);
#ifdef HAVE_ZFS
int	fstyp_zfs(FILE *, char *, size_t);
#endif

int	fsvtyp_hammer(const char *blkdevs, char *label, size_t size);
int	fsvtyp_hammer_partial(const char *blkdevs, char *label, size_t size);

#endif /* !FSTYP_H */
