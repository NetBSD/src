/*	$NetBSD: tape.c,v 1.61.2.1 2009/05/13 19:19:05 jym Exp $	*/

/*
 * Copyright (c) 1983, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
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
#if 0
static char sccsid[] = "@(#)tape.c	8.9 (Berkeley) 5/1/95";
#else
__RCSID("$NetBSD: tape.c,v 1.61.2.1 2009/05/13 19:19:05 jym Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/mtio.h>
#include <sys/stat.h>

#include <ufs/ufs/dinode.h>
#include <protocols/dumprestore.h>

#include <err.h>
#include <errno.h>
#include <paths.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <md5.h>
#include <rmd160.h>
#include <sha1.h>

#include "restore.h"
#include "extern.h"

static u_int32_t fssize = MAXBSIZE;
static int	mt = -1;
static int	pipein = 0;
static char	magtape[BUFSIZ];
static int	blkcnt;
static int	numtrec;
static char	*tapebuf;
static union	u_spcl endoftapemark;
static int	blksread;		/* blocks read since last header */
static int	tpblksread = 0;		/* TP_BSIZE blocks read */
static int	tapesread;
static jmp_buf	restart;
static int	gettingfile = 0;	/* restart has a valid frame */
#ifdef RRESTORE
static const char *host = NULL;
#endif

static int	ofile;
static char	lnkbuf[MAXPATHLEN + 1];
static int	pathlen;

int		oldinofmt;	/* old inode format conversion required */
int		Bcvt;		/* Swap Bytes (for CCI or sun) */

const struct digest_desc *ddesc;
const struct digest_desc digest_descs[] = {
	{ "MD5",
	  (void (*)(void *))MD5Init,
	  (void (*)(void *, const u_char *, u_int))MD5Update,
	  (char *(*)(void *, void *))MD5End, },
	{ "SHA1",
	  (void (*)(void *))SHA1Init,
	  (void (*)(void *, const u_char *, u_int))SHA1Update,
	  (char *(*)(void *, void *))SHA1End, },
	{ "RMD160",
	  (void (*)(void *))RMD160Init,
	  (void (*)(void *, const u_char *, u_int))RMD160Update,
	  (char *(*)(void *, void *))RMD160End, },
	{ .dd_name = NULL },
};

static union digest_context {
	MD5_CTX dc_md5;
	SHA1_CTX dc_sha1;
	RMD160_CTX dc_rmd160;
} dcontext;

union digest_buffer {
	char db_md5[32 + 1];
	char db_sha1[40 + 1];
	char db_rmd160[40 + 1];
};

#define	FLUSHTAPEBUF()	blkcnt = ntrec + 1

union u_ospcl {
	char dummy[TP_BSIZE];
	struct	s_ospcl {
		int32_t   c_type;
		int32_t   c_date;
		int32_t   c_ddate;
		int32_t   c_volume;
		int32_t   c_tapea;
		u_int16_t c_inumber;
		int32_t   c_magic;
		int32_t   c_checksum;
		struct odinode {
			unsigned short odi_mode;
			u_int16_t odi_nlink;
			u_int16_t odi_uid;
			u_int16_t odi_gid;
			int32_t   odi_size;
			int32_t   odi_rdev;
			char      odi_addr[36];
			int32_t   odi_atime;
			int32_t   odi_mtime;
			int32_t   odi_ctime;
		} c_odinode;
		int32_t c_count;
		char    c_addr[256];
	} s_ospcl;
};

static void	 accthdr(struct s_spcl *);
static int	 checksum(int *);
static void	 findinode(struct s_spcl *);
static void	 findtapeblksize(void);
static void	 getbitmap(char **);
static int	 gethead(struct s_spcl *);
static void	 readtape(char *);
static void	 setdumpnum(void);
static void	 terminateinput(void);
static void	 xtrfile(char *, long);
static void	 xtrlnkfile(char *, long);
static void	 xtrlnkskip(char *, long);
static void	 xtrskip(char *, long);
static void	 swap_header(struct s_spcl *);
static void	 swap_old_header(struct s_ospcl *);

const struct digest_desc *
digest_lookup(const char *name)
{
	const struct digest_desc *dd;

	for (dd = digest_descs; dd->dd_name != NULL; dd++)
		if (strcasecmp(dd->dd_name, name) == 0)
			return (dd);

	return (NULL);
}

/*
 * Set up an input source
 */
void
setinput(const char *source)
{
	char *cp;
	FLUSHTAPEBUF();
	if (bflag)
		newtapebuf(ntrec);
	else
		newtapebuf(NTREC > HIGHDENSITYTREC ? NTREC : HIGHDENSITYTREC);
	terminal = stdin;

#ifdef RRESTORE
	if ((cp = strchr(source, ':')) != NULL) {
		host = source;
		/* Ok, because const strings don't have : */
		*cp++ = '\0';
		source = cp;
		if (rmthost(host) == 0)
			exit(1);
	} else
#endif
	if (strcmp(source, "-") == 0) {
		/*
		 * Since input is coming from a pipe we must establish
		 * our own connection to the terminal.
		 */
		terminal = fopen(_PATH_TTY, "r");
		if (terminal == NULL) {
			(void)fprintf(stderr, "cannot open %s: %s\n",
			    _PATH_TTY, strerror(errno));
			terminal = fopen(_PATH_DEVNULL, "r");
			if (terminal == NULL) {
				(void)fprintf(stderr, "cannot open %s: %s\n",
				    _PATH_DEVNULL, strerror(errno));
				exit(1);
			}
		}
		pipein++;
	}
	(void) strcpy(magtape, source);
}

void
newtapebuf(long size)
{
	static int tapebufsize = -1;

	ntrec = size;
	if (size <= tapebufsize)
		return;
	if (tapebuf != NULL)
		free(tapebuf);
	tapebuf = malloc(size * TP_BSIZE);
	if (tapebuf == NULL) {
		fprintf(stderr, "Cannot allocate space for tape buffer\n");
		exit(1);
	}
	tapebufsize = size;
}

