/*	$NetBSD: efs_subr.c,v 1.1 2007/06/29 23:30:29 rumble Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: efs_subr.c,v 1.1 2007/06/29 23:30:29 rumble Exp $");

#include <sys/param.h>
#include <sys/kauth.h>
#include <sys/lwp.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/mount.h>
#include <sys/vnode.h>
#include <sys/namei.h>
#include <sys/stat.h>
#include <sys/malloc.h>

#include <miscfs/genfs/genfs_node.h>

#include <fs/efs/efs.h>
#include <fs/efs/efs_sb.h>
#include <fs/efs/efs_dir.h>
#include <fs/efs/efs_genfs.h>
#include <fs/efs/efs_mount.h>
#include <fs/efs/efs_extent.h>
#include <fs/efs/efs_dinode.h>
#include <fs/efs/efs_inode.h>
#include <fs/efs/efs_subr.h>

MALLOC_DECLARE(M_EFSTMP);

struct pool efs_inode_pool;

/*
 * Calculate a checksum for the provided superblock in __host byte order__.
 *
 * At some point SGI changed the checksum algorithm slightly, which can be
 * enabled with the 'new' flag.
 *
 * Presumably this change occured on or before 24 Oct 1988 (around IRIX 3.1),
 * so we're pretty unlikely to ever actually see an old checksum. Further, it
 * means that EFS_NEWMAGIC filesystems (IRIX >= 3.3) must match the new
 * checksum whereas EFS_MAGIC filesystems could potentially use either
 * algorithm.
 *
 * See comp.sys.sgi <1991Aug9.050838.16876@odin.corp.sgi.com>
 */
int32_t
efs_sb_checksum(struct efs_sb *esb, int new)
{
	int i;
	int32_t cksum;
	int16_t *sbarray = (int16_t *)esb;

	KASSERT((EFS_SB_CHECKSUM_SIZE % 2) == 0);

	for (i = cksum = 0; i < (EFS_SB_CHECKSUM_SIZE / 2); i++) {
		cksum ^= be16toh(sbarray[i]);
		cksum  = (cksum << 1) | (new && cksum < 0);
	}

	return (cksum);
}

/*
 * Determine if the superblock is valid.
 *
 * Returns 0 if valid, else invalid. If invalid, 'why' is set to an
 * explanation.
 */
int 
efs_sb_validate(struct efs_sb *esb, const char **why)
{
	uint32_t ocksum, ncksum;

	*why = NULL;

	if (be32toh(esb->sb_magic) != EFS_SB_MAGIC &&
	    be32toh(esb->sb_magic != EFS_SB_NEWMAGIC)) {
		*why = "sb_magic invalid";
		return (1);
	}

	ocksum = htobe32(efs_sb_checksum(esb, 0));
	ncksum = htobe32(efs_sb_checksum(esb, 1));
	if (esb->sb_checksum != ocksum && esb->sb_checksum != ncksum) {
		*why = "sb_checksum invalid";
		return (1);
	}

	if (be32toh(esb->sb_size) > EFS_SIZE_MAX) {
		*why = "sb_size > EFS_SIZE_MAX";
		return (1);
	}

	if (be32toh(esb->sb_firstcg) <= EFS_BB_BITMAP) {
		*why = "sb_firstcg <= EFS_BB_BITMAP";
		return (1);
	}

	/* XXX - add better sb consistency checks here */
	if (esb->sb_cgfsize == 0 ||
	    esb->sb_cgisize == 0 ||
	    esb->sb_ncg == 0 ||
	    esb->sb_bmsize == 0) {
		*why = "something bad happened";
		return (1);
	}

	return (0);
}

/*
 * Determine the basic block offset and inode index within that block, given
 * the inode 'ino' and filesystem parameters _in host byte order_. The inode
 * will live at byte address 'bboff' * EFS_BB_SIZE + 'index' * EFS_DINODE_SIZE.
 */
