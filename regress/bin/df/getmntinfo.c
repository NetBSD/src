/*	$NetBSD: getmntinfo.c,v 1.1 2004/03/26 14:53:39 enami Exp $	*/

#include <sys/param.h>
#include <sys/ucred.h>
#include <sys/mount.h>

#include <err.h>
#include <stdlib.h>
#include <string.h>

#define	KB		* 1024
#define	MB		* 1024 KB
#define	GB		* 1024 MB

static struct statfs *getnewstatfs(void);
static void other_variants(const struct statfs *, const int *, int,
    const int *, int);
static void setup_filer(void);
static void setup_ld0g(void);

static struct statfs *allstatfs;
static int sftotal, sfused;

struct statfs *
getnewstatfs(void)
{

	if (sftotal == sfused) {
		sftotal = sftotal ? sftotal * 2 : 1;
		allstatfs = realloc(allstatfs,
		    sftotal * sizeof(struct statfs));
		if (allstatfs == NULL)
			err(EXIT_FAILURE, "realloc");
	}

	return (&allstatfs[sfused++]);
}

void
other_variants(const struct statfs *tmpl, const int *minfree, int minfreecnt,
    const int *consumed, int consumedcnt)
{
	int64_t total, used;
	struct statfs *sf;
	int i, j;

	for (i = 0; i < minfreecnt; i++)
		for (j = 0; j < consumedcnt; j++) {
			sf = getnewstatfs();
			*sf = *tmpl;
			total = (int64_t)(u_long)sf->f_blocks * sf->f_bsize;
			used =  total * consumed[j] / 100;
			sf->f_bfree = (total - used) / sf->f_bsize;
			sf->f_bavail = (total * (100 - minfree[i]) / 100 -
			    used) / sf->f_bsize;
		}
}

/*
 * Parameter taken from:
 * http://mail-index.NetBSD.org/tech-userlevel/2004/03/24/0001.html
 */
void
setup_filer(void)
{
	static const struct statfs tmpl = {
#define	BSIZE	512
#define	TOTAL	1147ULL GB
#define	USED	132ULL MB
		.f_bsize = BSIZE,
		.f_blocks = TOTAL / BSIZE,
		.f_bfree = (TOTAL - USED) / BSIZE,
		.f_bavail = (TOTAL - USED) / BSIZE,
		.f_mntfromname = "filer:/",
		.f_mntonname = "/filer",
#undef USED
#undef TOTAL
#undef BSIZE
	};
	static const int minfree[] = { 0, 5, 10, 15, };
	static const int consumed[] = { 0, 20, 60, 95, 100 };
	struct statfs *sf;

	sf = getnewstatfs();
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
	static const struct statfs tmpl = {
#define	BSIZE	4096			/* Guess */
#define	TOTAL	1308726116ULL KB
#define	USED	17901268ULL KB
#define	AVAIL	1225388540ULL KB
		.f_bsize = BSIZE,
		.f_blocks = TOTAL / BSIZE,
		.f_bfree = (TOTAL - USED) / BSIZE,
		.f_bavail = AVAIL / BSIZE,
		.f_mntfromname = "/dev/ld0g",
		.f_mntonname = "/anon-root",
#undef AVAIL
#undef USED
#undef TOTAL
#undef BSIZE
	};
	static const int minfree[] = { 0, 5, 10, 15, };
	static const int consumed[] = { 0, 20, 60, 95, 100 };
	struct statfs *sf;

	sf = getnewstatfs();
	*sf = tmpl;
	other_variants(&tmpl, minfree, sizeof(minfree) / sizeof(minfree[0]),
	    consumed, sizeof(consumed) / sizeof(consumed[0]));
}

int
getmntinfo(struct statfs **mntbuf, int flags)
{

	setup_filer();
	setup_ld0g();

	*mntbuf = allstatfs;
	return (sfused);
}
