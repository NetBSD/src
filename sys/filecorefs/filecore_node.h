/*	$NetBSD: filecore_node.h,v 1.5 2001/09/15 16:12:56 chs Exp $	*/

/*-
 * Copyright (c) 1998 Andrew McMurry
 * Copyright (c) 1994 The Regents of the University of California.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 *
 *	filecore_node.h		1.1	1998/6/26
 */

/*
 * In a future format, directories may be more than 2Gb in length,
 * however, in practice this seems unlikely. So, we define
 * the type doff_t as a long to keep down the cost of doing
 * lookup on a 32-bit machine. If you are porting to a 64-bit
 * architecture, you should make doff_t the same as off_t.
 */
#define doff_t	long

struct filecore_node {
	LIST_ENTRY(filecore_node) i_hash;
	struct	vnode *i_vnode;	/* vnode associated with this inode */
	struct	vnode *i_devvp;	/* vnode for block I/O */
	u_long	i_flag;		/* see below */
	dev_t	i_dev;		/* device where inode resides */
	ino_t	i_number;	/* the identity of the inode */
	daddr_t	i_block;	/* the disc address of the file */
	ino_t	i_parent;	/* the ino of the file's parent */

	struct	filecore_mnt *i_mnt;	/* filesystem associated with this inode */
	struct	lockf *i_lockf;	/* head of byte-level lock list */
        int	i_diroff;	/* offset in dir, where we found last entry */

	struct	filecore_direntry i_dirent; /* directory entry */
};

#define	i_forw		i_chain[0]
#define	i_back		i_chain[1]
#define i_size		i_dirent.len

/* flags */
#define	IN_ACCESS	0x0020		/* inode access time to be updated */

#define VTOI(vp) ((struct filecore_node *)(vp)->v_data)
#define ITOV(ip) ((ip)->i_vnode)

#define filecore_staleinode(ip) ((ip)->i_dirent.name[0]==0)

/*
 * Prototypes for Filecore vnode operations
 */
int	filecore_lookup		__P((void *));
#define	filecore_open		genfs_nullop
#define	filecore_close		genfs_nullop
int	filecore_access		__P((void *));
int	filecore_getattr	__P((void *));
int	filecore_read		__P((void *));
#define	filecore_poll		genfs_poll
#define	filecore_mmap		genfs_mmap
#define	filecore_seek		genfs_seek
int	filecore_readdir	__P((void *));
int	filecore_readlink	__P((void *));
#define	filecore_abortop	genfs_abortop
int	filecore_inactive	__P((void *));
int	filecore_reclaim	__P((void *));
int	filecore_link		__P((void *));
int	filecore_symlink	__P((void *));
int	filecore_bmap		__P((void *));
int	filecore_strategy	__P((void *));
int	filecore_print		__P((void *));
int	filecore_pathconf	__P((void *));
int	filecore_blkatoff	__P((void *));

struct	vnode *filecore_ihashget __P((dev_t, ino_t));
void	filecore_ihashins __P((struct filecore_node *));
void	filecore_ihashrem __P((struct filecore_node *));

mode_t	filecore_mode	__P((struct filecore_node *));
struct timespec	filecore_time	__P((struct filecore_node *));
ino_t	filecore_getparent	__P((struct filecore_node *));
int	filecore_fn2unix	__P((char *, char *, u_int8_t *));
int	filecore_fncmp		__P((const char *, const char *, u_short));
int	filecore_dbread		__P((struct filecore_node *, struct buf **));