/*
 * Verify that the tape drive can be accessed and
 * that it actually is a dump tape.
 */
void
setup(void)
{
	int i, j, *ip;
	struct stat stbuf;

	vprintf(stdout, "Verify tape and initialize maps\n");
#ifdef RRESTORE
	if (host)
		mt = rmtopen(magtape, 0, 0);
	else
#endif
	if (pipein)
		mt = 0;
	else
		mt = open(magtape, O_RDONLY, 0);
	if (mt < 0) {
		fprintf(stderr, "%s: %s\n", magtape, strerror(errno));
		exit(1);
	}
	volno = 1;
	setdumpnum();
	FLUSHTAPEBUF();
	if (!pipein && !bflag)
		findtapeblksize();
	if (gethead(&spcl) == FAIL) {
		blkcnt--; /* push back this block */
		blksread--;
		tpblksread--;
		cvtflag++;
		if (gethead(&spcl) == FAIL) {
			fprintf(stderr, "Tape is not a dump tape\n");
			exit(1);
		}
		fprintf(stderr, "Converting to new file system format.\n");
	}
	if (pipein) {
		endoftapemark.s_spcl.c_magic = cvtflag ? OFS_MAGIC :
		    FS_UFS2_MAGIC;
		endoftapemark.s_spcl.c_type = TS_END;
		ip = (int *)&endoftapemark;
		j = sizeof(union u_spcl) / sizeof(int);
		i = 0;
		do
			i += *ip++;
		while (--j);
		endoftapemark.s_spcl.c_checksum = CHECKSUM - i;
	}
	if (vflag || command == 't')
		printdumpinfo();
	dumptime = spcl.c_ddate;
	dumpdate = spcl.c_date;
	if (stat(".", &stbuf) < 0) {
		fprintf(stderr, "cannot stat .: %s\n", strerror(errno));
		exit(1);
	}
	if (stbuf.st_blksize >= TP_BSIZE && stbuf.st_blksize <= MAXBSIZE)
		fssize = stbuf.st_blksize;
	if (((fssize - 1) & fssize) != 0) {
		fprintf(stderr, "bad block size %d\n", fssize);
		exit(1);
	}
	if (spcl.c_volume != 1) {
		fprintf(stderr, "Tape is not volume 1 of the dump\n");
		exit(1);
	}
	if (gethead(&spcl) == FAIL) {
		dprintf(stdout, "header read failed at %d blocks\n", blksread);
		panic("no header after volume mark!\n");
	}
	findinode(&spcl);
	if (spcl.c_type != TS_CLRI) {
		fprintf(stderr, "Cannot find file removal list\n");
		exit(1);
	}
	getbitmap(&usedinomap);
	getbitmap(&dumpmap);
	/*
	 * If there may be whiteout entries on the tape, pretend that the
	 * whiteout inode exists, so that the whiteout entries can be
	 * extracted.
	 */
	if (oldinofmt == 0)
		SETINO(WINO, dumpmap);
}

/*
 * Prompt user to load a new dump volume.
 * "Nextvol" is the next suggested volume to use.
 * This suggested volume is enforced when doing full
 * or incremental restores, but can be overrridden by
 * the user when only extracting a subset of the files.
 */
