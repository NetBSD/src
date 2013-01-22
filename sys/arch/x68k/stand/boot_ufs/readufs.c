/*	$NetBSD: readufs.c,v 1.15 2013/01/22 09:39:14 dholland Exp $	*/
/*	from Id: readufs.c,v 1.8 2003/04/08 09:19:32 itohy Exp 	*/

/*
 * Read UFS (FFS / LFS)
 *
 * Written in 1999, 2002, 2003 by ITOH Yasufumi.
 * Public domain.
 *
 * Intended to be used for boot programs (first stage).
 * DON'T ADD ANY FANCY FEATURE.  THIS SHALL BE COMPACT.
 */

#include "readufs.h"

#define fs	ufs_info

static void raw_read_queue(void *buf, daddr_t blkpos, size_t bytelen);
static int ufs_read_indirect(daddr_t blk, int level, uint8_t **buf,
		unsigned *poff, size_t count);

#ifdef DEBUG_WITH_STDIO
void ufs_list_dir(ino32_t dirino);
int main(int argc, char *argv[]);
#endif

#ifdef DEBUG_WITH_STDIO
int fd;

void
RAW_READ(void *buf, daddr_t blkpos, size_t bytelen)
{

	if (pread(fd, buf, bytelen, (off_t)dbtob(blkpos)) != (ssize_t) bytelen)
		err(1, "pread: buf %p, blk %d, len %u",
		    buf, (int) blkpos, bytelen);
}
#endif

struct ufs_info fs;

/*
 * Read contiguous sectors at once for speedup.
 */
static size_t rq_len;

static void
raw_read_queue(void *buf, daddr_t blkpos, size_t bytelen)
	/* bytelen:		 must be DEV_BSIZE aligned */
{
	static daddr_t rq_start;
	static char *rq_buf;

	if (rq_len) {
		if (bytelen && blkpos == rq_start + (ssize_t) btodb(rq_len)
				&& buf == rq_buf + rq_len) {
			rq_len += bytelen;
			return;
		} else {
#ifdef DEBUG_WITH_STDIO
			printf("raw_read_queue: read: buf %p, blk %d, len %d\n",
				 rq_buf, (int) rq_start, rq_len);
#endif
			RAW_READ(rq_buf, rq_start, rq_len);
		}
	}
	rq_buf = buf;
	rq_start = blkpos;
	rq_len = bytelen;
}

#define RAW_READ_QUEUE_INIT()	(rq_len = 0)
#define RAW_READ_QUEUE_FLUSH()	\
		raw_read_queue((void *) 0, (daddr_t) 0, (size_t) 0)


/*
 * Read a file, specified by dinode.
 * No support for holes or (short) symbolic links.
 */
size_t
ufs_read(union ufs_dinode *di, void *buf, unsigned off, size_t count)
	/* off:	 position in block */
{
	struct ufs_info *ufsinfo = &fs;
	size_t bsize = ufsinfo->bsize;
	uint8_t *b = buf;
	int i;
	size_t disize, nread;
	daddr_t pos;
#if defined(USE_UFS1) && defined(USE_UFS2)
	enum ufs_ufstype uver = ufsinfo->ufstype;
#endif

#ifdef DEBUG_WITH_STDIO
	printf("ufs_read: off: %d, count %u\n", off, count);
#endif

	disize = DI_SIZE(di);

	if (disize < count + off * bsize)
		count = disize - off * bsize;

	/* FS block size alignment. */
	nread = count;
	count = (count + bsize - 1) & ~(bsize - 1);

	RAW_READ_QUEUE_INIT();

	/* Read direct blocks. */
	for ( ; off < UFS_NDADDR && count > 0; off++) {
#if defined(USE_UFS1) && defined(USE_UFS2)
		if (uver == UFSTYPE_UFS1)
			pos = di->di1.di_db[off];
		else
			pos = di->di2.di_db[off];
#else
		pos = di->di_thisver.di_db[off];
#endif
#if 0
		printf("ufs_read: read: blk: %d\n",
			(int) pos << ufsinfo->fsbtodb);
#endif
		raw_read_queue(b, pos << ufsinfo->fsbtodb, bsize);
		b += bsize;
		count -= bsize;
	}
	off -= UFS_NDADDR;

	/* Read indirect blocks. */
	for (i = 0; i < UFS_NIADDR && count > 0; i++) {
#if defined(USE_UFS1) && defined(USE_UFS2)
		if (uver == UFSTYPE_UFS1)
			pos = di->di1.di_ib[i];
		else
			pos = di->di2.di_ib[i];
#else
		pos = di->di_thisver.di_ib[i];
#endif
		count = ufs_read_indirect(pos, i, &b, &off, count);
	}

	RAW_READ_QUEUE_FLUSH();

	return nread;
}

