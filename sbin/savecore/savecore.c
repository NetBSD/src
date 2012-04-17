/*	$NetBSD: savecore.c,v 1.84.2.1 2012/04/17 00:05:43 yamt Exp $	*/

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
__COPYRIGHT("@(#) Copyright (c) 1986, 1992, 1993\
 The Regents of the University of California.  All rights reserved.");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)savecore.c	8.5 (Berkeley) 4/28/95";
#else
__RCSID("$NetBSD: savecore.c,v 1.84.2.1 2012/04/17 00:05:43 yamt Exp $");
#endif
#endif /* not lint */

#define _KSYMS_PRIVATE

#include <stdbool.h>

#include <sys/param.h>
#include <sys/mount.h>
#include <sys/msgbuf.h>
#include <sys/syslog.h>
#include <sys/time.h>
#include <sys/ksyms.h>

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
#include <stdarg.h>
#include <kvm.h>

extern FILE *zopen(const char *fname, const char *mode);

#define	KREAD(kd, addr, p)\
	(kvm_read(kd, addr, (char *)(p), sizeof(*(p))) != sizeof(*(p)))

static struct nlist current_nl[] = {	/* Namelist for currently running system. */
#define	X_DUMPDEV	0
	{ .n_name = "_dumpdev" },
#define	X_DUMPLO	1
	{ .n_name = "_dumplo" },
#define	X_TIME_SECOND	2
	{ .n_name = "_time_second" },
#define X_TIME		3
	{ .n_name = "_time" },
#define	X_DUMPSIZE	4
	{ .n_name = "_dumpsize" },
#define	X_VERSION	5
	{ .n_name = "_version" },
#define	X_DUMPMAG	6
	{ .n_name = "_dumpmag" },
#define	X_PANICSTR	7
	{ .n_name = "_panicstr" },
#define	X_PANICSTART	8
	{ .n_name = "_panicstart" },
#define	X_PANICEND	9
	{ .n_name = "_panicend" },
#define	X_MSGBUF	10
	{ .n_name = "_msgbufp" },
#define	X_DUMPCDEV	11
	{ .n_name = "_dumpcdev" },
#define X_SYMSZ		12
	{ .n_name = "_ksyms_symsz" },
#define X_STRSZ		13
	{ .n_name = "_ksyms_strsz" },
#define X_KHDR		14
	{ .n_name = "_ksyms_hdr" },
#define X_SYMTABS	15
	{ .n_name = "_ksyms_symtabs" },
	{ .n_name = NULL },
};
static int cursyms[] = { X_DUMPDEV, X_DUMPLO, X_VERSION, X_DUMPMAG, X_DUMPCDEV, -1 };
static int dumpsyms[] = { X_TIME_SECOND, X_TIME, X_DUMPSIZE, X_VERSION, X_PANICSTR,
    X_DUMPMAG, X_SYMSZ, X_STRSZ, X_KHDR, X_SYMTABS, -1 };

static struct nlist dump_nl[] = {	/* Name list for dumped system. */
	{ .n_name = "_dumpdev" },	/* Entries MUST be the same as */
	{ .n_name = "_dumplo" },	/*	those in current_nl[].  */
	{ .n_name = "_time_second" },
	{ .n_name = "_time" },
	{ .n_name = "_dumpsize" },
	{ .n_name = "_version" },
	{ .n_name = "_dumpmag" },
	{ .n_name = "_panicstr" },
	{ .n_name = "_panicstart" },
	{ .n_name = "_panicend" },
	{ .n_name = "_msgbufp" },
	{ .n_name = "_dumpcdev" },
	{ .n_name = "_ksyms_symsz" },
	{ .n_name = "_ksyms_strsz" },
	{ .n_name = "_ksyms_hdr" },
	{ .n_name = "_ksyms_symtabs" },
	{ .n_name = NULL },
};

/* Types match kernel declarations. */
static off_t	dumplo;				/* where dump starts on dumpdev */
static u_int32_t dumpmag;			/* magic number in dump */
static int	dumpsize;			/* amount of memory dumped */
static off_t dumpbytes;			/* in bytes */

static const char	*kernel;		/* name of used kernel */
static const char	*dirname;		/* directory to save dumps in */
static char	*ddname;			/* name of dump device */
static dev_t	dumpdev;			/* dump device */
static dev_t	dumpcdev = NODEV;		/* dump device (char equivalent) */
static int	dumpfd;				/* read/write descriptor on dev */
static kvm_t	*kd_dump;			/* kvm descriptor on dev	*/
static time_t	now;				/* current date */
static char	panic_mesg[1024];
static long	panicstr;
static char	vers[1024];
static char	gzmode[3];