void
getvol(int nextvol)
{
	int newvol, savecnt, wantnext, i;
	union u_spcl tmpspcl;
#	define tmpbuf tmpspcl.s_spcl
	char buf[TP_BSIZE];

	newvol = savecnt = wantnext = 0;
	if (nextvol == 1) {
		tapesread = 0;
		gettingfile = 0;
	}
	if (pipein) {
		if (nextvol != 1)
			panic("Changing volumes on pipe input?\n");
		if (volno == 1)
			return;
		goto gethdr;
	}
	savecnt = blksread;
again:
	if (pipein)
		exit(1); /* pipes do not get a second chance */
	if (command == 'R' || command == 'r' || curfile.action != SKIP) {
		newvol = nextvol;
		wantnext = 1;
	} else { 
		newvol = 0;
		wantnext = 0;
	}
	while (newvol <= 0) {
		if (tapesread == 0) {
			fprintf(stderr, "%s%s%s%s%s",
			    "You have not read any tapes yet.\n",
			    "Unless you know which volume your",
			    " file(s) are on you should start\n",
			    "with the last volume and work",
			    " towards the first.\n");
			fprintf(stderr,
			    "(Use 1 for the first volume/tape, etc.)\n");
		} else {
			fprintf(stderr, "You have read volumes");
			strcpy(buf, ": ");
			for (i = 1; i < 32; i++)
				if (tapesread & (1 << i)) {
					fprintf(stderr, "%s%d", buf, i);
					strcpy(buf, ", ");
				}
			fprintf(stderr, "\n");
		}
		do	{
			fprintf(stderr, "Specify next volume #: ");
			(void) fflush(stderr);
			(void) fgets(buf, BUFSIZ, terminal);
		} while (!feof(terminal) && buf[0] == '\n');
		if (feof(terminal))
			exit(1);
		newvol = atoi(buf);
		if (newvol <= 0) {
			fprintf(stderr,
			    "Volume numbers are positive numerics\n");
		}
	}
	if (newvol == volno) {
		tapesread |= 1 << volno;
		return;
	}
	closemt();
	fprintf(stderr, "Mount tape volume %d\n", newvol);
	fprintf(stderr, "Enter ``none'' if there are no more tapes\n");
	fprintf(stderr, "otherwise enter tape name (default: %s) ", magtape);
	(void) fflush(stderr);
	(void) fgets(buf, BUFSIZ, terminal);
	if (feof(terminal))
		exit(1);
	if (!strcmp(buf, "none\n")) {
		terminateinput();
		return;
	}
	if (buf[0] != '\n') {
		(void) strcpy(magtape, buf);
		magtape[strlen(magtape) - 1] = '\0';
	}
#ifdef RRESTORE
	if (host)
		mt = rmtopen(magtape, 0, 0);
	else
#endif
		mt = open(magtape, O_RDONLY, 0);

	if (mt == -1) {
		fprintf(stderr, "Cannot open %s\n", magtape);
		volno = -1;
		goto again;
	}
gethdr:
	volno = newvol;
	setdumpnum();
	FLUSHTAPEBUF();
	if (gethead(&tmpbuf) == FAIL) {
		dprintf(stdout, "header read failed at %d blocks\n", blksread);
		fprintf(stderr, "tape is not dump tape\n");
		volno = 0;
		goto again;
	}
	if (tmpbuf.c_volume != volno) {
	  	fprintf(stderr,
		"Volume mismatch: expecting %d, tape header claims it is %d\n",
		    volno, tmpbuf.c_volume);
		volno = 0;
		goto again;
	}
	if (tmpbuf.c_date != dumpdate || tmpbuf.c_ddate != dumptime) {
		time_t ttime = tmpbuf.c_date;
		fprintf(stderr, "Wrong dump date\n\tgot: %s",
			ctime(&ttime));
		fprintf(stderr, "\twanted: %s", ctime(&dumpdate));
		volno = 0;
		goto again;
	}
	tapesread |= 1 << volno;
	blksread = savecnt;
 	/*
 	 * If continuing from the previous volume, skip over any
 	 * blocks read already at the end of the previous volume.
 	 *
 	 * If coming to this volume at random, skip to the beginning
 	 * of the next record.
 	 */
	dprintf(stdout, "read %ld recs, tape starts with %ld\n", 
		(long)tpblksread, (long)tmpbuf.c_firstrec);
 	if (tmpbuf.c_type == TS_TAPE && (tmpbuf.c_flags & DR_NEWHEADER)) {
 		if (!wantnext) {
 			tpblksread = tmpbuf.c_firstrec;
 			for (i = tmpbuf.c_count; i > 0; i--)
 				readtape(buf);
 		} else if (tmpbuf.c_firstrec > 0 &&
			   tmpbuf.c_firstrec < tpblksread - 1) {
			/*
			 * -1 since we've read the volume header
			 */
 			i = tpblksread - tmpbuf.c_firstrec - 1;
			dprintf(stderr, "Skipping %d duplicate record%s.\n",
				i, i > 1 ? "s" : "");
 			while (--i >= 0)
 				readtape(buf);
 		}
 	}
	if (curfile.action == USING) {
		if (volno == 1)
			panic("active file into volume 1\n");
		return;
	}
	/*
	 * Skip up to the beginning of the next record
	 */
	if (tmpbuf.c_type == TS_TAPE && (tmpbuf.c_flags & DR_NEWHEADER))
		for (i = tmpbuf.c_count; i > 0; i--)
			readtape(buf);
	(void) gethead(&spcl);
	findinode(&spcl);
	if (gettingfile) {
		gettingfile = 0;
		longjmp(restart, 1);
	}
}

/*
 * Handle unexpected EOF.
 */
static void
terminateinput(void)
{

	if (gettingfile && curfile.action == USING) {
		printf("Warning: %s %s\n",
		    "End-of-input encountered while extracting", curfile.name);
	}
	curfile.name = "<name unknown>";
	curfile.action = UNKNOWN;
	curfile.mode = 0;
	curfile.ino = maxino;
	if (gettingfile) {
		gettingfile = 0;
		longjmp(restart, 1);
	}
}

/*
 * handle multiple dumps per tape by skipping forward to the
 * appropriate one.
 */
static void
setdumpnum(void)
{
	struct mtop tcom;

	if (dumpnum == 1 || volno != 1)
		return;
	if (pipein) {
		fprintf(stderr, "Cannot have multiple dumps on pipe input\n");
		exit(1);
	}
	tcom.mt_op = MTFSF;
	tcom.mt_count = dumpnum - 1;
#ifdef RRESTORE
	if (host)
		rmtioctl(MTFSF, dumpnum - 1);
	else 
#endif
		if (ioctl(mt, (int)MTIOCTOP, (char *)&tcom) < 0)
			fprintf(stderr, "ioctl MTFSF: %s\n", strerror(errno));
}

void
printdumpinfo(void)
{
	time_t ttime;

	ttime = spcl.c_date;
	fprintf(stdout, "Dump   date: %s", ctime(&ttime));
	ttime = spcl.c_ddate;
	fprintf(stdout, "Dumped from: %s",
	    (spcl.c_ddate == 0) ? "the epoch\n" : ctime(&ttime));
	fprintf(stderr, "Level %d dump of %s on %s:%s\n",
		spcl.c_level, spcl.c_filesys, 
		*spcl.c_host? spcl.c_host: "[unknown]", spcl.c_dev);
	fprintf(stderr, "Label: %s\n", spcl.c_label);

	if (Mtreefile) {
		ttime = spcl.c_date;
		fprintf(Mtreefile, "#Dump   date: %s", ctime(&ttime));
		ttime = spcl.c_ddate;
		fprintf(Mtreefile, "#Dumped from: %s",
		    (spcl.c_ddate == 0) ? "the epoch\n" : ctime(&ttime));
		fprintf(Mtreefile, "#Level %d dump of %s on %s:%s\n",
			spcl.c_level, spcl.c_filesys, 
			*spcl.c_host? spcl.c_host: "[unknown]", spcl.c_dev);
		fprintf(Mtreefile, "#Label: %s\n", spcl.c_label);
		fprintf(Mtreefile, "/set uname=root gname=wheel\n");
		if (ferror(Mtreefile))
			err(1, "error writing to mtree file");
	}
}

