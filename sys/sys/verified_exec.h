/*	$NetBSD: verified_exec.h,v 1.4 2003/07/08 06:49:23 itojun Exp $	*/

/*-
 * Copyright (c) 1998-1999 Brett Lymn
 *                         (blymn@baea.com.au, brett_lymn@yahoo.com.au)
 * All rights reserved.
 *
 * This code has been donated to The NetBSD Foundation by the Author.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the author may not be used to endorse or promote products
 *    derived from this software withough specific prior written permission
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
 *
 *
 */

/*
 *
 * Definitions for the Verified Executables kernel function.
 *
 */
#include <sys/param.h>

#ifndef V_EXEC_H
#define V_EXEC_H 1

#define MAXFINGERPRINTLEN 20  /* enough room for largest signature... */

struct verified_exec_params  {
	unsigned char type;
	unsigned char fp_type;  /* type of fingerprint this is */
	char file[MAXPATHLEN];
	unsigned char fingerprint[MAXFINGERPRINTLEN];
};

/*
 * Types of veriexec inodes we can have
 */
#define VERIEXEC_DIRECT   0  /* Allow direct execution */
#define VERIEXEC_INDIRECT 1  /* Only allow indirect execution */
#define VERIEXEC_FILE     2  /* Fingerprint of a plain file */

/*
 * Types of fingerprints we support.
 */
#define FINGERPRINT_TYPE_MD5 1 /* MD5 hash */
#define MD5_FINGERPRINTLEN 16  /* and it's length in chars */
#define FINGERPRINT_TYPE_SHA1 2 /* SHA1 hash */
#define SHA1_FINGERPRINTLEN 20  /* and it's length in chars */

#define VERIEXECLOAD _IOW('S', 0x1, struct verified_exec_params)

#ifdef _KERNEL
void	verifiedexecattach __P((struct device *, struct device *, void *));
int     verifiedexecopen __P((dev_t, int, int, struct proc *));
int     verifiedexecclose __P((dev_t, int, int, struct proc *));
int     verifiedexecioctl __P((dev_t, u_long, caddr_t, int, struct proc *));
/*
 * list structure definitions - needed in kern_exec.c
 */

struct veriexec_devhead veriexec_dev_head;
struct veriexec_devhead veriexec_file_dev_head;

struct veriexec_dev_list {
	unsigned long id;
	LIST_HEAD(inodehead, veriexec_inode_list) inode_head;
	LIST_ENTRY(veriexec_dev_list) entries;
};

struct veriexec_inode_list 
{
	unsigned char type;
	unsigned char fp_type;
	unsigned long inode;
	unsigned char fingerprint[MAXFINGERPRINTLEN];
	LIST_ENTRY(veriexec_inode_list) entries;
};

struct veriexec_inode_list *get_veriexec_inode(struct veriexec_devhead *,
	    long, long, char *);
int evaluate_fingerprint(struct vnode *, struct veriexec_inode_list *,
	    struct proc *, u_quad_t, char *);
int fingerprintcmp(struct veriexec_inode_list *, unsigned char *);

#endif
#endif
