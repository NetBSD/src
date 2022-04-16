/* $NetBSD: xlint.c,v 1.91 2022/04/16 00:15:47 rillig Exp $ */

/*
 * Copyright (c) 1996 Christopher G. Demetriou.  All Rights Reserved.
 * Copyright (c) 1994, 1995 Jochen Pohl
 * All Rights Reserved.
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
 *      This product includes software developed by Jochen Pohl for
 *	The NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#include <sys/cdefs.h>
#if defined(__RCSID) && !defined(lint)
__RCSID("$NetBSD: xlint.c,v 1.91 2022/04/16 00:15:47 rillig Exp $");
#endif

#include <sys/param.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <errno.h>
#include <stdarg.h>
#include <fcntl.h>
#include <paths.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <util.h>

#include "lint.h"
#include "pathnames.h"
#include "findcc.h"

#define DEFAULT_PATH		_PATH_DEFPATH

/* Parameters for the C preprocessor. */
static struct {
	char	**flags;	/* flags always passed */
	char	**lcflags;	/* flags, controlled by sflag/tflag */
	char	*outfile;	/* path name for preprocessed C source */
	int	outfd;		/* file descriptor for outfile */
} cpp = { NULL, NULL, NULL, -1 };

/* Parameters for lint1, which checks an isolated translation unit. */
static struct {
	char	**flags;
	char	**outfiles;
} lint1;

/* Parameters for lint2, which performs cross-translation-unit checks. */
static struct {
	char	**flags;
	char	**infiles;	/* input files (without libraries) */
	char	**inlibs;	/* input libraries */
	char	*outlib;	/* output library that will be created */
} lint2;

/* directory for temporary files */
static	const char *tmpdir;

/* default libraries */
static	char	**deflibs;

/* additional libraries */
static	char	**libs;

/* search path for libraries */
static	char	**libsrchpath;

static const char *libexec_dir;

/* flags */
static	bool	iflag, oflag, Cflag, sflag, tflag, Fflag, dflag;

/* print the commands executed to run the stages of compilation */
static	bool	Vflag;

/* filename for oflag */
static	char	*outputfn;

/* reset after first .c source has been processed */
static	bool	first = true;

/*
 * name of a file which is currently written by a child and should
 * be removed after abnormal termination of the child
 */
static	const	char *currfn;

#if !defined(TARGET_PREFIX)
#define	TARGET_PREFIX	""
#endif
static const char target_prefix[] = TARGET_PREFIX;

static	void	fname(const char *);
static	void	runchild(const char *, char *const *, const char *, int);
static	void	findlibs(char *const *);
static	bool	rdok(const char *);
static	void	run_lint2(void);
static	void	cat(char *const *, const char *);

static char **
list_new(void)
{
	char **list;

	list = xcalloc(1, sizeof(*list));
	return list;
}

static void
list_add_ref(char ***lstp, char *s)
{
	char	**lst, **olst;
	int	i;

	olst = *lstp;
	for (i = 0; olst[i] != NULL; i++)
		continue;
	lst = xrealloc(olst, (i + 2) * sizeof(*lst));
	lst[i] = s;
	lst[i + 1] = NULL;
	*lstp = lst;
}

static void
list_add(char ***lstp, const char *s)
{

	list_add_ref(lstp, xstrdup(s));
}

static void
list_add_unique(char ***lstp, const char *s)
{

	for (char **p = *lstp; *p != NULL; p++)
		if (strcmp(*p, s) == 0)
			return;
	list_add(lstp, s);
}

static void
list_add_all(char ***destp, char *const *src)
{
	int	i, k;
	char	**dest, **odest;

	odest = *destp;
	for (i = 0; odest[i] != NULL; i++)
		continue;
	for (k = 0; src[k] != NULL; k++)
		continue;
	dest = xrealloc(odest, (i + k + 1) * sizeof(*dest));
	for (k = 0; src[k] != NULL; k++)
		dest[i + k] = xstrdup(src[k]);
	dest[i + k] = NULL;
	*destp = dest;
}