static int	clear, compress, force, verbose;	/* flags */

static void	check_kmem(void);
static int	check_space(void);
static void	clear_dump(void);
static int	Create(char *, int);
static int	dump_exists(void);
static char	*find_dev(dev_t, mode_t);
static int	get_crashtime(void);
static void	kmem_setup(void);
static void	Lseek(int, off_t, int);
static int	Open(const char *, int rw);
static void	save_core(void);
__dead static void	usage(const char *fmt, ...) __printflike(1, 2);

int
main(int argc, char *argv[])
{
	int ch, level, testonly;
	char *ep;

	kernel = NULL;
	level = 1;		/* default to fastest gzip compression */
	testonly = 0;
	gzmode[0] = 'w';

	openlog("savecore", LOG_PERROR, LOG_DAEMON);

	while ((ch = getopt(argc, argv, "cdfnN:vzZ:")) != -1)
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
		case 'n':
			testonly = 1;
			break;
		case 'N':
			kernel = optarg;
			break;
		case 'z':
			compress = 1;
			break;
		case 'Z':
			level = (int)strtol(optarg, &ep, 10);
			if (level < 0 || level > 9)
				usage("Invalid compression `%s'", optarg);
			break;
		case '?':
			usage("Missing argument for flag `%c'", optopt);
		default:
			usage("Unknown flag `%c'", ch);
		}
	argc -= optind;
	argv += optind;

	if (argc != 0)
		dirname = argv[0];
	else
		dirname = "/var/crash";

	gzmode[1] = level + '0';

	(void)time(&now);
	kmem_setup();

	if (clear && !testonly) {
		clear_dump();
		exit(0);
	}

	if (!dump_exists() && !force)
		exit(1);

	if (testonly)
		/* If -n was passed and there was a dump, exit at level 0 */
		exit(0);

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

static void
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
	
	for (i = 0; cursyms[i] != -1; i++) {
		if (current_nl[cursyms[i]].n_value != 0)
			continue;
		switch (cursyms[i]) {
		case X_TIME_SECOND:
		case X_TIME:
		case X_DUMPCDEV:
			break;
		default:
			syslog(LOG_ERR, "%s: %s not in namelist",
			    kernel, current_nl[cursyms[i]].n_name);
			exit(1);
		}
	}

	if (KREAD(kd_kern, current_nl[X_DUMPDEV].n_value, &dumpdev) != 0) {
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
		syslog(LOG_WARNING, "kvm_read: %s", kvm_geterr(kd_kern));
		exit(1);
	}

	(void)kvm_read(kd_kern, current_nl[X_VERSION].n_value, vers,
	    sizeof(vers));
	vers[sizeof(vers) - 1] = '\0';

	if (current_nl[X_DUMPCDEV].n_value != 0) {
		if (KREAD(kd_kern, current_nl[X_DUMPCDEV].n_value,
		    &dumpcdev) != 0) {
			syslog(LOG_WARNING, "kvm_read: %s",
			    kvm_geterr(kd_kern));
			exit(1);
		}
		ddname = find_dev(dumpcdev, S_IFCHR);
	} else
		ddname = find_dev(dumpdev, S_IFBLK);
	if (strncmp(ddname, "/dev/cons", 8) == 0 ||
	    strncmp(ddname, "/dev/tty", 7) == 0 ||
	    strncmp(ddname, "/dev/pty", 7) == 0 ||
	    strncmp(ddname, "/dev/pts", 7) == 0) {
		syslog(LOG_ERR, "dumpdev %s is tty; override kernel", ddname);
		exit(1);
	}
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
		if (dump_nl[dumpsyms[i]].n_value == 0 &&
			dumpsyms[i] != X_TIME_SECOND &&
			dumpsyms[i] != X_TIME) {
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

static void
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
		    kvm_getkernelname(kd_dump), vers, core_vers);

	panicstart = panicend = 0;
	if (KREAD(kd_dump, dump_nl[X_PANICSTART].n_value, &panicstart) != 0) {
		syslog(LOG_WARNING, "kvm_read: %s", kvm_geterr(kd_dump));
		goto nomsguf;
	}
	if (KREAD(kd_dump, dump_nl[X_PANICEND].n_value, &panicend) != 0) {
		syslog(LOG_WARNING, "kvm_read: %s", kvm_geterr(kd_dump));
		goto nomsguf;
	}
	if (panicstart != 0 && panicend != 0) {
		if (KREAD(kd_dump, dump_nl[X_MSGBUF].n_value, &bufp)) {
			syslog(LOG_WARNING, "kvm_read: %s", kvm_geterr(kd_dump));
			goto nomsguf;
		}
		if (kvm_read(kd_dump, (long)bufp, &msgbuf,
		    offsetof(struct kern_msgbuf, msg_bufc)) !=
		    offsetof(struct kern_msgbuf, msg_bufc)) {
			syslog(LOG_WARNING, "kvm_read: %s", kvm_geterr(kd_dump));
			goto nomsguf;
		}
		if (msgbuf.msg_magic != MSG_MAGIC) {
			syslog(LOG_WARNING, "msgbuf magic incorrect");
			goto nomsguf;
		}
		bufdata = malloc(msgbuf.msg_bufs);
		if (bufdata == NULL) {
			syslog(LOG_WARNING, "couldn't allocate space for msgbuf data");
			goto nomsguf;
		}
		if (kvm_read(kd_dump, (long)&bufp->msg_bufc, bufdata,
		    msgbuf.msg_bufs) != msgbuf.msg_bufs) {
			syslog(LOG_WARNING, "kvm_read: %s", kvm_geterr(kd_dump));
			free(bufdata);
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
		free(bufdata);

		panicstr = 1;	/* anything not zero */
		return;
	}
nomsguf:
	if (KREAD(kd_dump, dump_nl[X_PANICSTR].n_value, &panicstr) != 0) {
		syslog(LOG_WARNING, "kvm_read: %s", kvm_geterr(kd_dump));
		return;
	}
	if (panicstr) {
		cp = panic_mesg;
		panicloc = panicstr;
		do {
			if (KREAD(kd_dump, panicloc, cp) != 0) {
				syslog(LOG_WARNING, "kvm_read: %s",
				    kvm_geterr(kd_dump));
				break;
			}
			panicloc++;
		} while (*cp++ && cp < &panic_mesg[sizeof(panic_mesg)-1]);
		panic_mesg[sizeof(panic_mesg) - 1] = '\0';
	}
}

