/*	$NetBSD: fts.h,v 1.6 1997/10/21 00:55:10 fvdl Exp $	*/

/*
 * Copyright (c) 1989, 1993
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
 *	@(#)fts.h	8.3 (Berkeley) 8/14/94
 */

#ifndef	_FTS_H_
#define	_FTS_H_

/*
 * fts_options flags
 */
#define	FTS_COMFOLLOW	0x001		/* follow command line symlinks */
#define	FTS_LOGICAL	0x002		/* logical walk */
#define	FTS_NOCHDIR	0x004		/* don't change directories */
#define	FTS_NOSTAT	0x008		/* don't get stat info */
#define	FTS_PHYSICAL	0x010		/* physical walk */
#define	FTS_SEEDOT	0x020		/* return dot and dot-dot */
#define	FTS_XDEV	0x040		/* don't cross devices */
#define	FTS_WHITEOUT	0x080		/* return whiteout information */
#define	FTS_OPTIONMASK	0x0ff		/* valid user option mask */

#define	FTS_NAMEONLY	0x100		/* (private) child names only */
#define	FTS_STOP	0x200		/* (private) unrecoverable error */

typedef struct {
	struct _ftsent12 *fts_cur;	/* current node */
	struct _ftsent12 *fts_child;	/* linked list of children */
	struct _ftsent12 **fts_array;	/* sort array */
	dev_t fts_dev;			/* starting device # */
	char *fts_path;			/* path for this descent */
	int fts_rfd;			/* fd for root */
	int fts_pathlen;		/* sizeof(path) */
	int fts_nitems;			/* elements in the sort array */
	int (*fts_compar)();		/* compare function */

	int fts_options;		/* fts_open options, global flags */
} FTS12;

typedef struct {
	struct _ftsent *fts_cur;	/* current node */
	struct _ftsent *fts_child;	/* linked list of children */
	struct _ftsent **fts_array;	/* sort array */
	dev_t fts_dev;			/* starting device # */
	char *fts_path;			/* path for this descent */
	int fts_rfd;			/* fd for root */
	int fts_pathlen;		/* sizeof(path) */
	int fts_nitems;			/* elements in the sort array */
	int (*fts_compar)();		/* compare function */

	int fts_options;		/* fts_open options, global flags */
} FTS;

/*
 * fts_level defines.
 */
#define	FTS_ROOTPARENTLEVEL	-1
#define	FTS_ROOTLEVEL		 0

/*
 * fts_info defines
 */
#define	FTS_D		 1		/* preorder directory */
#define	FTS_DC		 2		/* directory that causes cycles */
#define	FTS_DEFAULT	 3		/* none of the above */
#define	FTS_DNR		 4		/* unreadable directory */
#define	FTS_DOT		 5		/* dot or dot-dot */
#define	FTS_DP		 6		/* postorder directory */
#define	FTS_ERR		 7		/* error; errno is set */
#define	FTS_F		 8		/* regular file */
#define	FTS_INIT	 9		/* initialized only */
#define	FTS_NS		10		/* stat(2) failed */
#define	FTS_NSOK	11		/* no stat(2) requested */
#define	FTS_SL		12		/* symbolic link */
#define	FTS_SLNONE	13		/* symbolic link without target */
#define	FTS_W		14		/* whiteout object */

/*
 * fts_flags defines
 */
#define	FTS_DONTCHDIR	 0x01		/* don't chdir .. to the parent */
#define	FTS_SYMFOLLOW	 0x02		/* followed a symlink to get here */
#define	FTS_ISW		 0x04		/* this is a whiteout object */

/*
 * fts_set instructions
 */
#define	FTS_AGAIN	 1		/* read node again */
#define	FTS_FOLLOW	 2		/* follow symbolic link */
#define	FTS_NOINSTR	 3		/* no instructions */
#define	FTS_SKIP	 4		/* discard node */

typedef struct _ftsent {
	struct _ftsent *fts_cycle;	/* cycle node */
	struct _ftsent *fts_parent;	/* parent directory */
	struct _ftsent *fts_link;	/* next file in directory */
	long fts_number;	        /* local numeric value */
	void *fts_pointer;	        /* local address value */
	char *fts_accpath;		/* access path */
	char *fts_path;			/* root path */
	int fts_errno;			/* errno for this node */
	int fts_symfd;			/* fd for symlink */
	u_short fts_pathlen;		/* strlen(fts_path) */
	u_short fts_namelen;		/* strlen(fts_name) */

	ino_t fts_ino;			/* inode */
	dev_t fts_dev;			/* device */
	nlink_t fts_nlink;		/* link count */

	short fts_level;		/* depth (-1 to N) */

	u_short fts_info;		/* user flags for FTSENT structure */

	u_short fts_flags;		/* private flags for FTSENT structure */

	u_short fts_instr;		/* fts_set() instructions */

	struct stat *fts_statp;		/* stat(2) information */
	char fts_name[1];		/* file name */
} FTSENT;

typedef struct _ftsent12 {
	struct _ftsent12 *fts_cycle;	/* cycle node */
	struct _ftsent12 *fts_parent;	/* parent directory */
	struct _ftsent12 *fts_link;	/* next file in directory */
	long fts_number;	        /* local numeric value */
	void *fts_pointer;	        /* local address value */
	char *fts_accpath;		/* access path */
	char *fts_path;			/* root path */
	int fts_errno;			/* errno for this node */
	int fts_symfd;			/* fd for symlink */
	u_short fts_pathlen;		/* strlen(fts_path) */
	u_short fts_namelen;		/* strlen(fts_name) */

	ino_t fts_ino;			/* inode */
	dev_t fts_dev;			/* device */
	u_int16_t fts_nlink;		/* link count */

	short fts_level;		/* depth (-1 to N) */

	u_short fts_info;		/* user flags for FTSENT structure */

	u_short fts_flags;		/* private flags for FTSENT structure */

	u_short fts_instr;		/* fts_set() instructions */

	struct stat12 *fts_statp;	/* stat(2) information */
	char fts_name[1];		/* file name */
} FTSENT12;

#include <sys/cdefs.h>

__BEGIN_DECLS
FTSENT12	*fts_children __P((FTS12 *, int));
int		 fts_close __P((FTS12 *));
FTS12		*fts_open __P((char * const *, int,
	    	    int (*)(const FTSENT12 **, const FTSENT12 **)));
FTSENT12	*fts_read __P((FTS12 *));
int	 	fts_set __P((FTS12 *, FTSENT12 *, int));

FTSENT	*__fts_children13 __P((FTS *, int));
int	 __fts_close13 __P((FTS *));
FTS	*__fts_open13 __P((char * const *, int,
	    int (*)(const FTSENT **, const FTSENT **)));
FTSENT	*__fts_read13 __P((FTS *));
int	 __fts_set13 __P((FTS *, FTSENT *, int));
__END_DECLS

#define fts_children(f,i)	__fts_children13(f,i)
#define fts_close(f)		__fts_close13(f)
#define fts_open(p,i,f)		__fts_open13(p,i,f)
#define fts_read(f)		__fts_read13(f)
#define fts_set(f,g,i)	__fts_set13(f,g,i)

#endif /* !_FTS_H_ */