static void
list_clear(char ***lstp)
{
	char	*s;
	int	i;

	for (i = 0; (*lstp)[i] != NULL; i++)
		continue;
	while (i-- > 0) {
		s = (*lstp)[i];
		(*lstp)[i] = NULL;
		free(s);
	}
}

static void
pass_to_lint1(const char *opt)
{

	list_add(&lint1.flags, opt);
}

static void
pass_flag_to_lint1(int flag)
{
	char buf[3];

	buf[0] = '-';
	buf[1] = (char)flag;
	buf[2] = '\0';
	pass_to_lint1(buf);
}

static void
pass_to_lint2(const char *opt)
{

	list_add(&lint2.flags, opt);
}

static void
pass_flag_to_lint2(int flag)
{
	char buf[3];

	buf[0] = '-';
	buf[1] = (char)flag;
	buf[2] = '\0';
	pass_to_lint2(buf);
}

static void
pass_to_cpp(const char *opt)
{

	list_add(&cpp.flags, opt);
}

static char *
concat2(const char *s1, const char *s2)
{
	char	*s;

	s = xmalloc(strlen(s1) + strlen(s2) + 1);
	(void)strcpy(s, s1);
	(void)strcat(s, s2);

	return s;
}

/*
 * Clean up after a signal.
 */
static void __attribute__((__noreturn__))
terminate(int signo)
{
	int	i;

	if (cpp.outfd != -1)
		(void)close(cpp.outfd);
	if (cpp.outfile != NULL) {
		if (signo != 0 && getenv("LINT_KEEP_CPPOUT_ON_ERROR") != NULL)
			printf("lint: preprocessor output kept in %s\n",
			    cpp.outfile);
		else
			(void)remove(cpp.outfile);
	}

	if (lint1.outfiles != NULL) {
		for (i = 0; lint1.outfiles[i] != NULL; i++)
			(void)remove(lint1.outfiles[i]);
	}

	if (lint2.outlib != NULL)
		(void)remove(lint2.outlib);

	if (currfn != NULL)
		(void)remove(currfn);

	if (signo != 0)
		(void)raise_default_signal(signo);
	exit(signo != 0 ? 1 : 0);
}

/*
 * Returns a pointer to the last component of strg after delim.
 * Returns strg if the string does not contain delim.
 */
static const char *
lbasename(const char *strg, int delim)
{
	const	char *cp, *cp1, *cp2;

	cp = cp1 = cp2 = strg;
	while (*cp != '\0') {
		if (*cp++ == delim) {
			cp2 = cp1;
			cp1 = cp;
		}
	}
	return *cp1 == '\0' ? cp2 : cp1;
}

static void __attribute__((__noreturn__, __format__(__printf__, 1, 2)))
usage(const char *fmt, ...)
{
	const char *name;
	int indent;
	va_list ap;

	name = getprogname();
	fprintf(stderr, "%s: ", name);
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fprintf(stderr, "\n");

	indent = (int)(strlen("usage: ") + strlen(name));
	(void)fprintf(stderr,
	    "usage: %s [-abceghprvwxzHFST] [-s|-t] [-i|-nu]\n"
	    "%*s [-Dname[=def]] [-Uname] [-Idirectory] [-Z <cpparg>]\n"
	    "%*s [-Ldirectory] [-llibrary] [-ooutputfile]\n"
	    "%*s [-X <id>[,<id>]...] [-Ac11] file...\n",
	    name, indent, "", indent, "", indent, "");
	(void)fprintf(stderr,
	    "       %s [-abceghprvwzHFST] [-s|-t] -Clibrary\n"
	    "%*s [-Dname[=def]] [-Uname] [-Idirectory] [-Z <cpparg>]\n"
	    "%*s [-Bpath] [-X <id>[,<id>]...] [-R old=new] file ...\n",
	    name, indent, "", indent, "");
	terminate(-1);
}