static int
ufs_read_indirect(daddr_t blk, int level, uint8_t **buf, unsigned *poff, size_t count)
	/* poff:	 position in block */
{
	struct ufs_info *ufsinfo = &fs;
	size_t bsize = ufsinfo->bsize;
	void *idbuf = alloca(bsize);
#ifdef USE_UFS1
	int32_t *idbuf1 = idbuf;
#endif
#ifdef USE_UFS2
	int64_t *idbuf2 = idbuf;
#endif
	daddr_t pos;
	unsigned off = *poff;
	unsigned b;

#ifdef DEBUG_WITH_STDIO
	printf("ufs_read_indirect: off: %d, count %u\n", off, count);
#endif
	if (off) {
		unsigned subindirsize = 1, indirsize;
		int i;

		for (i = level; i > 0; i--)
			subindirsize *= ufsinfo->nindir;
		indirsize = subindirsize * ufsinfo->nindir;
		if (off >= indirsize) {
			/* no need to read any data */
			*poff = off - indirsize;
			return 0;
		}

		b = off / subindirsize;
		off -= b * subindirsize;
		*poff = 0;
	} else
		b = 0;

	/* read the indirect block */
	RAW_READ(idbuf, blk << ufsinfo->fsbtodb, bsize);

	for ( ; b < ufsinfo->nindir && count > 0; b++) {
#if defined(USE_UFS1) && defined(USE_UFS2)
		if (ufsinfo->ufstype == UFSTYPE_UFS1)
#endif
#ifdef USE_UFS1
			pos = idbuf1[b];
#endif
#if defined(USE_UFS1) && defined(USE_UFS2)
		else
#endif
#ifdef USE_UFS2
			pos = idbuf2[b];
#endif

		if (level)
			count = ufs_read_indirect(pos, level - 1, buf, &off, count);
		else {
#if 0
			printf("ufs_read: read: blk: %d\n",
				(int) pos << ufsinfo->fsbtodb);
#endif
			raw_read_queue(*buf, pos << ufsinfo->fsbtodb, bsize);
			*buf += bsize;
			count -= bsize;
		}
	}

	return count;
}

/*
 * look-up fn in directory dirino
 */
ino32_t
ufs_lookup(ino32_t dirino, const char *fn)
{
	union ufs_dinode dirdi;
	struct direct *pdir;
	char *p, *endp;
	size_t disize;

	if (ufs_get_inode(dirino, &dirdi))
		return 0;

	if ((dirdi.di_common.di_mode & IFMT) != IFDIR)
		return 0;			/* Not a directory */

	disize = DI_SIZE(&dirdi);

#if 0
	p = alloca((disize + fs.bsize - 1) & ~(fs.bsize - 1));
#else	/* simplify calculation to reduce code size */
	p = alloca(disize + fs.bsize);
#endif
	ufs_read(&dirdi, p, 0, disize);
	endp = p + disize;
	for ( ; pdir = (void *) p, p < endp; p += pdir->d_reclen) {
		if (pdir->d_ino && !strcmp(fn, pdir->d_name))
			return pdir->d_ino;
	}
	return 0;				/* No such file or directory */
}

/*
 * look-up a file in absolute pathname from the root directory
 */
ino32_t
ufs_lookup_path(const char *path)
{
	char fn[FFS_MAXNAMLEN + 1];
	char *p;
	ino32_t ino = UFS_ROOTINO;

	do {
		while (*path == '/')
			path++;
		for (p = fn; *path && *path != '/'; )
			*p++ = *path++;
		*p++ = '\0';
		ino = ufs_lookup(ino, fn);
	} while (ino && *path);

	return ino;
}

#if 0
size_t
ufs_load_file(void *buf, ino32_t dirino, const char *fn)
{
	size_t cnt, disize;
	union ufs_dinode dinode;

	if (ufs_fn_inode(dirino, fn, &dinode))
		return (unsigned) 0;
	disize = DI_SIZE(&dinode);
	cnt = ufs_read(&dinode, buf, 0, disize);

	return cnt;
}
#endif

int
ufs_init(void)
{
	return 1
#ifdef USE_FFS
		&& try_ffs()
#endif
#ifdef USE_LFS
		&& try_lfs()
#endif
		;
}

#ifdef DEBUG_WITH_STDIO
void
ufs_list_dir(ino32_t dirino)
{
	union ufs_dinode dirdi;
	struct direct *pdir;
	char *p, *endp;
	size_t disize;

	if (ufs_get_inode(dirino, &dirdi))
		errx(1, "ino = %d: not found", dirino);

	disize = DI_SIZE(&dirdi);
	p = alloca((disize + fs.bsize - 1) & ~(fs.bsize - 1));
	ufs_read(&dirdi, p, 0, disize);
	endp = p + disize;
	for ( ; pdir = (void *) p, p < endp; p += pdir->d_reclen) {
		if (pdir->d_ino)
			printf("%6d %s\n", pdir->d_ino, pdir->d_name);
	}
}
#endif

#ifdef DEBUG_WITH_STDIO
int
main(argc, argv)
	int argc __attribute__((unused));
	char *argv[];
{
	union ufs_dinode dinode;

	if ((fd = open(argv[1], O_RDONLY)) < 0)
		err(1, "open: %s", argv[1]);

	if (ufs_init())
		errx(1, "%s: unknown fs", argv[1]);

#if 1
	ufs_list_dir(UFS_ROOTINO);
	{
		void *p;
		size_t cnt;
		ino32_t ino;
		size_t disize;

		if ((ino = ufs_lookup_path(argv[2])) == 0)
			errx(1, "%s: not found", argv[2]);
		ufs_get_inode(ino, &dinode);
		disize = DI_SIZE(&dinode);
		p = malloc((disize + fs.bsize - 1) & ~(fs.bsize - 1));
		cnt = ufs_read(&dinode, p, 0, disize);
		write(3, p, cnt);
		free(p);
	}
#endif

	return 0;
}
#endif
