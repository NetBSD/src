/*	$NetBSD: getmntinfo.c,v 1.3 2004/07/17 00:31:38 enami Exp $	*/

#include <sys/param.h>
#include <sys/ucred.h>
#include <sys/mount.h>

#include <err.h>
#include <stdlib.h>
#include <string.h>

#define	KB		* 1024
#define	MB		* 1024 KB
#define	GB		* 1024 MB

static struct statvfs *getnewstatvfs(void);
static void other_variants(const struct statvfs *, const int *, int,
    const int *, int);
static void setup_filer(void);
static void setup_ld0g(void);
static void setup_strpct(void);

static struct statvfs *allstatvfs;
static int sftotal, sfused;

struct statvfs *
getnewstatvfs(void)
{

	if (sftotal == sfused) {
		sftotal = sftotal ? sftotal * 2 : 1;
		allstatvfs = realloc(allstatvfs,
		    sftotal * sizeof(struct statvfs));
		if (allstatvfs == NULL)
			err(EXIT_FAILURE, "realloc");
	}

	return (&allstatvfs[sfused++]);
}

void
other_variants(const struct statvfs *tmpl, const int *minfree, int minfreecnt,
    const int *consumed, int consumedcnt)
{
	int64_t total, used;
	struct statvfs *sf;
	int i, j;

	for (i = 0; i < minfreecnt; i++)
		for (j = 0; j < consumedcnt; j++) {
			sf = getnewstatvfs();
			*sf = *tmpl;
			total = (int64_t)(u_long)sf->f_blocks * sf->f_bsize;
			used =  total * consumed[j] / 100;
			sf->f_bfree = (total - used) / sf->f_bsize;
			sf->f_bavail = (total * (100 - minfree[i]) / 100 -
			    used) / sf->f_bsize;
			sf->f_bresvd = sf->f_bfree - sf->f_bavail;
		}
}

/*
 * Parameter taken from:
 * http://mail-index.NetBSD.org/tech-userlevel/2004/03/24/0001.html
 */
void
setup_filer(void)
{
	static const struct statvfs tmpl = {
#define	BSIZE	512
#define	TOTAL	1147ULL GB
#define	USED	132ULL MB
		.f_bsize = BSIZE,
		.f_frsize = BSIZE,
		.f_blocks = TOTAL / BSIZE,
		.f_bfree = (TOTAL - USED) / BSIZE,
		.f_bavail = (TOTAL - USED) / BSIZE,
		.f_bresvd = 0,
		.f_mntfromname = "filer:/",
		.f_mntonname = "/filer",
#undef USED
#undef TOTAL
#undef BSIZE
	};
	static const int minfree[] = { 0, 5, 10, 15, };
	static const int consumed[] = { 0, 20, 60, 95, 100 };
	struct statvfs *sf;

	sf = getnewstatvfs();
	*sf = tmpl;
	other_variants(&tmpl, minfree, sizeof(minfree) / sizeof(minfree[0]),
	    consumed, sizeof(consumed) / sizeof(consumed[0]));
}

/*
 * Parameter taken from:
 * http://mail-index.NetBSD.org/current-users/2004/03/01/0038.html
 */
void
setup_ld0g(void)
{
	static const struct statvfs tmpl = {
#define	BSIZE	4096			/* Guess */
#define	TOTAL	1308726116ULL KB
#define	USED	17901268ULL KB
#define	AVAIL	1225388540ULL KB
		.f_bsize = BSIZE,
		.f_frsize = BSIZE,
		.f_blocks = TOTAL / BSIZE,
		.f_bfree = (TOTAL - USED) / BSIZE,
		.f_bavail = AVAIL / BSIZE,
		.f_bresvd = (TOTAL - USED) / BSIZE - AVAIL / BSIZE,
		.f_mntfromname = "/dev/ld0g",
		.f_mntonname = "/anon-root",
#undef AVAIL
#undef USED
#undef TOTAL
#undef BSIZE
	};
	static const int minfree[] = { 0, 5, 10, 15, };
	static const int consumed[] = { 0, 20, 60, 95, 100 };
	struct statvfs *sf;

	sf = getnewstatvfs();
	*sf = tmpl;
	other_variants(&tmpl, minfree, sizeof(minfree) / sizeof(minfree[0]),
	    consumed, sizeof(consumed) / sizeof(consumed[0]));
}

/*
 * Test of strpct() with huge number.
 */
void
setup_strpct(void)
{
	static const struct statvfs tmpl = {
#define	BSIZE	4096			/* Guess */
#define	TOTAL	0x4ffffffffULL KB
#define	USED	(TOTAL / 2)
#define	AVAIL	(TOTAL / 2)
		.f_bsize = BSIZE,
		.f_frsize = BSIZE,
		.f_blocks = TOTAL / BSIZE,
		.f_bfree = (TOTAL - USED) / BSIZE,
		.f_bavail = AVAIL / BSIZE,
		.f_bresvd = (TOTAL - USED) / BSIZE - AVAIL / BSIZE,
		.f_mntfromname = "/dev/strpct",
		.f_mntonname = "/strpct",
#undef AVAIL
#undef USED
#undef TOTAL
#undef BSIZE
	};
	struct statvfs *sf;

	sf = getnewstatvfs();
	*sf = tmpl;
}

int
getmntinfo(struct statvfs **mntbuf, int flags)
{

	setup_filer();
	setup_ld0g();
	setup_strpct();

	*mntbuf = allstatvfs;
	return (sfused);
}