int
main(int argc, char *argv[])
{
	int	c;
	char	*tmp;
	size_t	len;
	const char *ks;

	setprogname(argv[0]);

	if ((tmp = getenv("TMPDIR")) == NULL || (len = strlen(tmp)) == 0) {
		tmpdir = xstrdup(_PATH_TMP);
	} else {
		tmpdir = concat2(tmp, tmp[len - 1] == '/' ? "" : "/");
	}

	cpp.outfile = concat2(tmpdir, "lint0.XXXXXX");
	cpp.outfd = mkstemp(cpp.outfile);
	if (cpp.outfd == -1) {
		warn("can't make temp");
		terminate(-1);
	}

	lint1.outfiles = list_new();
	lint2.infiles = list_new();
	cpp.flags = list_new();
	cpp.lcflags = list_new();
	lint1.flags = list_new();
	lint2.flags = list_new();
	lint2.inlibs = list_new();
	deflibs = list_new();
	libs = list_new();
	libsrchpath = list_new();

	pass_to_cpp("-E");
	pass_to_cpp("-x");
	pass_to_cpp("c");
	pass_to_cpp("-U__GNUC__");
	pass_to_cpp("-U__PCC__");
	pass_to_cpp("-U__SSE__");
	pass_to_cpp("-U__SSE4_1__");
	pass_to_cpp("-Wp,-CC");
	pass_to_cpp("-Wcomment");
	pass_to_cpp("-D__LINT__");
	pass_to_cpp("-Dlint");		/* XXX don't def. with -s */
	pass_to_cpp("-D__lint");
	pass_to_cpp("-D__lint__");

	list_add(&deflibs, "c");

	if (signal(SIGHUP, terminate) == SIG_IGN)
		(void)signal(SIGHUP, SIG_IGN);
	(void)signal(SIGINT, terminate);
	(void)signal(SIGQUIT, terminate);
	(void)signal(SIGTERM, terminate);
	while ((c = getopt(argc, argv,
	    "abcd:eghil:no:prstuvwxzA:B:C:D:FHI:L:M:PR:STU:VW:X:Z:")) != -1) {
		switch (c) {

		case 'a':
		case 'b':
		case 'c':
		case 'e':
		case 'g':
		case 'r':
		case 'v':
		case 'w':
		case 'z':
		case 'P':
			pass_flag_to_lint1(c);
			break;

		case 'A':
		case 'R':
		case 'X':
			pass_flag_to_lint1(c);
			pass_to_lint1(optarg);
			break;

		case 'F':
			Fflag = true;
			/* FALLTHROUGH */
		case 'u':
		case 'h':
			pass_flag_to_lint1(c);
			pass_flag_to_lint2(c);
			break;

		case 'i':
			if (Cflag)
				usage("%c and %s flags cannot be specified "
				    "together", 'C', "i");
			iflag = true;
			break;

		case 'n':
			list_clear(&deflibs);
			break;

		case 'p':
			if (*deflibs != NULL) {
				list_clear(&deflibs);
				list_add(&deflibs, "c");
			}
			pass_flag_to_lint1(c);
			break;

		case 's':
			if (tflag)
				usage("%c and %s flags cannot be specified "
				    "together", 's', "t");
			list_clear(&cpp.lcflags);
			list_add(&cpp.lcflags, "-trigraphs");
			list_add(&cpp.lcflags, "-Wtrigraphs");
			list_add(&cpp.lcflags, "-pedantic");
			list_add(&cpp.lcflags, "-D__STRICT_ANSI__");
			sflag = true;
			pass_flag_to_lint1(c);
			pass_flag_to_lint2(c);
			break;

		case 'S':
			if (tflag)
				usage("%c and %s flags cannot be specified "
				    "together", 'S', "t");
			pass_flag_to_lint1(c);
			break;

		case 'T':
			pass_to_cpp("-I" PATH_STRICT_BOOL_INCLUDE);
			pass_flag_to_lint1(c);
			break;

#if !HAVE_NBTOOL_CONFIG_H
		case 't':
			if (sflag)
				usage("%c and %s flags cannot be specified "
				    "together", 's', "t");
			tflag = true;
			list_clear(&cpp.lcflags);
			list_add(&cpp.lcflags, "-traditional");
			list_add(&cpp.lcflags, "-Wtraditional");
			list_add(&cpp.lcflags, "-D" MACHINE);
			list_add(&cpp.lcflags, "-D" MACHINE_ARCH);
			pass_flag_to_lint1(c);
			pass_flag_to_lint2(c);
			break;
#endif

		case 'x':
		case 'H':
			pass_flag_to_lint2(c);
			break;

		case 'C':
			if (Cflag)
				usage("%c flag already specified", 'C');
			if (oflag || iflag)
				usage("%c and %s flags cannot be specified "
				    "together", 'C', "o or i");
			Cflag = true;
			pass_flag_to_lint2(c);
			pass_to_lint2(optarg);
			lint2.outlib = xasprintf("llib-l%s.ln", optarg);
			list_clear(&deflibs);
			break;

		case 'd':
			if (dflag)
				usage("%c flag already specified", 'd');
			dflag = true;
			pass_to_cpp("-nostdinc");
			pass_to_cpp("-isystem");
			pass_to_cpp(optarg);
			break;

		case 'D':
		case 'I':
		case 'M':
		case 'U':
		case 'W':
			list_add_ref(&cpp.flags,
			    xasprintf("-%c%s", c, optarg));
			break;

		case 'l':
			list_add_unique(&libs, optarg);
			break;

		case 'o':
			if (oflag)
				usage("%c flag already specified", 'o');
			if (Cflag)
				usage("%c and %s flags cannot be specified "
				    "together", 'C', "o");
			oflag = true;
			outputfn = xstrdup(optarg);
			break;

		case 'L':
			list_add_unique(&libsrchpath, optarg);
			break;

		case 'B':
			libexec_dir = xstrdup(optarg);
			break;

		case 'V':
			Vflag = true;
			break;

		case 'Z':
			pass_to_cpp(optarg);
			break;

		default:
			usage("Unknown flag %c", c);
			/* NOTREACHED */
		}
	}
	argc -= optind;
	argv += optind;

	/*
	 * To avoid modifying getopt(3)'s state engine midstream, we
	 * explicitly accept just a few options after the first source file.
	 *
	 * In particular, only -l<lib> and -L<libdir> (and these with a space
	 * after -l or -L) are allowed.
	 */
	while (argc > 0) {
		const char *arg = argv[0];

		if (arg[0] == '-') {
			char ***list;

			/* option */
			if (arg[1] == 'l')
				list = &libs;
			else if (arg[1] == 'L')
				list = &libsrchpath;
			else {
				usage("Unknown argument %s", arg);
				/* NOTREACHED */
			}

			if (arg[2] != '\0')
				list_add_unique(list, arg + 2);
			else if (argc > 1) {
				argc--;
				list_add_unique(list, *++argv);
			} else
				usage("Missing argument for l or L");
		} else {
			/* filename */
			fname(arg);
			first = false;
		}
		argc--;
		argv++;
	}

	if (first)
		usage("Missing filename");

	if (iflag)
		terminate(0);

	if (!oflag) {
		if ((ks = getenv("LIBDIR")) == NULL || strlen(ks) == 0)
			ks = PATH_LINTLIB;
		list_add(&libsrchpath, ks);
		findlibs(libs);
		findlibs(deflibs);
	}

	run_lint2();

	if (oflag)
		cat(lint2.infiles, outputfn);

	if (Cflag)
		lint2.outlib = NULL;

	terminate(0);
	/* NOTREACHED */
}

