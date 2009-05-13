/*	$NetBSD: fstat.h,v 1.8.6.1 2009/05/13 19:19:50 jym Exp $	*/
/*-
 * Copyright (c) 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

struct  filestat {
	long	fsid;
	ino_t	fileid;
	mode_t	mode;
	off_t	size;
	dev_t	rdev;
};

/*
 * a kvm_read that returns true if everything is read 
 */
#define KVM_READ(kaddr, paddr, len) \
	((size_t)kvm_read(kd, (u_long)(kaddr), (void *)(paddr), (len)) \
	 == (size_t)(len))
#define KVM_NLIST(nl) \
	kvm_nlist(kd, (nl))
#define KVM_GETERR() \
	kvm_geterr(kd)

extern	kvm_t	*kd;
extern	int	 vflg;
extern	pid_t	 Pid;

#define dprintf	if (vflg) warnx

mode_t	getftype(enum vtype);
struct file;
int	pmisc(struct file *, const char *);
int	isofs_filestat(struct vnode *, struct filestat *);
int	ntfs_filestat(struct vnode *, struct filestat *);
int	ptyfs_filestat(struct vnode *, struct filestat *);
int	tmpfs_filestat(struct vnode *, struct filestat *);
