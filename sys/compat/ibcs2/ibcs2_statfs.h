/*	$NetBSD: ibcs2_statfs.h,v 1.3 1998/03/05 04:36:08 scottb Exp $	*/

/*
 * Copyright (c) 1994, 1998 Scott Bartram
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
 *      This product includes software developed by Scott Bartram.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef	_IBCS2_STATFS_H
#define	_IBCS2_STATFS_H

struct ibcs2_statfs {
	short	f_fstyp;
	long	f_bsize;
	long	f_frsize;
	long	f_blocks;
	long	f_bfree;
	long	f_files;
	long	f_ffree;
	char	f_fname[6];
	char	f_fpack[6];
};

struct ibcs2_statvfs {
	u_long	f_bsize;
	u_long	f_frsize;
	u_long	f_blocks;
	u_long	f_bfree;
	u_long	f_bavail;
	u_long	f_files;
	u_long	f_ffree;
	u_long	f_favail;
	u_long	f_fsid;
	char    f_basetype[16];
	u_long	f_flag;
	u_long	f_namemax;
	char    f_fstr[32];
	u_long	pad[16];
};

#endif /* _IBCS2_STATFS_H */