/*
 * Read a file name from the command line
 * and pass it through lint1 if it is a C source.
 */
static void
fname(const char *name)
{
	const	char *bn, *suff;
	char	**args, *ofn, *pathname;
	const char *CC;
	size_t	len;
	int	fd;

	bn = lbasename(name, '/');
	suff = lbasename(bn, '.');

	if (strcmp(suff, "ln") == 0) {
		/* only for lint2 */
		if (!iflag)
			list_add(&lint2.infiles, name);
		return;
	}

	if (strcmp(suff, "c") != 0 &&
	    (strncmp(bn, "llib-l", 6) != 0 || bn != suff)) {
		warnx("unknown file type: %s", name);
		return;
	}

	if (!iflag || !first)
		(void)printf("%s:\n", Fflag ? name : bn);

	/* build the name of the output file of lint1 */
	if (oflag) {
		ofn = outputfn;
		outputfn = NULL;
		oflag = false;
	} else if (iflag) {
		len = bn == suff ? strlen(bn) : (size_t)((suff - 1) - bn);
		ofn = xasprintf("%.*s.ln", (int)len, bn);
	} else {
		ofn = xasprintf("%slint1.XXXXXX", tmpdir);
		fd = mkstemp(ofn);
		if (fd == -1) {
			warn("can't make temp");
			terminate(-1);
		}
		(void)close(fd);
	}
	if (!iflag)
		list_add(&lint1.outfiles, ofn);

	args = list_new();

	/* run cc */
	if ((CC = getenv("CC")) == NULL)
		CC = DEFAULT_CC;
	if ((pathname = findcc(CC)) == NULL)
		if (setenv("PATH", DEFAULT_PATH, 1) == 0)
			pathname = findcc(CC);
	if (pathname == NULL) {
		(void)fprintf(stderr, "%s: %s: not found\n", getprogname(), CC);
		exit(EXIT_FAILURE);
	}

	list_add(&args, pathname);
	list_add_all(&args, cpp.flags);
	list_add_all(&args, cpp.lcflags);
	list_add(&args, name);

	/* we reuse the same tmp file for cpp output, so rewind and truncate */
	if (lseek(cpp.outfd, (off_t)0, SEEK_SET) != 0) {
		warn("lseek");
		terminate(-1);
	}
	if (ftruncate(cpp.outfd, (off_t)0) != 0) {
		warn("ftruncate");
		terminate(-1);
	}

	runchild(pathname, args, cpp.outfile, cpp.outfd);
	free(pathname);
	list_clear(&args);

	/* run lint1 */

	if (libexec_dir == NULL) {
		pathname = xasprintf("%s/%slint1",
		    PATH_LIBEXEC, target_prefix);
	} else {
		/*
		 * XXX Unclear whether we should be using target_prefix
		 * XXX here.  --thorpej@wasabisystems.com
		 */
		pathname = concat2(libexec_dir, "/lint1");
	}

	list_add(&args, pathname);
	list_add_all(&args, lint1.flags);
	list_add(&args, cpp.outfile);
	list_add(&args, ofn);

	runchild(pathname, args, ofn, -1);
	free(pathname);
	list_clear(&args);

	list_add(&lint2.infiles, ofn);
	free(ofn);

	free(args);
}

