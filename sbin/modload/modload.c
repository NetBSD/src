/*	$NetBSD: modload.c,v 1.40 2003/07/13 07:45:27 itojun Exp $	*/

/*
 * Copyright (c) 1993 Terrence R. Lambert.
 * All rights reserved.
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
 *      This product includes software developed by Terrence R. Lambert.
 * 4. The name Terrence R. Lambert may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TERRENCE R. LAMBERT ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE TERRENCE R. LAMBERT BE LIABLE
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
__RCSID("$NetBSD: modload.c,v 1.40 2003/07/13 07:45:27 itojun Exp $");
#endif /* not lint */

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/conf.h>
#include <sys/mount.h>
#include <sys/lkm.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/sysctl.h>
#include <machine/cpu.h>
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <nlist.h>
#include "pathnames.h"

#define	TRUE 1
#define	FALSE 0

#include "modload.h"

#ifndef DFLT_ENTRY
#define	DFLT_ENTRY	"xxxinit"
#endif	/* !DFLT_ENTRY */
#ifndef DFLT_ENTRYEXT
#define	DFLT_ENTRYEXT	"_lkmentry"
#endif	/* !DFLT_ENTRYEXT */

int debug = 0;
int verbose = 0;
char *out = NULL;
int symtab = 0;
int Sflag;

static	void	cleanup(void);

/* prelink the module */
static int
prelink(const char *kernel,
	const char *entry,
	const char *outfile,
	const void *address,
	const char *object,
	const char *ldscript)
{
	char cmdbuf[1024];
	int error = 0;

	linkcmd(cmdbuf, sizeof(cmdbuf), kernel, entry, outfile, address,
	    object, ldscript);

	if (debug)
		fprintf(stderr, "%s\n", cmdbuf);

	switch (system(cmdbuf)) {
	case 0:				/* SUCCESS! */
		break;
	case 1:				/* uninformitive error */
		/*
		 * Someone needs to fix the return values from the NetBSD
		 * ld program -- it's totally uninformative.
		 *
		 * No such file		(4 on SunOS)
		 * Can't write output	(2 on SunOS)
		 * Undefined symbol	(1 on SunOS)
		 * etc.
		 */
	case 127:			/* can't load shell */
	case 32512:
	default:
		error = 1;
		break;
	}

	return error;
}

static void
usage(void)
{

	fprintf(stderr, "usage:\n");
	fprintf(stderr, "modload [-d] [-v] [-n] [-s] [-S] "
	    "[-A <kernel>] [-e <entry>]\n");
	fprintf(stderr,
	    "        [-p <postinstall>] [-o <output file>] <input file>\n");
	exit(1);
}

int fileopen = 0;
#define	DEV_OPEN	0x01
#define	MOD_OPEN	0x02
#define	PART_RESRV	0x04
#define	OUTFILE_CREAT	0x08

int devfd, modfd;
struct lmc_resrv resrv;

static void
cleanup(void)
{

	if (fileopen & PART_RESRV) {
		/*
		 * Free up kernel memory
		 */
		if (ioctl(devfd, LMUNRESRV, 0) == -1)
			warn("can't release slot 0x%08x memory", resrv.slot);
	}

	if (fileopen & DEV_OPEN)
		close(devfd);

	if (fileopen & MOD_OPEN)
		close(modfd);

	if (fileopen & OUTFILE_CREAT)
		unlink(out);
}

static int
verify_entry(const char *entry, char *filename)
{
	struct	nlist	names[2];
	int n;
	char *s;

	memset(names, 0, sizeof(names));
	asprintf(&s, "_%s", entry);
	if (!s)
		err(1, "malloc");
#ifdef	_AOUT_INCLUDE_
	names[0].n_un.n_name = s;
#else
	names[0].n_name = s;
#endif

	n = nlist(filename, names);
	if (n == -1)
		err(1, "nlist %s", filename);
	return n;
}

/*
 * Transfer data to kernel memory in chunks
 * of MODIOBUF size at a time.
 */
void
loadbuf(void *buf, size_t len)
{
	struct lmc_loadbuf ldbuf;
	size_t n;
	char *p = buf;

	while(len) {
		n = MIN(len, MODIOBUF);
		ldbuf.cnt = n;
		ldbuf.data = p;
		if (ioctl(devfd, LMLOADBUF, &ldbuf) == -1)
			err(11, "error loading buffer");
		len -= n;
		p += n;
	}
}