int
extractfile(char *name)
{
	union digest_buffer dbuffer;
	int flags;
	uid_t uid;
	gid_t gid;
	mode_t mode;
	struct timeval mtimep[2], ctimep[2];
	struct entry *ep;
	int setbirth;

	curfile.name = name;
	curfile.action = USING;
	mtimep[0].tv_sec = curfile.atime_sec;
	mtimep[0].tv_usec = curfile.atime_nsec / 1000;
	mtimep[1].tv_sec = curfile.mtime_sec;
	mtimep[1].tv_usec = curfile.mtime_nsec / 1000;

	setbirth = curfile.birthtime_sec != 0;

	if (setbirth) {
		ctimep[0].tv_sec = curfile.atime_sec;
		ctimep[0].tv_usec = curfile.atime_nsec / 1000;
		ctimep[1].tv_sec = curfile.birthtime_sec;
		ctimep[1].tv_usec = curfile.birthtime_nsec / 1000;
	}
	uid = curfile.uid;
	gid = curfile.gid;
	mode = curfile.mode;
	flags = curfile.file_flags;
	switch (mode & IFMT) {

	default:
		fprintf(stderr, "%s: unknown file mode 0%o\n", name, mode);
		skipfile();
		return (FAIL);

	case IFSOCK:
		vprintf(stdout, "skipped socket %s\n", name);
		skipfile();
		return (GOOD);

	case IFDIR:
		if (mflag) {
			ep = lookupname(name);
			if (ep == NULL || ep->e_flags & EXTRACT)
				panic("unextracted directory %s\n", name);
			skipfile();
			return (GOOD);
		}
		vprintf(stdout, "extract file %s\n", name);
		return (genliteraldir(name, curfile.ino));

	case IFLNK:
		lnkbuf[0] = '\0';
		pathlen = 0;
		getfile(xtrlnkfile, xtrlnkskip);
		if (pathlen == 0) {
			vprintf(stdout,
			    "%s: zero length symbolic link (ignored)\n", name);
			return (GOOD);
		}
		if (uflag)
			(void) unlink(name);
		if (linkit(lnkbuf, name, SYMLINK) == GOOD) {
			if (setbirth)
				(void) lutimes(name, ctimep);
			(void) lutimes(name, mtimep);
			(void) lchown(name, uid, gid);
			(void) lchmod(name, mode);
			if (Mtreefile) {
				writemtree(name, "link",
				    uid, gid, mode, flags);
			} else 
				(void) lchflags(name, flags);
			return (GOOD);
		}
		return (FAIL);

	case IFCHR:
	case IFBLK:
		vprintf(stdout, "extract special file %s\n", name);
		if (Nflag) {
			skipfile();
			return (GOOD);
		}
		if (uflag)
			(void) unlink(name);
		if (mknod(name, (mode & (IFCHR | IFBLK)) | 0600,
		    (int)curfile.rdev) < 0) {
			fprintf(stderr, "%s: cannot create special file: %s\n",
			    name, strerror(errno));
			skipfile();
			return (FAIL);
		}
		skipfile();
		if (setbirth)
			(void) utimes(name, ctimep);
		(void) utimes(name, mtimep);
		(void) chown(name, uid, gid);
		(void) chmod(name, mode);
		if (Mtreefile) {
			writemtree(name,
			    ((mode & (S_IFBLK | IFCHR)) == IFBLK) ?
			    "block" : "char",
			    uid, gid, mode, flags);
		} else 
			(void) chflags(name, flags);
		return (GOOD);

	case IFIFO:
		vprintf(stdout, "extract fifo %s\n", name);
		if (Nflag) {
			skipfile();
			return (GOOD);
		}
		if (uflag)
			(void) unlink(name);
		if (mkfifo(name, 0600) < 0) {
			fprintf(stderr, "%s: cannot create fifo: %s\n",
			    name, strerror(errno));
			skipfile();
			return (FAIL);
		}
		skipfile();
		if (setbirth)
			(void) utimes(name, ctimep);
		(void) utimes(name, mtimep);
		(void) chown(name, uid, gid);
		(void) chmod(name, mode);
		if (Mtreefile) {
			writemtree(name, "fifo",
			    uid, gid, mode, flags);
		} else 
			(void) chflags(name, flags);
		return (GOOD);

	case IFREG:
		vprintf(stdout, "extract file %s\n", name);
		if (uflag)
			(void) unlink(name);
		if (!Nflag && (ofile = open(name, O_WRONLY | O_CREAT | O_TRUNC,
		    0600)) < 0) {
			fprintf(stderr, "%s: cannot create file: %s\n",
			    name, strerror(errno));
			skipfile();
			return (FAIL);
		}
		if (Dflag)
			(*ddesc->dd_init)(&dcontext);
		getfile(xtrfile, xtrskip);
		if (Dflag) {
			(*ddesc->dd_end)(&dcontext, &dbuffer);
			for (ep = lookupname(name); ep != NULL;
			    ep = ep->e_links)
				fprintf(stdout, "%s (%s) = %s\n",
				    ddesc->dd_name, myname(ep),
				    (char *)&dbuffer);
		}
		if (Nflag)
			return (GOOD);
		if (setbirth)
			(void) futimes(ofile, ctimep);
		(void) futimes(ofile, mtimep);
		(void) fchown(ofile, uid, gid);
		(void) fchmod(ofile, mode);
		if (Mtreefile) {
			writemtree(name, "file",
			    uid, gid, mode, flags);
		} else 
			(void) fchflags(ofile, flags);
		(void) close(ofile);
		return (GOOD);
	}
	/* NOTREACHED */
}

/*
 * skip over bit maps on the tape
 */
void
skipmaps(void)
{

	while (spcl.c_type == TS_BITS || spcl.c_type == TS_CLRI)
		skipfile();
}

/*
 * skip over a file on the tape
 */