static bool
is_safe_shell(char ch)
{

	return ch_isalnum(ch) || ch == '%' || ch == '+' || ch == ',' ||
	       ch == '-' || ch == '.' || ch == '/' || ch == ':' ||
	       ch == '=' || ch == '@' || ch == '_';
}

static void
print_sh_quoted(const char *s)
{

	if (s[0] == '\0')
		goto needs_quoting;
	for (const char *p = s; *p != '\0'; p++)
		if (!is_safe_shell(*p))
			goto needs_quoting;

	(void)printf("%s", s);
	return;

needs_quoting:
	(void)putchar('\'');
	for (const char *p = s; *p != '\0'; p++) {
		if (*p == '\'')
			(void)printf("'\\''");
		else
			(void)putchar(*p);
	}
	(void)putchar('\'');
}

static void
runchild(const char *path, char *const *args, const char *crfn, int fdout)
{
	int	status, rv, signo, i;

	if (Vflag) {
		print_sh_quoted(args[0]);
		for (i = 1; args[i] != NULL; i++) {
			(void)printf(" ");
			print_sh_quoted(args[i]);
		}
		(void)printf("\n");
	}

	currfn = crfn;

	(void)fflush(stdout);

	switch (vfork()) {
	case -1:
		warn("cannot fork");
		terminate(-1);
		/* NOTREACHED */
	default:
		/* parent */
		break;
	case 0:
		/* child */

		/* set up the standard output if necessary */
		if (fdout != -1) {
			(void)dup2(fdout, STDOUT_FILENO);
			(void)close(fdout);
		}
		(void)execvp(path, args);
		warn("cannot exec %s", path);
		_exit(1);
		/* NOTREACHED */
	}

	while ((rv = wait(&status)) == -1 && errno == EINTR) ;
	if (rv == -1) {
		warn("wait");
		terminate(-1);
	}
	if (WIFSIGNALED(status)) {
		signo = WTERMSIG(status);
#if HAVE_DECL_SYS_SIGNAME
		warnx("%s got SIG%s", path, sys_signame[signo]);
#else
		warnx("%s got signal %d", path, signo);
#endif
		terminate(-1);
	}
	if (WEXITSTATUS(status) != 0)
		terminate(-1);
	currfn = NULL;
}

