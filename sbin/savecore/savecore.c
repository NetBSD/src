/*	$NetBSD: savecore.c,v 1.60 2003/05/18 02:11:13 itojun Exp $	*/

/*-
 * Copyright (c) 1986, 1992, 1993
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
 */

#include <sys/cdefs.h>
#ifndef lint
__COPYRIGHT("@(#) Copyright (c) 1986, 1992, 1993\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)savecore.c	8.5 (Berkeley) 4/28/95";
#else
__RCSID("$NetBSD: savecore.c,v 1.60 2003/05/18 02:11:13 itojun Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/mount.h>
#include <sys/msgbuf.h>
#include <sys/syslog.h>
#include <sys/time.h>

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <nlist.h>
#include <paths.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <tzfile.h>
#include <unistd.h>
#include <util.h>
#include <limits.h>
#include <kvm.h>

extern FILE *zopen(const char *fname, const char *mode);

#define	KREAD(kd, addr, p)\
	(kvm_read(kd, addr, (char *)(p), sizeof(*(p))) != sizeof(*(p)))

struct nlist current_nl[] = {	/* Namelist for currently running system. */
#define	X_DUMPDEV	0
	{ "_dumpdev" },
#define	X_DUMPLO	1
	{ "_dumplo" },
#define	X_TIME		2
	{ "_time" },
#define	X_DUMPSIZE	3
	{ "_dumpsize" },
#define	X_VERSION	4
	{ "_version" },
#define	X_DUMPMAG	5
	{ "_dumpmag" },
#define	X_PANICSTR	6
	{ "_panicstr" },
#define	X_PANICSTART	7
	{ "_panicstart" },
#define	X_PANICEND	8
	{ "_panicend" },
#define	X_MSGBUF	9
	{ "_msgbufp" },
	{ NULL },
};
int cursyms[] = { X_DUMPDEV, X_DUMPLO, X_VERSION, X_DUMPMAG, -1 };
int dumpsyms[] = { X_TIME, X_DUMPSIZE, X_VERSION, X_PANICSTR, X_DUMPMAG, -1 };

struct nlist dump_nl[] = {	/* Name list for dumped system. */
	{ "_dumpdev" },		/* Entries MUST be the same as */
	{ "_dumplo" },		/*	those in current_nl[].  */
	{ "_time" },
	{ "_dumpsize" },
	{ "_version" },
	{ "_dumpmag" },
	{ "_panicstr" },
	{ "_panicstart" },
	{ "_panicend" },
	{ "_msgbufp" },
	{ NULL },
};

/* Types match kernel declarations. */
off_t	dumplo;				/* where dump starts on dumpdev */
u_int32_t dumpmag;			/* magic number in dump */
int	dumpsize;			/* amount of memory dumped */

const char	*kernel;		/* name of used kernel */
char	*dirname;			/* directory to save dumps in */
char	*ddname;			/* name of dump device */
dev_t	dumpdev;			/* dump device */
int	dumpfd;				/* read/write descriptor on block dev */
kvm_t	*kd_dump;			/* kvm descriptor on block dev	*/
time_t	now;				/* current date */
char	panic_mesg[1024];
long	panicstr;
char	vers[1024];
char	gzmode[3];

static int	clear, compress, force, verbose;	/* flags */

void	check_kmem(void);
int	check_space(void);
void	clear_dump(void);
int	Create(char *, int);
int	dump_exists(void);
char	*find_dev(dev_t, int);
int	get_crashtime(void);
void	kmem_setup(void);
void	Lseek(int, off_t, int);
int	main(int, char *[]);
int	Open(const char *, int rw);
char	*rawname(char *s);
void	save_core(void);
void	usage(void);
void	Write(int, void *, int);

int
main(int argc, char *argv[])
{
	int ch, level;
	char *ep;

	dirname = NULL;
	kernel = NULL;
	level = 1;		/* default to fastest gzip compression */
	gzmode[0] = 'w';

	openlog("savecore", LOG_PERROR, LOG_DAEMON);

	while ((ch = getopt(argc, argv, "cdfN:vzZ:")) != -1)
		switch(ch) {
		case 'c':
			clear = 1;
			break;
		case 'd':		/* Not documented. */
		case 'v':
			verbose = 1;
			break;
		case 'f':
			force = 1;
			break;
		case 'N':
			kernel = optarg;
			break;
		case 'z':
			compress = 1;
			break;
		case 'Z':
			level = (int)strtol(optarg, &ep, 10);
			if (level < 0 || level > 9) {
				(void)syslog(LOG_ERR, "invalid compression %s",
				    optarg);
				usage();
			}
			break;
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (argc != (clear ? 0 : 1))
		usage();

	gzmode[1] = level + '0';
	if (!clear)
		dirname = argv[0];

	if (kernel == NULL) {
		kernel = getbootfile();
	}

	(void)time(&now);
	kmem_setup();

	if (clear) {
		clear_dump();
		exit(0);
	}

	if (!dump_exists() && !force)
		exit(1);

	check_kmem();

	if (panicstr)
		syslog(LOG_ALERT, "reboot after panic: %s", panic_mesg);
	else
		syslog(LOG_ALERT, "reboot");

	if ((!get_crashtime() || !check_space()) && !force)
		exit(1);

	save_core();

	clear_dump();
	exit(0);
}

void
kmem_setup(void)
{
	kvm_t *kd_kern;
	char errbuf[_POSIX2_LINE_MAX];
	int i, hdrsz;
	
	/*
	 * Some names we need for the currently running system, others for
	 * the system that was running when the dump was made.  The values
	 * obtained from the current system are used to look for things in
	 * /dev/kmem that cannot be found in the kernel namelist, but are
	 * presumed to be the same (since the disk partitions are probably
	 * the same!)
	 */
	kd_kern = kvm_openfiles(kernel, NULL, NULL, O_RDONLY, errbuf);
	if (kd_kern == NULL) {
		syslog(LOG_ERR, "%s: kvm_openfiles: %s", kernel, errbuf);
		exit(1);
	}
	if (kvm_nlist(kd_kern, current_nl) == -1)
		syslog(LOG_ERR, "%s: kvm_nlist: %s", kernel,
		    kvm_geterr(kd_kern));
	
	for (i = 0; cursyms[i] != -1; i++)
		if (current_nl[cursyms[i]].n_value == 0) {
			syslog(LOG_ERR, "%s: %s not in namelist",
			    kernel, current_nl[cursyms[i]].n_name);
			exit(1);
		}

	if (KREAD(kd_kern, current_nl[X_DUMPDEV].n_value, &dumpdev) != 0) {
		if (verbose)
		    syslog(LOG_WARNING, "kvm_read: %s", kvm_geterr(kd_kern));
		exit(1);
	}
	if (dumpdev == NODEV) {
		syslog(LOG_WARNING, "no core dump (no dumpdev)");
		exit(1);
	}
	{
	    long l_dumplo;

	    if (KREAD(kd_kern, current_nl[X_DUMPLO].n_value, &l_dumplo) != 0) {
		    if (verbose)
			syslog(LOG_WARNING, "kvm_read: %s", kvm_geterr(kd_kern));
		    exit(1);
	    }
	    if (l_dumplo == -1) {
		syslog(LOG_WARNING, "no core dump (invalid dumplo)");
		exit(1);
	    }
	    dumplo = DEV_BSIZE * (off_t) l_dumplo;
	}

	if (verbose)
		(void)printf("dumplo = %lld (%ld * %ld)\n",
		    (long long)dumplo, (long)(dumplo / DEV_BSIZE), (long)DEV_BSIZE);
	if (KREAD(kd_kern, current_nl[X_DUMPMAG].n_value, &dumpmag) != 0) {
		if (verbose)
		    syslog(LOG_WARNING, "kvm_read: %s", kvm_geterr(kd_kern));
		exit(1);
	}

	(void)kvm_read(kd_kern, current_nl[X_VERSION].n_value, vers,
	    sizeof(vers));
	vers[sizeof(vers) - 1] = '\0';

	ddname = find_dev(dumpdev, S_IFBLK);
	dumpfd = Open(ddname, O_RDWR);

	kd_dump = kvm_openfiles(kernel, ddname, NULL, O_RDWR, errbuf);
	if (kd_dump == NULL) {
		syslog(LOG_ERR, "%s: kvm_openfiles: %s", kernel, errbuf);
		exit(1);
	}

	if (kvm_nlist(kd_dump, dump_nl) == -1)
		syslog(LOG_ERR, "%s: kvm_nlist: %s", kernel,
		    kvm_geterr(kd_dump));

	for (i = 0; dumpsyms[i] != -1; i++)
		if (dump_nl[dumpsyms[i]].n_value == 0) {
			syslog(LOG_ERR, "%s: %s not in namelist",
			    kernel, dump_nl[dumpsyms[i]].n_name);
			exit(1);
		}
	hdrsz = kvm_dump_mkheader(kd_dump, dumplo);

	/*
	 * If 'hdrsz' == 0, kvm_dump_mkheader() failed on the magic-number
	 * checks, ergo no dump is present...
	 */
	if (hdrsz == 0) {
		syslog(LOG_WARNING, "no core dump");
		exit(1);
	}
	if (hdrsz == -1) {
		syslog(LOG_ERR, "%s: kvm_dump_mkheader: %s", kernel,
		    kvm_geterr(kd_dump));
		exit(1);
	}
	dumplo += hdrsz;
	kvm_close(kd_kern);
}

void
check_kmem(void)
{
	char *cp, *bufdata;
	struct kern_msgbuf msgbuf, *bufp;
	long panicloc, panicstart, panicend;
	char core_vers[1024];

	(void)kvm_read(kd_dump, dump_nl[X_VERSION].n_value, core_vers,
	    sizeof(core_vers));
	core_vers[sizeof(core_vers) - 1] = '\0';

	if (strcmp(vers, core_vers) != 0)
		syslog(LOG_WARNING,
		    "warning: %s version mismatch:\n\t%s\nand\t%s\n",
		    kernel, vers, core_vers);

	panicstart = panicend = 0;
	if (KREAD(kd_dump, dump_nl[X_PANICSTART].n_value, &panicstart) != 0) {
		if (verbose)
		    syslog(LOG_WARNING, "kvm_read: %s", kvm_geterr(kd_dump));
		goto nomsguf;
	}
	if (KREAD(kd_dump, dump_nl[X_PANICEND].n_value, &panicend) != 0) {
		if (verbose)
		    syslog(LOG_WARNING, "kvm_read: %s", kvm_geterr(kd_dump));
		goto nomsguf;
	}
	if (panicstart != 0 && panicend != 0) {
		if (KREAD(kd_dump, dump_nl[X_MSGBUF].n_value, &bufp)) {
			if (verbose)
				syslog(LOG_WARNING, "kvm_read: %s", kvm_geterr(kd_dump));
			goto nomsguf;
		}
		if (kvm_read(kd_dump, (long)bufp, &msgbuf,
		    offsetof(struct kern_msgbuf, msg_bufc)) !=
		    offsetof(struct kern_msgbuf, msg_bufc)) {
			if (verbose)
				syslog(LOG_WARNING, "kvm_read: %s", kvm_geterr(kd_dump));
			goto nomsguf;
		}
		if (msgbuf.msg_magic != MSG_MAGIC) {
			if (verbose)
				syslog(LOG_WARNING, "msgbuf magic incorrect");
			goto nomsguf;
		}
		bufdata = malloc(msgbuf.msg_bufs);
		if (bufdata == NULL) {
			if (verbose)
				syslog(LOG_WARNING, "couldn't allocate space for msgbuf data");
			goto nomsguf;
		}
		if (kvm_read(kd_dump, (long)&bufp->msg_bufc, bufdata,
		    msgbuf.msg_bufs) != msgbuf.msg_bufs) {
			if (verbose)
				syslog(LOG_WARNING, "kvm_read: %s", kvm_geterr(kd_dump));
			goto nomsguf;
		}
		cp = panic_mesg;
		while (panicstart != panicend && cp < &panic_mesg[sizeof(panic_mesg)-1]) {
			*cp++ = bufdata[panicstart];
			panicstart++;
			if (panicstart >= msgbuf.msg_bufs)
				panicstart = 0;
		}
		/* Don't end in a new-line */
		cp = &panic_mesg[strlen(panic_mesg)] - 1;
		if (*cp == '\n')
			*cp = '\0';
		panic_mesg[sizeof(panic_mesg) - 1] = '\0';

		panicstr = 1;	/* anything not zero */
		return;
	}
nomsguf:
	if (KREAD(kd_dump, dump_nl[X_PANICSTR].n_value, &panicstr) != 0) {
		if (verbose)
		    syslog(LOG_WARNING, "kvm_read: %s", kvm_geterr(kd_dump));
		return;
	}
	if (panicstr) {
		cp = panic_mesg;
		panicloc = panicstr;
		do {
			if (KREAD(kd_dump, panicloc, cp) != 0) {
				if (verbose)
				    syslog(LOG_WARNING, "kvm_read: %s",
					kvm_geterr(kd_dump));
				break;
			}
			panicloc++;
		} while (*cp++ && cp < &panic_mesg[sizeof(panic_mesg)-1]);
		panic_mesg[sizeof(panic_mesg) - 1] = '\0';
	}
}

int
dump_exists(void)
{
	u_int32_t newdumpmag;

	if (KREAD(kd_dump, dump_nl[X_DUMPMAG].n_value, &newdumpmag) != 0) {
		if (verbose)
		    syslog(LOG_WARNING, "kvm_read: %s", kvm_geterr(kd_dump));
		return (0);
	}

	/* Read the dump size. */
	if (KREAD(kd_dump, dump_nl[X_DUMPSIZE].n_value, &dumpsize) != 0) {
		if (verbose)
		    syslog(LOG_WARNING, "kvm_read: %s", kvm_geterr(kd_dump));
		return (0);
	}
	dumpsize *= getpagesize();

	/*
	 * Return zero if core dump doesn't seem to be there, and note
	 * it for syslog.  This check and return happens after the dump size
	 * is read, so dumpsize is whether or not the core is valid (for -f).
	 */
	if (newdumpmag != dumpmag) {
		if (verbose)
			syslog(LOG_WARNING,
			    "magic number mismatch (0x%x != 0x%x)",
			    newdumpmag, dumpmag);
		syslog(LOG_WARNING, "no core dump");
		return (0);
	}
	return (1);
}

void
clear_dump(void)
{
	if (kvm_dump_inval(kd_dump) == -1)
		syslog(LOG_ERR, "%s: kvm_clear_dump: %s", ddname,
		    kvm_geterr(kd_dump));

}

char buf[1024 * 1024];

void
save_core(void)
{
	FILE *fp;
	int bounds, ifd, nr, nw, ofd;
	char *rawp, path[MAXPATHLEN];

	ofd = -1;
	/*
	 * Get the current number and update the bounds file.  Do the update
	 * now, because may fail later and don't want to overwrite anything.
	 */
	umask(066);
	(void)snprintf(path, sizeof(path), "%s/bounds", dirname);
	if ((fp = fopen(path, "r")) == NULL)
		goto err1;
	if (fgets(buf, sizeof(buf), fp) == NULL) {
		if (ferror(fp))
err1:			syslog(LOG_WARNING, "%s: %m", path);
		bounds = 0;
	} else
		bounds = atoi(buf);
	if (fp != NULL)
		(void)fclose(fp);
	if ((fp = fopen(path, "w")) == NULL)
		syslog(LOG_ERR, "%s: %m", path);
	else {
		(void)fprintf(fp, "%d\n", bounds + 1);
		(void)fclose(fp);
	}

	/* Create the core file. */
	(void)snprintf(path, sizeof(path), "%s/netbsd.%d.core%s",
	    dirname, bounds, compress ? ".gz" : "");
	if (compress) {
		if ((fp = zopen(path, gzmode)) == NULL) {
			syslog(LOG_ERR, "%s: %m", path);
			exit(1);
		}
	} else {
		ofd = Create(path, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
		fp  = fdopen(ofd, "w");
		if (fp == NULL) {
			syslog(LOG_ERR, "%s: fdopen: %m", path);
			exit(1);
		}
	}

	/* Open the raw device. */
	rawp = rawname(ddname);
	if ((ifd = open(rawp, O_RDONLY)) == -1) {
		syslog(LOG_WARNING, "%s: %m; using block device", rawp);
		ifd = dumpfd;
	}

	/* Seek to the start of the core. */
	Lseek(ifd, dumplo, SEEK_SET);

	if (kvm_dump_wrtheader(kd_dump, fp, dumpsize) == -1) {
		syslog(LOG_ERR, "kvm_dump_wrtheader: %s : %s", path,
		    kvm_geterr(kd_dump));
		exit(1);
	}

	/* Copy the core file. */
	syslog(LOG_NOTICE, "writing %score to %s",
	    compress ? "compressed " : "", path);
	for (; dumpsize > 0; dumpsize -= nr) {
		char nbuf[7];
		humanize_number(nbuf, 7, dumpsize, "", HN_AUTOSCALE, 0);
		(void)printf("%7s\r", nbuf);
		(void)fflush(stdout);
		nr = read(ifd, buf, MIN(dumpsize, sizeof(buf)));
		if (nr <= 0) {
			if (nr == 0)
				syslog(LOG_WARNING,
				    "WARNING: EOF on dump device");
			else
				syslog(LOG_ERR, "%s: %m", rawp);
			goto err2;
		}
		nw = fwrite(buf, 1, nr, fp);
		if (nw != nr) {
			syslog(LOG_ERR, "%s: %s",
			    path, strerror(nw == 0 ? EIO : errno));
err2:			syslog(LOG_WARNING,
			    "WARNING: core may be incomplete");
			(void)printf("\n");
			exit(1);
		}
	}
	(void)close(ifd);
	(void)fclose(fp);

	/* Copy the kernel. */
	ifd = Open(kernel, O_RDONLY);
	(void)snprintf(path, sizeof(path), "%s/netbsd.%d%s",
	    dirname, bounds, compress ? ".gz" : "");
	if (compress) {
		if ((fp = zopen(path, gzmode)) == NULL) {
			syslog(LOG_ERR, "%s: %m", path);
			exit(1);
		}
	} else
		ofd = Create(path, S_IRUSR | S_IWUSR);
	syslog(LOG_NOTICE, "writing %skernel to %s",
	    compress ? "compressed " : "", path);
	while ((nr = read(ifd, buf, sizeof(buf))) > 0) {
		if (compress)
			nw = fwrite(buf, 1, nr, fp);
		else
			nw = write(ofd, buf, nr);
		if (nw != nr) {
			syslog(LOG_ERR, "%s: %s",
			    path, strerror(nw == 0 ? EIO : errno));
			syslog(LOG_WARNING,
			    "WARNING: kernel may be incomplete");
			exit(1);
		}
	}
	if (nr < 0) {
		syslog(LOG_ERR, "%s: %m", kernel);
		syslog(LOG_WARNING, "WARNING: kernel may be incomplete");
		exit(1);
	}
	if (compress)
		(void)fclose(fp);
	else
		(void)close(ofd);
}

char *
find_dev(dev_t dev, int type)
{
	DIR *dfd;
	struct dirent *dir;
	struct stat sb;
	char *dp, device[MAXPATHLEN + 1], *p;
	size_t l;

	if ((dfd = opendir(_PATH_DEV)) == NULL) {
		syslog(LOG_ERR, "%s: %m", _PATH_DEV);
		exit(1);
	}
	strlcpy(device, _PATH_DEV, sizeof(device));
	p = &device[strlen(device)];
	l = sizeof(device) - strlen(device);
	while ((dir = readdir(dfd))) {
		strlcpy(p, dir->d_name, l);
		if (lstat(device, &sb)) {
			syslog(LOG_ERR, "%s: %m", device);
			continue;
		}
		if ((sb.st_mode & S_IFMT) != type)
			continue;
		if (dev == sb.st_rdev) {
			closedir(dfd);
			if ((dp = strdup(device)) == NULL) {
				syslog(LOG_ERR, "%m");
				exit(1);
			}
			return (dp);
		}
	}
	closedir(dfd);
	syslog(LOG_ERR, "can't find device %d/%d", major(dev), minor(dev));
	exit(1);
}

char *
rawname(char *s)
{
	char *sl;
	char name[MAXPATHLEN];

	if ((sl = strrchr(s, '/')) == NULL || sl[1] == '0') {
		syslog(LOG_ERR,
		    "can't make raw dump device name from %s", s);
		return (s);
	}
	(void)snprintf(name, sizeof(name), "%.*s/r%s", (int)(sl - s), s,
	    sl + 1);
	if ((sl = strdup(name)) == NULL) {
		syslog(LOG_ERR, "%m");
		exit(1);
	}
	return (sl);
}

int
get_crashtime(void)
{
	struct timeval dtime;
	time_t dumptime;			/* Time the dump was taken. */

	if (KREAD(kd_dump, dump_nl[X_TIME].n_value, &dtime) != 0) {
		if (verbose)
		    syslog(LOG_WARNING, "kvm_read: %s", kvm_geterr(kd_dump));
		return (0);
	}
	dumptime = dtime.tv_sec;
	if (dumptime == 0) {
		if (verbose)
			syslog(LOG_ERR, "dump time is zero");
		return (0);
	}
	(void)printf("savecore: system went down at %s", ctime(&dumptime));
#define	LEEWAY	(7 * SECSPERDAY)
	if (dumptime < now - LEEWAY || dumptime > now + LEEWAY) {
		(void)printf("dump time is unreasonable\n");
		return (0);
	}
	return (1);
}

int
check_space(void)
{
	FILE *fp;
	off_t minfree, spacefree, kernelsize, needed;
	struct stat st;
	struct statfs fsbuf;
	char mbuf[100], path[MAXPATHLEN];

#ifdef __GNUC__
	(void) &minfree;
#endif

	if (stat(kernel, &st) < 0) {
		syslog(LOG_ERR, "%s: %m", kernel);
		exit(1);
	}
	kernelsize = st.st_blocks * S_BLKSIZE;
	if (statfs(dirname, &fsbuf) < 0) {
		syslog(LOG_ERR, "%s: %m", dirname);
		exit(1);
	}
	spacefree = fsbuf.f_bavail;
	spacefree *= fsbuf.f_bsize;
	spacefree /= 1024;

	(void)snprintf(path, sizeof(path), "%s/minfree", dirname);
	if ((fp = fopen(path, "r")) == NULL)
		minfree = 0;
	else {
		if (fgets(mbuf, sizeof(mbuf), fp) == NULL)
			minfree = 0;
		else
			minfree = atoi(mbuf);
		(void)fclose(fp);
	}

	needed = (dumpsize + kernelsize) / 1024;
 	if (minfree > 0 && spacefree - needed < minfree) {
		syslog(LOG_WARNING,
		    "no dump, not enough free space in %s", dirname);
		return (0);
	}
	if (spacefree - needed < minfree)
		syslog(LOG_WARNING,
		    "dump performed, but free space threshold crossed");
	return (1);
}

int
Open(const char *name, int rw)
{
	int fd;

	if ((fd = open(name, rw, 0)) < 0) {
		syslog(LOG_ERR, "%s: %m", name);
		exit(1);
	}
	return (fd);
}

void
Lseek(int fd, off_t off, int flag)
{
	off_t ret;

	ret = lseek(fd, off, flag);
	if (ret == -1) {
		syslog(LOG_ERR, "lseek: %m");
		exit(1);
	}
}

int
Create(char *file, int mode)
{
	int fd;

	fd = open(file, O_WRONLY | O_CREAT | O_TRUNC, mode);
	if (fd < 0) {
		syslog(LOG_ERR, "%s: %m", file);
		exit(1);
	}
	return (fd);
}

void
Write(int fd, void *bp, int size)
{
	int n;

	if ((n = write(fd, bp, size)) < size) {
		syslog(LOG_ERR, "write: %s", strerror(n == -1 ? errno : EIO));
		exit(1);
	}
}

void
usage(void)
{
	(void)syslog(LOG_ERR,
	    "usage: savecore [-cfvz] [-N system] [-Z level] directory");
	exit(1);
}