void
skipfile(void)
{

	curfile.action = SKIP;
	getfile(xtrnull, xtrnull);
}

/*
 * Extract a bitmap from the tape.
 * The first bitmap sets maxino;
 * other bitmaps must be of same size.
 */
void
getbitmap(char **map)
{
	int i;
	size_t volatile size = spcl.c_size;
	size_t volatile mapsize = size;
	char *mapptr;

	curfile.action = USING;
	if (spcl.c_type == TS_END)
		panic("ran off end of tape\n");
	if (spcl.c_magic != FS_UFS2_MAGIC)
		panic("not at beginning of a file\n");
	if (!gettingfile && setjmp(restart) != 0)
		return;
	gettingfile++;
	mapptr = *map = malloc(size);
loop:
	if (*map == NULL)
		panic("no memory for %s\n", curfile.name);
	for (i = 0; i < spcl.c_count && size >= TP_BSIZE; i++) {
		readtape(mapptr);
		mapptr += TP_BSIZE;
		size -= TP_BSIZE;
	}
	if (size != 0 || i != spcl.c_count)
		panic("%s: inconsistent map size\n", curfile.name);
	if (gethead(&spcl) == GOOD && spcl.c_type == TS_ADDR) {
		size = spcl.c_count * TP_BSIZE;
		*map = realloc(*map, mapsize + size);
		mapptr = *map + mapsize;
		mapsize += size;
		goto loop;
	}
	if (maxino == 0)
		maxino = mapsize * NBBY + 1;
	else if (maxino != mapsize * NBBY + 1)
		panic("%s: map size changed\n", curfile.name);
	findinode(&spcl);
	gettingfile = 0;
}

/*
 * Extract a file from the tape.
 * When an allocated block is found it is passed to the fill function;
 * when an unallocated block (hole) is found, a zeroed buffer is passed
 * to the skip function.
 */
void
getfile(void (*fill)(char *buf, long size),
	void (*skip)(char *buf, long size))
{
	int i;
	int volatile curblk;
	quad_t volatile size;
	static char clearedbuf[MAXBSIZE];
	char buf[MAXBSIZE / TP_BSIZE][TP_BSIZE];
	char junk[TP_BSIZE];

	curblk = 0;
	size = spcl.c_size;

	if (spcl.c_type == TS_END)
		panic("ran off end of tape\n");
	if (spcl.c_magic != FS_UFS2_MAGIC)
		panic("not at beginning of a file\n");
	if (!gettingfile && setjmp(restart) != 0)
		return;
	gettingfile++;
loop:
	for (i = 0; i < spcl.c_count; i++) {
		if (spcl.c_addr[i]) {
			readtape(&buf[curblk++][0]);
			if ((uint32_t)curblk == fssize / TP_BSIZE) {
				(*fill)((char *)buf, (long)(size > TP_BSIZE ?
				     fssize : (curblk - 1) * TP_BSIZE + size));
				curblk = 0;
			}
		} else {
			if (curblk > 0) {
				(*fill)((char *)buf, (long)(size > TP_BSIZE ?
				     curblk * TP_BSIZE :
				     (curblk - 1) * TP_BSIZE + size));
				curblk = 0;
			}
			(*skip)(clearedbuf, (long)(size > TP_BSIZE ?
				TP_BSIZE : size));
		}
		if ((size -= TP_BSIZE) <= 0) {
			for (i++; i < spcl.c_count; i++)
				if (spcl.c_addr[i])
					readtape(junk);
			break;
		}
	}
	if (gethead(&spcl) == GOOD && size > 0) {
		if (spcl.c_type == TS_ADDR)
			goto loop;
		dprintf(stdout,
			"Missing address (header) block for %s at %d blocks\n",
			curfile.name, blksread);
	}
	if (curblk > 0)
		(*fill)((char *)buf, (long)((curblk * TP_BSIZE) + size));
	/* Skip over Linux extended attributes. */
	if (spcl.c_type == TS_INODE && (spcl.c_flags & DR_EXTATTRIBUTES)) {
		for (i = 0; i < spcl.c_count; i++)
			readtape(junk);
		(void)gethead(&spcl);
	}
	findinode(&spcl);
	gettingfile = 0;
}

/*
 * Write out the next block of a file.
 */
static void
xtrfile(char *buf, long size)
{

	if (Dflag)
		(*ddesc->dd_update)(&dcontext, buf, size);
	if (Nflag)
		return;
	if (write(ofile, buf, (int) size) == -1) {
		fprintf(stderr,
		    "write error extracting inode %llu, name %s\nwrite: %s\n",
			(unsigned long long)curfile.ino, curfile.name,
			strerror(errno));
		exit(1);
	}
}

/*
 * Skip over a hole in a file.
 */
/* ARGSUSED */
static void
xtrskip(char *buf, long size)
{

	if (Dflag)
		(*ddesc->dd_update)(&dcontext, buf, size);
	if (Nflag)
		return;
	if (lseek(ofile, size, SEEK_CUR) == -1) {
		fprintf(stderr,
		    "seek error extracting inode %llu, name %s\nlseek: %s\n",
			(unsigned long long)curfile.ino, curfile.name,
			strerror(errno));
		exit(1);
	}
}

/*
 * Collect the next block of a symbolic link.
 */
static void
xtrlnkfile(char *buf, long size)
{

	pathlen += size;
	if (pathlen > MAXPATHLEN) {
		fprintf(stderr, "symbolic link name: %s->%s%s; too long %d\n",
		    curfile.name, lnkbuf, buf, pathlen);
		exit(1);
	}
	(void) strcat(lnkbuf, buf);
}

/*
 * Skip over a hole in a symbolic link (should never happen).
 */
/* ARGSUSED */
static void
xtrlnkskip(char *buf __unused, long size __unused)
{

	fprintf(stderr, "unallocated block in symbolic link %s\n",
		curfile.name);
	exit(1);
}