void
efs_locate_inode(ino_t ino, struct efs_sb *sbp, uint32_t *bboff, int *index)
{
	uint32_t cgfsize, firstcg;
	uint16_t cgisize;

	cgisize = be16toh(sbp->sb_cgisize);
	cgfsize = be32toh(sbp->sb_cgfsize);
	firstcg = be32toh(sbp->sb_firstcg),

	*bboff = firstcg + ((ino / (cgisize * EFS_DINODES_PER_BB)) * cgfsize) +
	    ((ino % (cgisize * EFS_DINODES_PER_BB)) / EFS_DINODES_PER_BB);
	*index = ino & (EFS_DINODES_PER_BB - 1);
}

/*
 * Read in an inode from disk.
 *
 * We actually take in four inodes at a time. Hopefully these will stick
 * around in the buffer cache and get used without going to disk.
 *
 * Returns 0 on success.
 */
int
efs_read_inode(struct efs_mount *emp, ino_t ino, struct lwp *l,
    struct efs_dinode *di)
{
	struct efs_sb *sbp;
	struct buf *bp;
	int index, err;
	uint32_t bboff;

	sbp = &emp->em_sb;
	efs_locate_inode(ino, sbp, &bboff, &index);

	err = efs_bread(emp, bboff, EFS_BY2BB(EFS_DINODE_SIZE), l, &bp);
	if (err) {
		brelse(bp);
		return (err);
	}
	memcpy(di, ((struct efs_dinode *)bp->b_data) + index, sizeof(*di));
	brelse(bp);

	return (0);
}

/*
 * Perform a read from our device handling the potential DEV_BSIZE
 * messiness (although as of 19.2.2006, all ports appear to use 512) as
 * we as EFS block sizing.
 *
 * bboff: basic block offset
 * nbb: number of basic blocks to be read
 *
 * Returns 0 on success.
 */
int
efs_bread(struct efs_mount *emp, uint32_t bboff, int nbb, struct lwp *l,
    struct buf **bp)
{
	KASSERT(nbb > 0);
	KASSERT(bboff < EFS_SIZE_MAX);

	return (bread(emp->em_devvp, (daddr_t)bboff * (EFS_BB_SIZE / DEV_BSIZE),
	    nbb * EFS_BB_SIZE, (l == NULL) ? NOCRED : l->l_cred, bp));
}

/*
 * Synchronise the in-core, host ordered and typed inode fields with their
 * corresponding on-disk, EFS ordered and typed copies.
 *
 * This is the inverse of efs_dinode_sync_inode(), and should be called when
 * an inode is loaded from disk.
 */
void
efs_sync_dinode_to_inode(struct efs_inode *ei)
{

	ei->ei_mode		= be16toh(ei->ei_di.di_mode);	/*same as nbsd*/
	ei->ei_nlink		= be16toh(ei->ei_di.di_nlink);
	ei->ei_uid		= be16toh(ei->ei_di.di_uid);
	ei->ei_gid		= be16toh(ei->ei_di.di_gid);
	ei->ei_size		= be32toh(ei->ei_di.di_size);
	ei->ei_atime		= be32toh(ei->ei_di.di_atime);
	ei->ei_mtime		= be32toh(ei->ei_di.di_mtime);
	ei->ei_ctime		= be32toh(ei->ei_di.di_ctime);
	ei->ei_gen		= be32toh(ei->ei_di.di_gen);
	ei->ei_numextents 	= be16toh(ei->ei_di.di_numextents);
	ei->ei_version		= ei->ei_di.di_version;
}

/*
 * Synchronise the on-disk, EFS ordered and typed inode fields with their
 * corresponding in-core, host ordered and typed copies.
 *
 * This is the inverse of efs_inode_sync_dinode(), and should be called before
 * an inode is flushed to disk.
 */
void
efs_sync_inode_to_dinode(struct efs_inode *ei)
{
	
	panic("readonly -- no need to call me");
}

#ifdef DIAGNOSTIC
/*
 * Ensure that the in-core inode's host cached fields match its on-disk copy.
 * 
 * Returns 0 if they match.
 */
