/*	$NetBSD: linux_types.h,v 1.2 2001/09/02 08:39:37 manu Exp $ */

/*-
 * Copyright (c) 1995, 1998, 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Frank van der Linden, Eric Haszlakiewicz and Emmanuel Dreyfus.
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

#ifndef _MIPS_LINUX_TYPES_H
#define _MIPS_LINUX_TYPES_H

/* 
 * from Linux's include/asm-mips/posix-types.h 
 */
typedef struct {
	int val[2];
} linux_fsid_t;

typedef int linux_uid_t;
typedef int linux_gid_t;
typedef unsigned int linux_dev_t;
typedef unsigned int linux_mode_t;
typedef long linux_time_t;
typedef long linux_clock_t;
typedef long linux_off_t;
typedef int linux_pid_t;
#if defined(ELFSIZE) && (ELFSIZE == 64)
typedef unsigned int linux_ino_t;
typedef unsigned int linux_nlink_t;
typedef unsigned long linux_ino_t32;
typedef int linux_nlink_t32;
#else
typedef unsigned long linux_ino_t;
typedef int linux_nlink_t;
typedef linux_ino_t linux_ino_t32;
typedef linux_nlink_t linux_nlink_t32;
#endif

/* 
 * From Linux's include/asm-mips/termbits.h 
 */
typedef unsigned char linux_cc_t;
#if defined(ELFSIZE) && (ELFSIZE == 64)
typedef unsigned int linux_speed_t;
typedef unsigned int linux_tcflag_t;
#else
typedef unsigned long speed_t;
typedef unsigned long tcflag_t;
#endif

/* 
 * From Linux's include/asm-mips/statfs.h 
 */
struct linux_statfs {
	long l_ftype;	   /* Linux name -> NetBSD Linux emul name: s/f_/I_f/ */
	long l_fbsize;	
	long l_ffrsize;	
	long l_fblocks;	
	long l_fbfree;	
	long l_ffiles;	
	long l_fffree;	
	long l_fbavail;	
	linux_fsid_t l_ffsid;	
	long l_fnamelen;	
	long l_fspare[6];	
};

#if defined(ELFSIZE) && (ELFSIZE == 64)
/* 
 * From Linux's include/asm-mips64/stat.h
 * 64 bit version of struct linux_stat memory layout is the 
 * same as of struct stat64 of the 32-bit Linux kernel
 */
struct linux_stat {
	linux_dev_t	lst_dev;
	unsigned int	lst_pad0[3];
	unsigned long	lst_ino;
	linux_mode_t	lst_mode;
	linux_nlink_t	lst_nlink;
	linux_uid_t	lst_uid;
	linux_gid_t	lst_gid;
	linux_dev_t	lst_rdev;
	unsigned int	lst_pad1[3];
	linux_off_t	lst_size;
	unsigned int	lst_atime;
	unsigned int	lreserved0;
	unsigned int	lst_mtime;
	unsigned int	lreserved1;
	unsigned int	lst_ctime;
	unsigned int	lreserved2;
	unsigned int	lst_blksize;
	unsigned int	lst_pad2;
	unsigned long	lst_blocks;
};

struct stat32 {
	linux_dev_t	lst_dev;
 	int		lst_pad1[3];
 	linux_ino_t32	lst_ino;
 	linux_mode_t	lst_mode;
 	linux_nlink_t32	lst_nlink;
 	linux_uid_t	lst_uid;
 	linux_gid_t	lst_gid;
 	linux_dev_t	lst_rdev;
 	int		lst_pad2[2];
 	linux_off_t	lst_size;
 	int		lst_pad3;
 	linux_time_t	lst_atime;
 	int		lreserved0;
 	linux_time_t	lst_mtime;
 	int		lreserved1;
 	linux_time_t	lst_ctime;
 	int		lreserved2;
 	int		lst_blksize;
 	int		lst_blocks;
 	int		lst_pad4[14];
};
#else
/* 
 * This matches struct stat64 in glibc2.1, hence the absolutely
 * insane amounts of padding around dev_t's.
 *
 * Still from Linux's include/asm-mips/stat.h 
 */  
struct linux_stat64 {
	unsigned long	lst_dev;   
	unsigned long	lst_pad0[3];
	linux_ino_t	lst_ino;
	linux_mode_t	lst_mode;
	linux_nlink_t	lst_nlink;
	linux_uid_t	lst_uid;
	linux_gid_t	lst_gid;
	unsigned long	lst_rdev;
	unsigned long	lst_pad1[3];  
	long long	lst_size;
	linux_time_t	lst_atime;
	unsigned long 	lreserved0;
	linux_time_t	lst_mtime;
	unsigned long 	lreserved1;
	linux_time_t	 lst_ctime;
	unsigned long 	lreserved2;
	unsigned long	lst_blksize;
	long long 	lst_blocks;
};

/* 
 * struct linux_stat is defined in Linux's include/asm-mips/stat.h
 * There is also a old_kernel_stat in Linux
 */
struct linux_stat {
	linux_dev_t	lst_dev; 
	long		lst_pad[3];
	linux_ino_t32	lst_ino;
	linux_mode_t	lst_mode;
	linux_nlink_t32	lst_nlink;
	linux_uid_t	lst_uid;
	linux_gid_t	lst_gid;
	linux_dev_t	lst_rdev;
	long		lst_pad2[2];
	linux_off_t	lst_size;
	long		lst_pad3;
	linux_time_t	lst_atime;
	long		lreserved0;
	linux_time_t	lst_mtime;
	long		lreserved1;
	linux_time_t	lst_ctime;
	long		lreserved2;
	long		lst_blksize;
	long		lst_blocks;
	char		lst_fstype[16];
	long		lst_pad4[8];
	unsigned int	lst_flags
	unsigned int	lst_gen;
};
#endif

#endif /* !_MIPS_LINUX_TYPES_H */