static int
dump_exists(void)
{
	u_int32_t newdumpmag;

	if (KREAD(kd_dump, dump_nl[X_DUMPMAG].n_value, &newdumpmag) != 0) {
		syslog(LOG_WARNING, "kvm_read: %s", kvm_geterr(kd_dump));
		return (0);
	}

	/* Read the dump size. */
	if (KREAD(kd_dump, dump_nl[X_DUMPSIZE].n_value, &dumpsize) != 0) {
		syslog(LOG_WARNING, "kvm_read: %s", kvm_geterr(kd_dump));
		return (0);
	}
	dumpbytes = (off_t)dumpsize * getpagesize();

	/*
	 * Return zero if core dump doesn't seem to be there, and note
	 * it for syslog.  This check and return happens after the dump size
	 * is read, so dumpsize is whether or not the core is valid (for -f).
	 */
	if (newdumpmag != dumpmag) {
		if (verbose)
			syslog(LOG_WARNING, "magic number mismatch "
			    "(0x%x != 0x%x)", newdumpmag, dumpmag);
		syslog(LOG_WARNING, "no core dump");
		return (0);
	}
	return (1);
}

static void
clear_dump(void)
{
	if (kvm_dump_inval(kd_dump) == -1)
		syslog(LOG_ERR, "%s: kvm_dump_inval: %s", ddname,
		    kvm_geterr(kd_dump));

}

static char buf[1024 * 1024];

static void
save_kernel(int ofd, FILE *fp, char *path)
{
	int nw, nr, ifd;

	ifd = Open(kernel, O_RDONLY);
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
}

static int
ksymsget(u_long addr, void *ptr, size_t size)
{

	if ((size_t)kvm_read(kd_dump, addr, ptr, size) != size) {
		syslog(LOG_WARNING, "kvm_read: %s", kvm_geterr(kd_dump));
		return 1;
	}
	return 0;
}

