/*	from Id: readufs.c,v 1.7 2002/01/26 15:55:51 itohy Exp 	*/

/*
 * Read UFS (FFS / LFS)
 *
 * Written by ITOH, Yasufumi (itohy@netbsd.org).
 * Public domain.
 *
 * Intended to be used for boot programs (first stage).
 * DON'T ADD ANY FANCY FEATURE.  THIS SHALL BE COMPACT.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <ufs/ufs/dinode.h>

#include "readufs.h"

#define fs	ufs_info

static void raw_read_queue __P((void *buf, daddr_t blkpos, size_t bytelen));
static int ufs_read_indirect __P((daddr_t blk, int level, caddr_t *buf,
		unsigned *poff, size_t count));

#ifdef DEBUG_WITH_STDIO
void ufs_list_dir __P((ino_t dirino));
int main __P((int argc, char *argv[]));
#endif

#ifdef DEBUG_WITH_STDIO
int fd;

void
RAW_READ(buf, blkpos, bytelen)
	void *buf;
	daddr_t blkpos;
	size_t bytelen;
{
	if (pread(fd, buf, bytelen, (off_t)dbtob(blkpos)) != (ssize_t) bytelen)
		err(1, "pread: buf %p, blk %u, len %u", buf, blkpos, bytelen);
}
#endif

struct ufs_info fs;

/*
 * Read contiguous sectors at once for speedup.
 */
static size_t rq_len;

static void
raw_read_queue(buf, blkpos, bytelen)
	void *buf;
	daddr_t blkpos;
	size_t bytelen;		/* must be DEV_BSIZE aligned */
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
				 rq_buf, rq_start, rq_len);
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
		raw_read_queue((void *) 0, (u_int32_t) 0, (size_t) 0)


/*
 * Read a file, specified by dinode.
 * No support for holes or (short) symbolic links.
 */
size_t
ufs_read(di, buf, off, count)
	struct dinode *di;
	void *buf;
	unsigned off;	/* position in block */
	size_t count;
{
	size_t bsize = fs.bsize;
	caddr_t b = buf;
	int i;
	size_t nread;

#ifdef DEBUG_WITH_STDIO
	printf("ufs_read: off: %d, count %u\n", off, count);
#endif
	if ((size_t) di->di_size < count + off * bsize)
		count = (size_t) di->di_size - off * bsize;

	/* FS block size alignment. */
	nread = count;
	count = (count + bsize - 1) & ~(bsize - 1);

	RAW_READ_QUEUE_INIT();

	/* Read direct blocks. */
	for ( ; off < NDADDR && count > 0; off++) {
#if 0
		printf("ufs_read: read: blk: %d\n",
			di->di_db[off] << fs.fsbtodb);
#endif
		raw_read_queue(b, di->di_db[off] << fs.fsbtodb, bsize);
		b += bsize;
		count -= bsize;
	}
	off -= NDADDR;

	/* Read indirect blocks. */
	for (i = 0; i < NIADDR && count > 0; i++)
		count = ufs_read_indirect(di->di_ib[i], i, &b, &off, count);

	RAW_READ_QUEUE_FLUSH();

	return (size_t) nread;
}

static int
ufs_read_indirect(blk, level, buf, poff, count)
	daddr_t blk;
	int level;
	caddr_t *buf;
	unsigned *poff;	/* position in block */
	size_t count;
{
	size_t bsize = fs.bsize;
	/* XXX ondisk32 */
	int32_t *idbuf = alloca(bsize);
	unsigned off = *poff;
	unsigned b;

#ifdef DEBUG_WITH_STDIO
	printf("ufs_read_indirect: off: %d, count %u\n", off, count);
#endif
	if (off) {
		unsigned subindirsize = 1, indirsize;
		int i;

		for (i = level; i > 0; i--)
			subindirsize *= fs.nindir;
		indirsize = subindirsize * fs.nindir;
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
	RAW_READ(idbuf, blk << fs.fsbtodb, bsize);

	for ( ; b < fs.nindir && count > 0; b++) {
		if (level)
			count = ufs_read_indirect(idbuf[b], level - 1, buf, &off, count);
		else {
#if 0
			printf("ufs_read: read: blk: %d\n",
				idbuf[b] << fs.fsbtodb);
#endif
			raw_read_queue(*buf, idbuf[b] << fs.fsbtodb, bsize);
			*buf += bsize;
			count -= bsize;
		}
	}

	return count;
}

/*
 * look-up fn in directory dirino
 */
ino_t
ufs_lookup(dirino, fn)
	ino_t dirino;
	const char *fn;
{
	struct dinode dirdi;
	struct direct *pdir;
	char *p, *endp;

	if (ufs_get_inode(dirino, &dirdi))
		return 0;

	if ((dirdi.di_mode & IFMT) != IFDIR)
		return 0;			/* Not a directory */

#if 0
	p = alloca(((size_t) dirdi.di_size + fs.bsize - 1) & ~(fs.bsize - 1));
#else	/* simplify calculation to reduce code size */
	p = alloca((size_t) dirdi.di_size + fs.bsize);
#endif
	ufs_read(&dirdi, p, 0, (size_t) dirdi.di_size);
	endp = p + dirdi.di_size;
	for ( ; pdir = (void *) p, p < endp; p += pdir->d_reclen) {
		if (pdir->d_ino && !strcmp(fn, pdir->d_name))
			return pdir->d_ino;
	}
	return 0;				/* No such file or directory */
}

/*
 * look-up a file in absolute pathname from the root directory
 */
ino_t
ufs_lookup_path(path)
	const char *path;
{
	char fn[MAXNAMLEN + 1];
	char *p;
	ino_t ino = ROOTINO;

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
ufs_load_file(buf, dirino, fn)
	void *buf;
	ino_t dirino;
	const char *fn;
{
	size_t cnt;
	struct dinode dinode;

	if (ufs_fn_inode(dirino, fn, &dinode))
		return (unsigned) 0;
	cnt = ufs_read(&dinode, buf, 0, (size_t) dinode.di_size);

	return cnt;
}
#endif

int
ufs_init()
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
ufs_list_dir(dirino)
	ino_t dirino;
{
	struct dinode dirdi;
	struct direct *pdir;
	char *p, *endp;

	if (ufs_get_inode(dirino, &dirdi))
		errx(1, "ino = %d: not found", dirino);

	p = alloca(((size_t) dirdi.di_size + fs.bsize - 1) & ~(fs.bsize - 1));
	ufs_read(&dirdi, p, 0, (size_t) dirdi.di_size);
	endp = p + dirdi.di_size;
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
	struct dinode dinode;

	if ((fd = open(argv[1], O_RDONLY)) < 0)
		err(1, "open: %s", argv[1]);

	if (ufs_init())
		errx(1, "%s: unknown fs", argv[1]);

#if 1
	ufs_list_dir(ROOTINO);
	{
		void *p;
		size_t cnt;
		ino_t ino;

		if ((ino = ufs_lookup_path(argv[2])) == 0)
			errx(1, "%s: not found", argv[2]);
		ufs_get_inode(ino, &dinode);
		p = malloc(((size_t) dinode.di_size + fs.bsize - 1) & ~(fs.bsize - 1));
		cnt = ufs_read(&dinode, p, 0, (size_t) dinode.di_size);
		write(3, p, cnt);
		free(p);
	}
#endif

	return 0;
}
#endif