/*
 * Noop, when an extraction function is not needed.
 */
/* ARGSUSED */
void
xtrnull(char *buf __unused, long size __unused)
{

	return;
}

/*
 * Read TP_BSIZE blocks from the input.
 * Handle read errors, and end of media.
 */
static void
readtape(char *buf)
{
	int rd, newvol, i;
	int cnt, seek_failed;

	if (blkcnt < numtrec) {
		memmove(buf, &tapebuf[(blkcnt++ * TP_BSIZE)], (long)TP_BSIZE);
		blksread++;
		tpblksread++;
		return;
	}
	for (i = 0; i < ntrec; i++)
		((struct s_spcl *)&tapebuf[i * TP_BSIZE])->c_magic = 0;
	if (numtrec == 0)
		numtrec = ntrec;
	cnt = ntrec * TP_BSIZE;
	rd = 0;
getmore:
#ifdef RRESTORE
	if (host)
		i = rmtread(&tapebuf[rd], cnt);
	else
#endif
		i = read(mt, &tapebuf[rd], cnt);
	/*
	 * Check for mid-tape short read error.
	 * If found, skip rest of buffer and start with the next.
	 */
	if (!pipein && numtrec < ntrec && i > 0) {
		dprintf(stdout, "mid-media short read error.\n");
		numtrec = ntrec;
	}
	/*
	 * Handle partial block read.
	 */
	if (pipein && i == 0 && rd > 0)
		i = rd;
	else if (i > 0 && i != ntrec * TP_BSIZE) {
		if (pipein) {
			rd += i;
			cnt -= i;
			if (cnt > 0)
				goto getmore;
			i = rd;
		} else {
			/*
			 * Short read. Process the blocks read.
			 */
			if (i % TP_BSIZE != 0)
				vprintf(stdout,
				    "partial block read: %d should be %d\n",
				    i, ntrec * TP_BSIZE);
			numtrec = i / TP_BSIZE;
		}
	}
	/*
	 * Handle read error.
	 */
	if (i < 0) {
		fprintf(stderr, "Tape read error while ");
		switch (curfile.action) {
		default:
			fprintf(stderr, "trying to set up tape\n");
			break;
		case UNKNOWN:
			fprintf(stderr, "trying to resynchronize\n");
			break;
		case USING:
			fprintf(stderr, "restoring %s\n", curfile.name);
			break;
		case SKIP:
			fprintf(stderr, "skipping over inode %llu\n",
			    (unsigned long long)curfile.ino);
			break;
		}
		if (!yflag && !reply("continue"))
			exit(1);
		i = ntrec * TP_BSIZE;
		memset(tapebuf, 0, i);
#ifdef RRESTORE
		if (host)
			seek_failed = (rmtseek(i, 1) < 0);
		else
#endif
			seek_failed = (lseek(mt, i, SEEK_CUR) == (off_t)-1);

		if (seek_failed) {
			fprintf(stderr,
			    "continuation failed: %s\n", strerror(errno));
			exit(1);
		}
	}
	/*
	 * Handle end of tape.
	 */
	if (i == 0) {
		vprintf(stdout, "End-of-tape encountered\n");
		if (!pipein) {
			newvol = volno + 1;
			volno = 0;
			numtrec = 0;
			getvol(newvol);
			readtape(buf);
			return;
		}
		if (rd % TP_BSIZE != 0)
			panic("partial block read: %d should be %d\n",
				rd, ntrec * TP_BSIZE);
		terminateinput();
		memmove(&tapebuf[rd], &endoftapemark, (long)TP_BSIZE);
	}
	blkcnt = 0;
	memmove(buf, &tapebuf[(blkcnt++ * TP_BSIZE)], (long)TP_BSIZE);
	blksread++;
	tpblksread++;
}

static void
findtapeblksize(void)
{
	long i;

	for (i = 0; i < ntrec; i++)
		((struct s_spcl *)&tapebuf[i * TP_BSIZE])->c_magic = 0;
	blkcnt = 0;
#ifdef RRESTORE
	if (host)
		i = rmtread(tapebuf, ntrec * TP_BSIZE);
	else
#endif
		i = read(mt, tapebuf, ntrec * TP_BSIZE);

	if (i <= 0) {
		fprintf(stderr, "tape read error: %s\n", strerror(errno));
		exit(1);
	}
	if (i % TP_BSIZE != 0) {
		fprintf(stderr, "Tape block size (%ld) %s (%ld)\n",
			(long)i, "is not a multiple of dump block size",
			(long)TP_BSIZE);
		exit(1);
	}
	ntrec = i / TP_BSIZE;
	numtrec = ntrec;
	vprintf(stdout, "Tape block size is %d\n", ntrec);
}

void
closemt(void)
{

	if (mt < 0)
		return;
#ifdef RRESTORE
	if (host)
		rmtclose();
	else
#endif
		(void) close(mt);
}

/*
 * Read the next block from the tape.
 * Check to see if it is one of several vintage headers.
 * If it is an old style header, convert it to a new style header.
 * If it is not any valid header, return an error.
 */