static int
save_ksyms(int ofd, FILE *fp, char *path)
{
	struct ksyms_hdr khdr;
	int nw, symsz, strsz;
	TAILQ_HEAD(, ksyms_symtab) symtabs;
	struct ksyms_symtab st, *stptr;
	void *p;

	/* Get basic info and ELF headers, check if ksyms was on. */
	if (ksymsget(dump_nl[X_KHDR].n_value, &khdr, sizeof(khdr)))
		return 1;
	if (ksymsget(dump_nl[X_SYMSZ].n_value, &symsz, sizeof(symsz)))
		return 1;
	if (ksymsget(dump_nl[X_STRSZ].n_value, &strsz, sizeof(strsz)))
		return 1;
	if (symsz == 0 || strsz == 0)
		return 1;

	/* Update the ELF section headers for symbols/strings. */
	khdr.kh_shdr[SYMTAB].sh_size = symsz;
	khdr.kh_shdr[SYMTAB].sh_info = symsz / sizeof(Elf_Sym);
	khdr.kh_shdr[STRTAB].sh_offset = symsz +
	    khdr.kh_shdr[SYMTAB].sh_offset;
	khdr.kh_shdr[STRTAB].sh_size = strsz;

	/* Write out the ELF headers. */
	if (compress)
		nw = fwrite(&khdr, 1, sizeof(khdr), fp);
	else
		nw = write(ofd, &khdr, sizeof(khdr));
	if (nw != sizeof(khdr)) {
		syslog(LOG_ERR, "%s: %s",
		    path, strerror(nw == 0 ? EIO : errno));
		syslog(LOG_WARNING,
		    "WARNING: kernel may be incomplete");
		exit(1);
        }

        /* Dump symbol table. */
	if (ksymsget(dump_nl[X_SYMTABS].n_value, &symtabs, sizeof(symtabs)))
		return 1;
	stptr = TAILQ_FIRST(&symtabs);
	while (stptr != NULL) {
		if (ksymsget((u_long)stptr, &st, sizeof(st)))
			return 1;
		stptr = TAILQ_NEXT(&st, sd_queue);
		if ((p = malloc(st.sd_symsize)) == NULL)
			return 1;
		if (ksymsget((u_long)st.sd_symstart, p, st.sd_symsize)) {
			free(p);
			return 1;
		}
		if (compress)
			nw = fwrite(p, 1, st.sd_symsize, fp);
		else
			nw = write(ofd, p, st.sd_symsize);
		free(p);
		if (nw != st.sd_symsize) {
			syslog(LOG_ERR, "%s: %s",
			    path, strerror(nw == 0 ? EIO : errno));
			syslog(LOG_WARNING,
			    "WARNING: kernel may be incomplete");
			exit(1);
		}
	}

	/* Dump string table. */
	if (ksymsget(dump_nl[X_SYMTABS].n_value, &symtabs, sizeof(symtabs)))
		return 1;
	stptr = TAILQ_FIRST(&symtabs);
	while (stptr != NULL) {
		if (ksymsget((u_long)stptr, &st, sizeof(st)))
			return 1;
		stptr = TAILQ_NEXT(&st, sd_queue);
		if ((p = malloc(st.sd_symsize)) == NULL)
			return 1;
		if (ksymsget((u_long)st.sd_strstart, p, st.sd_strsize)) {
			free(p);
			return 1;
		}
		if (compress)
			nw = fwrite(p, 1, st.sd_strsize, fp);
		else
			nw = write(ofd, p, st.sd_strsize);
		free(p);
		if (nw != st.sd_strsize) {
			syslog(LOG_ERR, "%s: %s",
			    path, strerror(nw == 0 ? EIO : errno));
			syslog(LOG_WARNING,
			    "WARNING: kernel may be incomplete");
			exit(1);
		}
	}

	return 0;
}