static void
findlib(const char *lib)
{
	char *const *dir;
	char *lfn;

	for (dir = libsrchpath; *dir != NULL; dir++) {
		lfn = xasprintf("%s/llib-l%s.ln", *dir, lib);
		if (rdok(lfn))
			goto found;
		free(lfn);

		lfn = xasprintf("%s/lint/llib-l%s.ln", *dir, lib);
		if (rdok(lfn))
			goto found;
		free(lfn);
	}

	warnx("cannot find llib-l%s.ln", lib);
	return;

found:
	list_add_ref(&lint2.inlibs, concat2("-l", lfn));
	free(lfn);
}

static void
findlibs(char *const *liblst)
{
	char *const *p;

	for (p = liblst; *p != NULL; p++)
		findlib(*p);
}

static bool
rdok(const char *path)
{
	struct	stat sbuf;

	if (stat(path, &sbuf) == -1)
		return false;
	if (!S_ISREG(sbuf.st_mode))
		return false;
	if (access(path, R_OK) == -1)
		return false;
	return true;
}

static void
run_lint2(void)
{
	char	*path, **args;

	args = list_new();

	if (libexec_dir == NULL) {
		path = xasprintf("%s/%slint2", PATH_LIBEXEC, target_prefix);
	} else {
		/*
		 * XXX Unclear whether we should be using target_prefix
		 * XXX here.  --thorpej@wasabisystems.com
		 */
		path = concat2(libexec_dir, "/lint2");
	}

	list_add(&args, path);
	list_add_all(&args, lint2.flags);
	list_add_all(&args, lint2.inlibs);
	list_add_all(&args, lint2.infiles);

	runchild(path, args, lint2.outlib, -1);
	free(path);
	list_clear(&args);
	free(args);
}

static void
cat(char *const *srcs, const char *dest)
{
	int	ifd, ofd, i;
	char	*src, *buf;
	ssize_t	rlen;

	if ((ofd = open(dest, O_WRONLY | O_CREAT | O_TRUNC, 0666)) == -1) {
		warn("cannot open %s", dest);
		terminate(-1);
	}

	buf = xmalloc(MBLKSIZ);

	for (i = 0; (src = srcs[i]) != NULL; i++) {
		if ((ifd = open(src, O_RDONLY)) == -1) {
			free(buf);
			warn("cannot open %s", src);
			terminate(-1);
		}
		do {
			if ((rlen = read(ifd, buf, MBLKSIZ)) == -1) {
				free(buf);
				warn("read error on %s", src);
				terminate(-1);
			}
			if (write(ofd, buf, (size_t)rlen) == -1) {
				free(buf);
				warn("write error on %s", dest);
				terminate(-1);
			}
		} while (rlen == MBLKSIZ);
		(void)close(ifd);
	}
	(void)close(ofd);
	free(buf);
}