static int
gethead(struct s_spcl *buf)
{
	union u_ospcl u_ospcl;

	if (!cvtflag) {
		readtape((char *)buf);
		if (buf->c_magic != NFS_MAGIC &&
		    buf->c_magic != FS_UFS2_MAGIC) {
			if (bswap32(buf->c_magic) != NFS_MAGIC &&
			    bswap32(buf->c_magic) != FS_UFS2_MAGIC)
				return (FAIL);
			if (!Bcvt) {
				vprintf(stdout, "Note: Doing Byte swapping\n");
				Bcvt = 1;
			}
		}
		if (checksum((int *)buf) == FAIL)
			return (FAIL);
		if (Bcvt)
			swap_header(buf);
		goto good;
	}

	readtape((char *)(&u_ospcl.s_ospcl));
	if (checksum((int *)(&u_ospcl.s_ospcl)) == FAIL)
		return (FAIL);
	if (u_ospcl.s_ospcl.c_magic != OFS_MAGIC) {
		if (bswap32(u_ospcl.s_ospcl.c_magic) != OFS_MAGIC)
			return (FAIL);
		if (!Bcvt) {
			vprintf(stdout, "Note: Doing Byte swapping\n");
			Bcvt = 1;
		}
		swap_old_header(&u_ospcl.s_ospcl);
	}

	memset(buf, 0, TP_BSIZE);
	buf->c_type = u_ospcl.s_ospcl.c_type;
	buf->c_date = u_ospcl.s_ospcl.c_date;
	buf->c_ddate = u_ospcl.s_ospcl.c_ddate;
	buf->c_volume = u_ospcl.s_ospcl.c_volume;
	buf->c_tapea = u_ospcl.s_ospcl.c_tapea;
	buf->c_inumber = u_ospcl.s_ospcl.c_inumber;
	buf->c_checksum = u_ospcl.s_ospcl.c_checksum;
	buf->c_mode = u_ospcl.s_ospcl.c_odinode.odi_mode;
	buf->c_uid = u_ospcl.s_ospcl.c_odinode.odi_uid;
	buf->c_gid = u_ospcl.s_ospcl.c_odinode.odi_gid;
	buf->c_size = u_ospcl.s_ospcl.c_odinode.odi_size;
	buf->c_rdev = u_ospcl.s_ospcl.c_odinode.odi_rdev;
	buf->c_atime = u_ospcl.s_ospcl.c_odinode.odi_atime;
	buf->c_mtime = u_ospcl.s_ospcl.c_odinode.odi_mtime;
	buf->c_count = u_ospcl.s_ospcl.c_count;
	memmove(buf->c_addr, u_ospcl.s_ospcl.c_addr, (long)256);
	buf->c_magic = FS_UFS2_MAGIC;
good:
	switch (buf->c_type) {

	case TS_CLRI:
	case TS_BITS:
		/*
		 * Have to patch up missing information in bit map headers
		 */
		buf->c_inumber = 0;
		buf->c_size = buf->c_count * TP_BSIZE;
		break;

	case TS_TAPE:
		if ((buf->c_flags & DR_NEWINODEFMT) == 0)
			oldinofmt = 1;
		/* fall through */
	case TS_END:
		buf->c_inumber = 0;
		break;

	case TS_INODE:
		if (buf->c_magic == NFS_MAGIC) {
			buf->c_tapea = buf->c_old_tapea;
			buf->c_firstrec = buf->c_old_firstrec;
			buf->c_date = buf->c_old_date;
			buf->c_ddate = buf->c_old_ddate;
			buf->c_atime = buf->c_old_atime;
			buf->c_mtime = buf->c_old_mtime;
			buf->c_birthtime = 0;
			buf->c_birthtimensec = 0;
			buf->c_atimensec = buf->c_mtimensec = 0;
		}
			
	case TS_ADDR:
		break;

	default:
		panic("gethead: unknown inode type %d\n", buf->c_type);
		break;
	}

	buf->c_magic = FS_UFS2_MAGIC;

	/*
	 * If we are restoring a filesystem with old format inodes, 
	 * copy the uid/gid to the new location.
	 */
	if (oldinofmt) {
		buf->c_uid = buf->c_spare1[1];
		buf->c_gid = buf->c_spare1[2];
	}
	if (dflag)
		accthdr(buf);
	return(GOOD);
}

/*
 * Check that a header is where it belongs and predict the next header
 */
static void
accthdr(struct s_spcl *header)
{
	static ino_t previno = 0x7fffffff;
	static int prevtype;
	static long predict;
	long blks, i;

	if (header->c_type == TS_TAPE) {
		fprintf(stderr, "Volume header (%s inode format) ",
		    oldinofmt ? "old" : "new");
 		if (header->c_firstrec)
 			fprintf(stderr, "begins with record %lld",
 				(long long)header->c_firstrec);
 		fprintf(stderr, "\n");
		previno = 0x7fffffff;
		return;
	}
	if (previno == 0x7fffffff)
		goto newcalc;
	switch (prevtype) {
	case TS_BITS:
		fprintf(stderr, "Dumped inodes map header");
		break;
	case TS_CLRI:
		fprintf(stderr, "Used inodes map header");
		break;
	case TS_INODE:
		fprintf(stderr, "File header, ino %llu",
		    (unsigned long long)previno);
		break;
	case TS_ADDR:
		fprintf(stderr, "File continuation header, ino %llu",
		    (unsigned long long)previno);
		break;
	case TS_END:
		fprintf(stderr, "End of tape header");
		break;
	}
	if (predict != blksread - 1)
		fprintf(stderr, "; predicted %ld blocks, got %ld blocks",
			(long)predict, (long)(blksread - 1));
	fprintf(stderr, "\n");
newcalc:
	blks = 0;
	switch (header->c_type) {
	case TS_END:
		break;
	case TS_CLRI:
	case TS_BITS:
		blks = header->c_count;
		break;
	default:
		for (i = 0; i < header->c_count; i++)
			if (header->c_addr[i] != 0)
				blks++;
		break;
	}
	predict = blks;
	blksread = 0;
	prevtype = header->c_type;
	previno = header->c_inumber;
}

/*
 * Find an inode header.
 * Complain if had to skip, and complain is set.
 */