static void
save_core(void)
{
	FILE *fp;
	int bounds, ifd, nr, nw, ofd, tryksyms;
	char path[MAXPATHLEN], rbuf[MAXPATHLEN];
	const char *rawp;

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

	if (dumpcdev == NODEV) {
		/* Open the raw device. */
		rawp = getdiskrawname(rbuf, sizeof(rbuf), ddname);
		if (rawp == NULL) {
			syslog(LOG_WARNING, "%s: %m; can't convert to raw",
			    ddname);
			rawp = ddname;
		}
		if ((ifd = open(rawp, O_RDONLY)) == -1) {
			syslog(LOG_WARNING, "%s: %m; using block device",
			    rawp);
			ifd = dumpfd;
		}
	} else {
		rawp = ddname;
		ifd = dumpfd;
	}

	/* Seek to the start of the core. */
	Lseek(ifd, dumplo, SEEK_SET);

	if (kvm_dump_wrtheader(kd_dump, fp, (int32_t)dumpbytes) == -1) {
		syslog(LOG_ERR, "kvm_dump_wrtheader: %s : %s", path,
		    kvm_geterr(kd_dump));
		exit(1);
	}

	/* Copy the core file. */
	syslog(LOG_NOTICE, "writing %score to %s",
	    compress ? "compressed " : "", path);
	for (; dumpbytes > (off_t)0; dumpbytes -= (off_t)nr) {
		char nbuf[7];
		humanize_number(nbuf, 7, dumpbytes, "", HN_AUTOSCALE, 0);
		(void)printf("%7s\r", nbuf);
		(void)fflush(stdout);
		nr = read(ifd, buf, MIN(dumpbytes, (off_t)sizeof(buf)));
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
	if (dumpcdev == NODEV)
		(void)close(ifd);
	(void)fclose(fp);

	/* Create a kernel. */
	(void)snprintf(path, sizeof(path), "%s/netbsd.%d%s",
	    dirname, bounds, compress ? ".gz" : "");
	syslog(LOG_NOTICE, "writing %skernel to %s",
	    compress ? "compressed " : "", path);
	for (tryksyms = 1;; tryksyms = 0) {
		if (compress) {
			if ((fp = zopen(path, gzmode)) == NULL) {
				syslog(LOG_ERR, "%s: %m", path);
				exit(1);
			}
		} else
			ofd = Create(path, S_IRUSR | S_IWUSR);
		if (tryksyms) {
			if (!save_ksyms(ofd, fp, path))
				break;
			if (compress)
				(void)fclose(fp);
			else
				(void)close(ofd);
			unlink(path);
		} else {
			save_kernel(ofd, fp, path);
			break;
		}
	}
	if (compress)
		(void)fclose(fp);
	else
		(void)close(ofd);

	/*
	 * For development systems where the crash occurs during boot
	 * to multiuser.
	 */
	sync();
	sleep(1);
	sync();
	sleep(1);
}

static char *
find_dev(dev_t dev, mode_t type)
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
	syslog(LOG_ERR, "can't find device %lld/%lld",
	    (long long)major(dev), (long long)minor(dev));
	exit(1);
}

static int
get_crashtime(void)
{
	time_t dumptime;			/* Time the dump was taken. */
	struct timeval dtime;

	if (KREAD(kd_dump, dump_nl[X_TIME_SECOND].n_value, &dumptime) != 0) {
		if (KREAD(kd_dump, dump_nl[X_TIME].n_value, &dtime) != 0) {
			syslog(LOG_WARNING, "kvm_read: %s (and _time_second "
			    "is not defined also)", kvm_geterr(kd_dump));
			return (0);
		}
		dumptime = dtime.tv_sec;
	}
	if (dumptime == 0) {
		syslog(LOG_WARNING, "dump time is zero");
		return (0);
	}
	syslog(LOG_INFO, "system went down at %s", ctime(&dumptime));
#define	LEEWAY	(60 * SECSPERDAY)
	if (dumptime < now - LEEWAY || dumptime > now + LEEWAY) {
		syslog(LOG_WARNING, "dump time is unreasonable");
		return (0);
	}
	return (1);
}

static int
check_space(void)
{
	FILE *fp;
	off_t minfree, spacefree, kernelsize, needed;
	struct stat st;
	struct statvfs fsbuf;
	char mbuf[100], path[MAXPATHLEN];

	/* XXX assume a reasonable default, unless we find a kernel. */
	kernelsize = 20 * 1024 * 1024;
	if (!stat(kernel, &st)) kernelsize = st.st_blocks * S_BLKSIZE;
	if (statvfs(dirname, &fsbuf) < 0) {
		syslog(LOG_ERR, "%s: %m", dirname);
		exit(1);
	}
	spacefree = fsbuf.f_bavail;
	spacefree *= fsbuf.f_frsize;
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

	needed = (dumpbytes + kernelsize) / 1024;
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

static int
Open(const char *name, int rw)
{
	int fd;

	if ((fd = open(name, rw, 0)) < 0) {
		syslog(LOG_ERR, "%s: %m", name);
		exit(1);
	}
	return (fd);
}

static void
Lseek(int fd, off_t off, int flag)
{
	off_t ret;

	ret = lseek(fd, off, flag);
	if (ret == -1) {
		syslog(LOG_ERR, "lseek: %m");
		exit(1);
	}
}

static int
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

static void
usage(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	(void)vsyslog(LOG_ERR, fmt, ap);
	va_end(ap);
	(void)syslog(LOG_ERR,
	    "Usage: %s [-cfnvz] [-N system] [-Z level] [directory]",
	    getprogname());
	exit(1);
}