static int
efs_is_inode_synced(struct efs_inode *ei)
{
	int s;

	s = 0;
	/* XXX -- see above remarks about assumption */
	s += (ei->ei_mode	!= be16toh(ei->ei_di.di_mode));
	s += (ei->ei_nlink	!= be16toh(ei->ei_di.di_nlink));
	s += (ei->ei_uid	!= be16toh(ei->ei_di.di_uid));
	s += (ei->ei_gid	!= be16toh(ei->ei_di.di_gid));
	s += (ei->ei_size	!= be32toh(ei->ei_di.di_size));
	s += (ei->ei_atime	!= be32toh(ei->ei_di.di_atime));
	s += (ei->ei_mtime	!= be32toh(ei->ei_di.di_mtime));
	s += (ei->ei_ctime	!= be32toh(ei->ei_di.di_ctime));
	s += (ei->ei_gen	!= be32toh(ei->ei_di.di_gen));
	s += (ei->ei_numextents	!= be16toh(ei->ei_di.di_numextents));
	s += (ei->ei_version	!= ei->ei_di.di_version);

	return (s);
}
#endif

/*
 * Given an efs_dirblk structure and a componentname to search for, return the
 * corresponding inode if it is found.
 *
 * Returns 0 on success.
 */
static int
efs_dirblk_lookup(struct efs_dirblk *dir, struct componentname *cn,
    ino_t *inode)
{
	struct efs_dirent *de;
	int i, slot, offset;

	KASSERT(cn->cn_namelen <= EFS_DIRENT_NAMELEN_MAX);

	slot = offset = 0;

	for (i = 0; i < dir->db_slots; i++) {
		offset = EFS_DIRENT_OFF_EXPND(dir->db_space[i]);

		if (offset == EFS_DIRBLK_SLOT_FREE)
			continue;

		de = (struct efs_dirent *)((char *)dir + offset);
		if (de->de_namelen == cn->cn_namelen &&
		   (strncmp(cn->cn_nameptr, de->de_name, cn->cn_namelen) == 0)){
			slot = i;
			break;
		}
	}
	if (i == dir->db_slots)
		return (ENOENT);

	KASSERT(slot < offset && offset < EFS_DIRBLK_SPACE_SIZE);
	de = (struct efs_dirent *)((char *)dir + offset);
	*inode = be32toh(de->de_inumber);

	return (0);
}

/*
 * Given an extent descriptor that represents a directory, look up
 * componentname within its efs_dirblk's. If it is found, return the
 * corresponding inode in 'ino'.
 *
 * Returns 0 on success.
 */
static int
efs_extent_lookup(struct efs_mount *emp, struct efs_extent *ex,
    struct componentname *cn, ino_t *ino)
{
	struct efs_dirblk *db;
	struct buf *bp;
	int i, err;

	/*
	 * Read in the entire extent, evaluating all of the dirblks until we
	 * find our entry. If we don't, return ENOENT.
	 */
	err = efs_bread(emp, ex->ex_bn, ex->ex_length, NULL, &bp);
	if (err) {
		printf("efs: warning: invalid extent descriptor\n");
		brelse(bp);
		return (err);
	}

	for (i = 0; i < ex->ex_length; i++) {
		db = ((struct efs_dirblk *)bp->b_data) + i;
		if (efs_dirblk_lookup(db, cn, ino) == 0) {
			brelse(bp);
			return (0);
		}
	}
	
	brelse(bp);
	return (ENOENT);
}

/*
 * Given the provided in-core inode, look up the pathname requested. If
 * we find it, 'ino' reflects its corresponding on-disk inode number.
 *
 * Returns 0 on success.
 */
int
efs_inode_lookup(struct efs_mount *emp, struct efs_inode *ei,
    struct componentname *cn, ino_t *ino)
{
	struct efs_extent ex;
	struct efs_extent_iterator exi;
	int ret;
	
	KASSERT(VOP_ISLOCKED(ei->ei_vp));
	KASSERT(efs_is_inode_synced(ei) == 0);
	KASSERT((ei->ei_mode & S_IFMT) == S_IFDIR);

	efs_extent_iterator_init(&exi, ei);
	while ((ret = efs_extent_iterator_next(&exi, &ex)) == 0) {
		if (efs_extent_lookup(emp, &ex, cn, ino) == 0) {
			efs_extent_iterator_free(&exi);
			return (0);
		}
	}
	efs_extent_iterator_free(&exi);

	return ((ret == -1) ? ENOENT : ret);
}

/*
 * Convert on-disk extent structure to in-core format.
 */