static void
findinode(struct s_spcl *header)
{
	static long skipcnt = 0;
	long i;
	char buf[TP_BSIZE];

	curfile.name = "<name unknown>";
	curfile.action = UNKNOWN;
	curfile.mode = 0;
	curfile.ino = 0;
    top:
	do {
		if (header->c_magic != FS_UFS2_MAGIC) {
			skipcnt++;
			while (gethead(header) == FAIL ||
			    header->c_date != dumpdate)
				skipcnt++;
		}
		switch (header->c_type) {

		case TS_ADDR:
			/*
			 * Skip up to the beginning of the next record
			 */
			for (i = 0; i < header->c_count; i++)
				if (header->c_addr[i])
					readtape(buf);
			while (gethead(header) == FAIL ||
			    header->c_date != dumpdate)
				skipcnt++;
			/* We've read a header; don't drop it. */
			goto top;

		case TS_INODE:
			curfile.mode = header->c_mode;
			curfile.uid = header->c_uid;
			curfile.gid = header->c_gid;
			curfile.file_flags = header->c_file_flags;
			curfile.rdev = header->c_rdev;
			curfile.atime_sec = header->c_atime;
			curfile.atime_nsec = header->c_atimensec;
			curfile.mtime_sec = header->c_mtime;
			curfile.mtime_nsec = header->c_mtimensec;
			curfile.birthtime_sec = header->c_birthtime;
			curfile.birthtime_nsec = header->c_birthtimensec;
			curfile.size = header->c_size;
			curfile.ino = header->c_inumber;
			break;

		case TS_END:
			curfile.ino = maxino;
			break;

		case TS_CLRI:
			curfile.name = "<file removal list>";
			break;

		case TS_BITS:
			curfile.name = "<file dump list>";
			break;

		case TS_TAPE:
			panic("unexpected tape header\n");
			break;

		default:
			panic("unknown tape header type %d\n", spcl.c_type);
			break;

		}
	} while (header->c_type == TS_ADDR);
	if (skipcnt > 0)
		fprintf(stderr, "resync restore, skipped %ld blocks\n",
		    (long)skipcnt);
	skipcnt = 0;
}

static int
checksum(int *buf)
{
	int i, j;

	j = sizeof(union u_spcl) / sizeof(int);
	i = 0;
	if(!Bcvt) {
		do
			i += *buf++;
		while (--j);
	} else {
		do 
			i += bswap32(*buf++);
		while (--j);
	}
			
	if (i != CHECKSUM) {
		fprintf(stderr, "Checksum error %o, inode %llu file %s\n", i,
		    (unsigned long long)curfile.ino, curfile.name);
		return(FAIL);
	}
	return(GOOD);
}

#ifdef RRESTORE
#include <stdarg.h>

void
msg(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	(void)vfprintf(stderr, fmt, ap);
	va_end(ap);
}
#endif /* RRESTORE */

static void
swap_header(struct s_spcl *s)
{
	s->c_type = bswap32(s->c_type);
	s->c_old_date = bswap32(s->c_old_date);
	s->c_old_ddate = bswap32(s->c_old_ddate);
	s->c_volume = bswap32(s->c_volume);
	s->c_old_tapea = bswap32(s->c_old_tapea);
	s->c_inumber = bswap32(s->c_inumber);
	s->c_magic = bswap32(s->c_magic);
	s->c_checksum = bswap32(s->c_checksum);

	s->c_mode = bswap16(s->c_mode);
	s->c_size = bswap64(s->c_size);
	s->c_old_atime = bswap32(s->c_old_atime);
	s->c_atimensec = bswap32(s->c_atimensec);
	s->c_old_mtime = bswap32(s->c_old_mtime);
	s->c_mtimensec = bswap32(s->c_mtimensec);
	s->c_rdev = bswap32(s->c_rdev);
	s->c_birthtimensec = bswap32(s->c_birthtimensec);
	s->c_birthtime = bswap64(s->c_birthtime);
	s->c_atime = bswap64(s->c_atime);
	s->c_mtime = bswap64(s->c_mtime);
	s->c_file_flags = bswap32(s->c_file_flags);
	s->c_uid = bswap32(s->c_uid);
	s->c_gid = bswap32(s->c_gid);

	s->c_count = bswap32(s->c_count);
	s->c_level = bswap32(s->c_level);
	s->c_flags = bswap32(s->c_flags);
	s->c_old_firstrec = bswap32(s->c_old_firstrec);

	s->c_date = bswap64(s->c_date);
	s->c_ddate = bswap64(s->c_ddate);
	s->c_tapea = bswap64(s->c_tapea);
	s->c_firstrec = bswap64(s->c_firstrec);

	/*
	 * These are ouid and ogid.
	 */
	s->c_spare1[1] = bswap16(s->c_spare1[1]);
	s->c_spare1[2] = bswap16(s->c_spare1[2]);
}

static void
swap_old_header(struct s_ospcl *os)
{
	os->c_type = bswap32(os->c_type);
	os->c_date = bswap32(os->c_date);
	os->c_ddate = bswap32(os->c_ddate);
	os->c_volume = bswap32(os->c_volume);
	os->c_tapea = bswap32(os->c_tapea);
	os->c_inumber = bswap16(os->c_inumber);
	os->c_magic = bswap32(os->c_magic);
	os->c_checksum = bswap32(os->c_checksum);

	os->c_odinode.odi_mode = bswap16(os->c_odinode.odi_mode);
	os->c_odinode.odi_nlink = bswap16(os->c_odinode.odi_nlink);
	os->c_odinode.odi_uid = bswap16(os->c_odinode.odi_uid);
	os->c_odinode.odi_gid = bswap16(os->c_odinode.odi_gid);

	os->c_odinode.odi_size = bswap32(os->c_odinode.odi_size);
	os->c_odinode.odi_rdev = bswap32(os->c_odinode.odi_rdev);
	os->c_odinode.odi_atime = bswap32(os->c_odinode.odi_atime);
	os->c_odinode.odi_mtime = bswap32(os->c_odinode.odi_mtime);
	os->c_odinode.odi_ctime = bswap32(os->c_odinode.odi_ctime);

	os->c_count = bswap32(os->c_count);
}
