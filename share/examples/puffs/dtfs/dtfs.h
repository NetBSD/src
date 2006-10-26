/*	$NetBSD: dtfs.h,v 1.3 2006/10/26 22:53:25 pooka Exp $	*/

/*
 * Copyright (c) 2006  Antti Kantee.  All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef DTFS_H_
#define DTFS_H_

#include <sys/types.h>

#include <puffs.h>

PUFFSVFS_PROTOS(dtfs);
PUFFSVN_PROTOS(dtfs);

struct dtfs_mount {
	ino_t		dtm_nextfileid;	/* running number for file id	*/
	fsid_t		dtm_fsidx;	/* fsidx from puffs_start()	*/

	size_t		dtm_fsizes;	/* sum of file sizes in bytes	*/
	fsfilcnt_t	dtm_nfiles;	/* number of files		*/
};

struct dtfs_file {
	union {
		struct {
			uint8_t *data;
			size_t datalen;
		} reg;
		struct {
			struct puffs_node *dotdot;
			LIST_HEAD(, dtfs_dirent) dirents;
		} dir; 
		struct {
			char *target;
		} link;
		struct {
			dev_t rdev;
		} dev;
	} u;
#define df_data u.reg.data
#define df_datalen u.reg.datalen
#define df_dotdot u.dir.dotdot
#define df_dirents u.dir.dirents
#define df_linktarget u.link.target
#define df_rdev u.dev.rdev
};

struct dtfs_dirent {
	struct puffs_node *dfd_node;
	struct puffs_node *dfd_parent;
	char *dfd_name;

	LIST_ENTRY(dtfs_dirent) dfd_entries;
};

struct dtfs_fs {
	size_t dtfs_size;
};

struct puffs_node *	dtfs_genfile(struct puffs_node *, const char *,
				     enum vtype);
struct dtfs_file *	dtfs_newdir(void);
struct dtfs_file *	dtfs_newfile(void);
struct dtfs_dirent *	dtfs_dirgetnth(struct dtfs_file *, int);
struct dtfs_dirent *	dtfs_dirgetbyname(struct dtfs_file *, const char *);

void			dtfs_nukenode(struct puffs_node *, struct puffs_node *,
				      const char *);
void			dtfs_freenode(struct puffs_node *);
void			dtfs_setsize(struct puffs_node *, off_t, int);

void	dtfs_adddent(struct puffs_node *, struct dtfs_dirent *);
void	dtfs_removedent(struct puffs_node *, struct dtfs_dirent *);

void	dtfs_baseattrs(struct vattr *, enum vtype, int32_t, ino_t);


#define DTFS_CTOF(a) ((struct dtfs_file *)(((struct puffs_node *)a)->pn_data))
#define DTFS_PTOF(a) ((struct dtfs_file *)(a->pn_data))

#endif /* DTFS_H_ */