/* Transfer some empty space. */
void
loadspace(size_t len)
{
	char buf[MODIOBUF];
	size_t n;
	memset(buf, 0, sizeof(buf));
	while(len) {
		n = MIN(len, sizeof(buf));
		loadbuf(buf, n);
		len -= n;
	}
}

/*
 * Transfer symbol table to kernel memory in chunks
 * of MODIOBUF size at a time.
 */
void
loadsym(void *buf, size_t len)
{
	struct lmc_loadbuf ldbuf;
	size_t n;
	char *p = buf;

	while(len) {
		n = MIN(len, MODIOBUF);
		ldbuf.cnt = n;
		ldbuf.data = p;
		if(ioctl(devfd, LMLOADSYMS, &ldbuf) == -1)
			err(11, "error loading buffer");
		len -= n;
		p += n;
	}
}

/* Transfer some empty space. */
int
main(int argc, char **argv)
{
	int c;
	char *kname = NULL;
	char *entry = DFLT_ENTRY;
	char *post = NULL;
	char *ldscript = NULL;
	char *modobj;
	char modout[80], *p;
	struct stat stb;
	int strtablen;
	size_t modsize;	/* XXX */
	void *modentry;	/* XXX */
	int noready = 0;

	while ((c = getopt(argc, argv, "dnvse:p:o:A:ST:")) != -1) {
		switch (c) {
		case 'd':
			debug = 1;
			break;	/* debug */
		case 'v':
			verbose = 1;
			break;	/* verbose */
		case 'A':
			kname = optarg;
			break;	/* kernel */
		case 'e':
			entry = optarg;
			break;	/* entry point */
		case 'p':
			post = optarg;
			break;	/* postinstall */
		case 'o':
			out = optarg;
			break;	/* output file */
		case 'T':
			ldscript = optarg;
			break;	/* linker script */
		case 'n':
			noready = 1;
			break;
		case 's':
			symtab = 1;
			break;
		case 'S':
			Sflag = 1;
			break;
		case '?':
			usage();
			/* NOTREACHED */
		default:
			printf("default!\n");
			break;
		}
	}
	argc -= optind;
	argv += optind;

	if (argc != 1)
		usage();

	modobj = argv[0];

	atexit(cleanup);

	if (ldscript == NULL && access(_PATH_LDSCRIPT, R_OK) == 0)
		ldscript = _PATH_LDSCRIPT;

	/*
	 * Open the virtual device device driver for exclusive use (needed
	 * to write the new module to it as our means of getting it in the
	 * kernel).
	 */
	if ((devfd = open(_PATH_LKM, O_RDWR, 0)) == -1)
		err(3, _PATH_LKM);
	fileopen |= DEV_OPEN;

	strncpy(modout, modobj, sizeof(modout) - 1);
	modout[sizeof(modout) - 1] = '\0';

	p = strrchr(modout, '.');
	if (!p || strcmp(p, ".o"))
		errx(2, "module object must end in .o");
	*p = '\0';
	if (out == NULL)
		out = modout;

	/*
	 * Verify that the entry point for the module exists.
	 */
	if (verify_entry(entry, modobj)) {
		/*
		 * Try <modobj>_init if entry is DFLT_ENTRY.
		 */
		if (strcmp(entry, DFLT_ENTRY) == 0) {
			if ((p = strrchr(modout, '/')))
				p++;
			else
				p = modout;
			asprintf(&entry, "%s%s", p, DFLT_ENTRYEXT);
			if (!entry)
				err(1, "malloc");
			if (verify_entry(entry, modobj))
				errx(1, "entry point _%s not found in %s",
				    entry, modobj);
		} else
			errx(1, "entry point _%s not found in %s", entry,
			    modobj);
	}

	/*
	 * Check if /dev/ksyms can be used.
	 */
	if (kname == NULL) {
		int fd = open(_PATH_KSYMS, O_RDONLY);
		if (fd < 0) {
			warn("%s", _PATH_KSYMS);
		} else {
			close(fd);
			kname = _PATH_KSYMS;
		}
	}

	/*
	 * Determine name of kernel to use
	 */
	if (kname == NULL) {
#ifdef CPU_BOOTED_KERNEL
		/* 130 is 128 + '/' + '\0' */
		static char booted_kernel[130];
		int mib[2], rc;
		size_t len;
		struct stat st;

		mib[0] = CTL_MACHDEP;
		mib[1] = CPU_BOOTED_KERNEL;
		booted_kernel[0] = '/';
		booted_kernel[1] = '\0';
		len = sizeof(booted_kernel) - 2;
		rc = sysctl(&mib[0], 2, &booted_kernel[1], &len, NULL, 0);
		booted_kernel[sizeof(booted_kernel) - 1] = '\0';
		kname = (booted_kernel[1] == '/') ?
		    &booted_kernel[1] : &booted_kernel[0];
		if (rc != -1)
			rc = stat(kname, &st);
		if (rc == -1 || !S_ISREG(st.st_mode))
#endif /* CPU_BOOTED_KERNEL */
			kname = _PATH_UNIX;
	}
	/*
	 * Prelink to get file size
	 */
	if (prelink(kname, entry, out, 0, modobj, ldscript))
		errx(1, "can't prelink `%s' creating `%s'", modobj, out);
	if (Sflag == 0)
		fileopen |= OUTFILE_CREAT;

 	/*
 	 * Pre-open the 0-linked module to get the size information
 	 */
	if ((modfd = open(out, O_RDONLY, 0)) == -1)
		err(4, "%s", out);
	fileopen |= MOD_OPEN;

	/*
	 * stat for filesize to figure out string table size
	 */
	if (fstat(modfd, &stb) == -1)
	    err(3, "fstat `%s'", out);

	/*
	 * work out various sizes and fill in resrv bits
	 */
	if (mod_sizes(modfd, &modsize, &strtablen, &resrv, &stb) != 0)
		err(1, "can't get module sizes");

	/*
	 * Close the dummy module -- we have our sizing information.
	 */
	close(modfd);
	fileopen &= ~MOD_OPEN;

	/*
	 * Reserve the required amount of kernel memory -- this may fail
	 * to be successful.
	 */
	resrv.size = modsize;	/* size in bytes */
	resrv.name = modout;	/* objname w/o ".o" */
	resrv.slot = -1;	/* returned */
	resrv.addr = 0;		/* returned */

	if (verbose)
		warnx("reserving %lu bytes of memory", (unsigned long)modsize);
	if (ioctl(devfd, LMRESERV, &resrv) == -1)
		err(9, "can't reserve memory");

	fileopen |= PART_RESRV;

	/*
	 * Relink at kernel load address
	 */
	if (prelink(kname, entry, out, (void *)resrv.addr, modobj, ldscript))
		errx(1, "can't link `%s' creating `%s' bound to %p",
		     modobj, out, (void *)resrv.addr);

	/*
	 * Open the relinked module to load it...
	 */
	if ((modfd = open(out, O_RDONLY, 0)) == -1)
		err(4, "%s", out);
	fileopen |= MOD_OPEN;

	modentry = mod_load(modfd);
	if (debug)
		fprintf(stderr, "modentry = %p\n", modentry);

	if (symtab)
		mod_symload(strtablen);

	/*
	 * Save ourselves before disaster (potentitally) strikes...
	 */
	sync();

	if (noready)
		return 0;

	/*
	 * Trigger the module as loaded by calling the entry procedure;
	 * this will do all necessary table fixup to ensure that state
	 * is maintained on success, or blow everything back to ground
	 * zero on failure.
	 */
	if (ioctl(devfd, LMREADY, &modentry) == -1)
	    err(14, "error initializing module");

	/*
	 * Success!
	 */
	fileopen &= ~PART_RESRV;	/* loaded */
	printf("Module loaded as ID %d\n", resrv.slot);

	/*
	 * Execute the post-install program, if specified.
	 */
	if (post) {
		struct lmc_stat sbuf;
		char id[16], type[16];

		sbuf.id = resrv.slot;
		if (ioctl(devfd, LMSTAT, &sbuf) == -1)
			err(15, "error fetching module stats for post-install");
		(void)snprintf(id, sizeof(id), "%d", sbuf.id);
		(void)snprintf(type, sizeof(type), "0x%x", sbuf.type);
		if (sbuf.type == LM_DEV) {
			char arg3[16], arg4[16];
			int bmajor = LKM_BLOCK_MAJOR(sbuf.offset);
			int cmajor = LKM_CHAR_MAJOR(sbuf.offset);
			(void)snprintf(arg3, sizeof(arg3), "%d", cmajor);
			(void)snprintf(arg4, sizeof(arg4), "%d", bmajor);
			execl(post, post, id, type, arg3, arg4, 0);
		} else {
			char arg3[16];
			(void)snprintf(arg3, sizeof(arg3), "%ld",
			    (long)sbuf.offset);
			execl(post, post, id, type, arg3, 0);
		}
		err(16, "can't exec `%s'", post);
	}

	exit (0);
}