void 
efs_dextent_to_extent(struct efs_dextent *dex, struct efs_extent *ex)
{

	KASSERT(dex != NULL && ex != NULL);

	ex->ex_magic	= dex->ex_bytes[0];
	ex->ex_bn	= be32toh(dex->ex_words[0]) & 0x00ffffff;
	ex->ex_length	= dex->ex_bytes[4];
	ex->ex_offset	= be32toh(dex->ex_words[1]) & 0x00ffffff;
}

/*
 * Convert in-core extent format to on-disk structure.
 */
void
efs_extent_to_dextent(struct efs_extent *ex, struct efs_dextent *dex)
{

	KASSERT(ex != NULL && dex != NULL);
	KASSERT(ex->ex_magic == EFS_EXTENT_MAGIC);
	KASSERT((ex->ex_bn & ~EFS_EXTENT_BN_MASK) == 0);
	KASSERT((ex->ex_offset & ~EFS_EXTENT_OFFSET_MASK) == 0);

	dex->ex_words[0] = htobe32(ex->ex_bn);
	dex->ex_bytes[0] = ex->ex_magic;
	dex->ex_words[1] = htobe32(ex->ex_offset);
	dex->ex_bytes[4] = ex->ex_length;
}

/*
 * Initialise an extent iterator.
 */
void
efs_extent_iterator_init(struct efs_extent_iterator *exi, struct efs_inode *eip)
{
	
	exi->exi_eip		= eip;
	exi->exi_next		= 0;
	exi->exi_dnext		= 0;
	exi->exi_innext		= 0;
	exi->exi_incache	= NULL;
	exi->exi_nincache	= 0;
}

/*
 * Return the next EFS extent.
 *
 * Returns 0 if another extent was iterated, -1 if we've exhausted all
 * extents, or an error number. If 'exi' is non-NULL, the next extent is
 * written to it (should it exist).
 */
int
efs_extent_iterator_next(struct efs_extent_iterator *exi,
    struct efs_extent *exp)
{
	struct efs_inode *eip = exi->exi_eip;

	if (exi->exi_next++ >= eip->ei_numextents)
		return (-1);

	/* direct or indirect extents? */
	if (eip->ei_numextents <= EFS_DIRECTEXTENTS) {
		if (exp != NULL) {
			efs_dextent_to_extent(
			    &eip->ei_di.di_extents[exi->exi_dnext++], exp);
		}
	} else {
		/*
		 * Cache a full indirect extent worth of extent descriptors.
		 * This is maximally 124KB (248 * 512).
		 */
		if (exi->exi_incache == NULL) {
			struct efs_extent ex;
			struct buf *bp;
			int err;

			efs_dextent_to_extent(
			    &eip->ei_di.di_extents[exi->exi_dnext], &ex);

			err = efs_bread(VFSTOEFS(eip->ei_vp->v_mount),
			    ex.ex_bn, ex.ex_length, NULL, &bp);
			if (err) {
				EFS_DPRINTF(("efs_extent_iterator_next: "
				    "efs_bread failed: %d\n", err));
				brelse(bp);
				return (err);
			}

			exi->exi_incache = malloc(ex.ex_length * EFS_BB_SIZE,
			    M_EFSTMP, M_WAITOK);
			exi->exi_nincache = ex.ex_length * EFS_BB_SIZE /
			    sizeof(struct efs_dextent);
			memcpy(exi->exi_incache, bp->b_data,
			    ex.ex_length * EFS_BB_SIZE);
			brelse(bp);
		}

		if (exp != NULL) {
			efs_dextent_to_extent(
			    &exi->exi_incache[exi->exi_innext++], exp);
		}

		/* if this is the last one, ditch the cache */
		if (exi->exi_innext >= exi->exi_nincache) {
			exi->exi_innext = 0;
			exi->exi_nincache = 0;
			free(exi->exi_incache, M_EFSTMP);
			exi->exi_incache = NULL;
			exi->exi_dnext++;
		}
	}

	return (0);
}

/*
 * Clean up the extent iterator.
 */
void
efs_extent_iterator_free(struct efs_extent_iterator *exi)
{

	if (exi->exi_incache != NULL)
		free(exi->exi_incache, M_EFSTMP);
	efs_extent_iterator_init(exi, NULL);
}
