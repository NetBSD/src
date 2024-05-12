/* $NetBSD: xlint.c,v 1.125 2024/05/12 18:49:36 rillig Exp $ */

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
 *	This product includes software developed by Jochen Pohl for
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
#if defined(__RCSID)
__RCSID("$NetBSD: xlint.c,v 1.125 2024/05/12 18:49:36 rillig Exp $");
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

typedef struct {
	char	**items;
	size_t	len;
	size_t	cap;
} list;

/* Parameters for the C preprocessor. */
static struct {
	list	flags;		/* flags always passed */
	list	lcflags;	/* flags, controlled by sflag/tflag */
	char	*outfile;	/* path name for preprocessed C source */
	int	output_fd;	/* file descriptor for outfile */
} cpp;

/* Parameters for lint1, which checks an isolated translation unit. */
static struct {
	list	flags;
	list	outfiles;
} lint1;

/* Parameters for lint2, which performs cross-translation-unit checks. */
static struct {
	list	flags;
	list	input_files;	/* without libraries */
	list	input_libraries;
	char	*output_library;
} lint2;

static const char *tmpdir;
static list default_libraries;
static list additional_libraries;
static list library_search_path;
static const char *libexec_dir;
static bool Cflag, dflag, Fflag, iflag, sflag, tflag, Vflag;
static char *output_filename;	/* filename for -o */
static bool seen_filename;

/*
 * The file that is currently written by a child process and should be removed
 * in case the child process fails.
 */
static const char *currfn;

#if !defined(TARGET_PREFIX)
#define	TARGET_PREFIX	""
#endif
static const char target_prefix[] = TARGET_PREFIX;


static void
list_add_ref(list *l, char *s)
{

	if (l->len >= l->cap) {
		l->cap = 2 * l->len + 16;
		l->items = xrealloc(l->items, sizeof(*l->items) * l->cap);
	}
	l->items[l->len++] = s;
}

static void
list_add(list *l, const char *s)
{

	list_add_ref(l, xstrdup(s));
}

static void
list_add_flag(list *l, int c)
{

	list_add(l, (const char[3]){ '-', (char)c, '\0' });
}

static void
list_add_unique(list *l, const char *s)
{

	for (size_t i = 0; i < l->len; i++)
		if (strcmp(l->items[i], s) == 0)
			return;
	list_add(l, s);
}

static void
list_add_all(list *dst, const list *src)
{

	for (size_t i = 0; i < src->len; i++)
		list_add(dst, src->items[i]);
}

static void
list_clear(list *l)
{

	while (l->len > 0)
		free(l->items[--l->len]);
}

static char *
concat2(const char *s1, const char *s2)
{

	size_t len1 = strlen(s1);
	size_t len2 = strlen(s2);
	char *s = xrealloc(NULL, len1 + len2 + 1);
	memcpy(s, s1, len1);
	memcpy(s + len1, s2, len2 + 1);
	return s;
}

static void
set_tmpdir(void)
{
	const char *tmp;
	size_t len;

	tmpdir = (tmp = getenv("TMPDIR")) != NULL && (len = strlen(tmp)) != 0
	    ? concat2(tmp, tmp[len - 1] == '/' ? "" : "/")
	    : xstrdup(_PATH_TMP);
}

/* Clean up after a signal or at the regular end. */
__dead static void
terminate(int signo)
{

	if (cpp.output_fd != -1)
		(void)close(cpp.output_fd);
	if (cpp.outfile != NULL) {
		const char *keep_env = getenv("LINT_KEEP_CPPOUT");
		bool keep = keep_env != NULL && (strcmp(keep_env, "yes") == 0
		    || (strcmp(keep_env, "on-error") == 0 && signo != 0));
		if (keep)
			(void)printf("lint: preprocessor output kept in %s\n",
			    cpp.outfile);
		else
			(void)remove(cpp.outfile);
	}

	for (size_t i = 0; i < lint1.outfiles.len; i++)
		(void)remove(lint1.outfiles.items[i]);

	if (lint2.output_library != NULL)
		(void)remove(lint2.output_library);

	if (currfn != NULL && currfn != cpp.outfile)
		(void)remove(currfn);

	if (signo != 0)
		(void)raise_default_signal(signo);
	exit(signo != 0 ? 1 : 0);
}

__dead __printflike(1, 2) static void
usage(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	(void)vfprintf(stderr, fmt, ap);
	va_end(ap);
	if (fmt[0] != '\0')
		(void)fprintf(stderr, "\n");

	const char *name = getprogname();
	int indent = (int)(strlen("usage: ") + strlen(name));
	(void)fprintf(stderr,
	    "usage: %s [-abceghprstvwxzFHPSTV] [-Alevel] [-i|-nu]\n"
	    "%*s [-Dname[=def]] [-Uname] [-Idirectory] "
	    "[-M...] [-W...] [-Z ...]\n"
	    "%*s [-ddirectory] [-Ldirectory] [-llibrary] [-ooutputfile]\n"
	    "%*s [-Bpath] [-X id,...] [-q id,...] [-R old=new] file ...\n",
	    name, indent, "", indent, "", indent, "");
	(void)fprintf(stderr,
	    "       %s [-abceghprstvwzFHPSTV] [-Alevel] -Clibrary\n"
	    "%*s [-Bpath] [-R old=new] file ...\n",
	    name, indent, "");
	terminate(-1);
}

static const char *
skip_last(const char *path, int delim)
{

	const char *base = path;
	for (const char *p = path; *p != '\0'; p++)
		if (*p == delim)
			base = p + 1;
	return base;
}

static bool
is_safe_shell(char ch)
{

	return ch_isalnum(ch)
	    || ch == '%' || ch == '+' || ch == ',' || ch == '-' || ch == '.'
	    || ch == '/' || ch == ':' || ch == '=' || ch == '@' || ch == '_';
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
run_child(const char *path, const list *args, const char *crfn, int fdout)
{

	if (Vflag) {
		print_sh_quoted(args->items[0]);
		for (size_t i = 1; i < args->len - 1; i++) {
			(void)printf(" ");
			print_sh_quoted(args->items[i]);
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
		(void)execvp(path, args->items);
		warn("cannot exec %s", path);
		_exit(1);
		/* NOTREACHED */
	}

	int status, rv, signo;
	while ((rv = wait(&status)) == -1 && errno == EINTR)
		continue;
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
run_cpp(const char *name)
{

	const char *cc = getenv("CC");
	if (cc == NULL)
		cc = DEFAULT_CC;

	char *abs_cc = findcc(cc);
	if (abs_cc == NULL && setenv("PATH", _PATH_DEFPATH, 1) == 0)
		abs_cc = findcc(cc);
	if (abs_cc == NULL) {
		(void)fprintf(stderr, "%s: %s: not found\n", getprogname(), cc);
		exit(EXIT_FAILURE);
	}

	list args = { NULL, 0, 0 };
	list_add_ref(&args, abs_cc);
	list_add_all(&args, &cpp.flags);
	list_add_all(&args, &cpp.lcflags);
	list_add(&args, name);
	list_add_ref(&args, NULL);

	/* Rewind after a possible previous run of cpp and lint1. */
	if (lseek(cpp.output_fd, 0, SEEK_SET) != 0) {
		warn("lseek");
		terminate(-1);
	}
	if (ftruncate(cpp.output_fd, 0) != 0) {
		warn("ftruncate");
		terminate(-1);
	}

	run_child(abs_cc, &args, cpp.outfile, cpp.output_fd);
	list_clear(&args);
}

static void
run_lint1(const char *out_fname)
{

	char *abs_lint1 = libexec_dir != NULL
	    ? concat2(libexec_dir, "/lint1")
	    : xasprintf("%s/%slint1", PATH_LIBEXEC, target_prefix);

	list args = { NULL, 0, 0 };
	list_add_ref(&args, abs_lint1);
	list_add_all(&args, &lint1.flags);
	list_add(&args, cpp.outfile);
	list_add(&args, out_fname);
	list_add_ref(&args, NULL);

	run_child(abs_lint1, &args, out_fname, -1);
	list_clear(&args);
}

static void
handle_filename(const char *name)
{

	const char *base = skip_last(name, '/');
	const char *suff = skip_last(base, '.');

	if (strcmp(suff, "ln") == 0) {
		/* only for lint2 */
		if (!iflag)
			list_add(&lint2.input_files, name);
		return;
	}

	if (strcmp(suff, "c") == 0)
		goto c_file;
	if (strncmp(base, "llib-l", 6) == 0 && base == suff)
		goto c_file;
	warnx("unknown file type: %s", name);
	return;

c_file:
	if (!iflag || seen_filename)
		(void)printf("%s:\n", Fflag ? name : base);

	/* build the name of the output file of lint1 */
	char *ofn;
	if (output_filename != NULL) {
		ofn = output_filename;
		output_filename = NULL;
	} else if (iflag) {
		size_t len = base == suff
		    ? strlen(base)
		    : (size_t)((suff - 1) - base);
		ofn = xasprintf("%.*s.ln", (int)len, base);
	} else {
		ofn = xasprintf("%slint1.XXXXXX", tmpdir);
		int fd = mkstemp(ofn);
		if (fd == -1) {
			warn("can't make temp");
			terminate(-1);
		}
		(void)close(fd);
	}
	if (!iflag)
		list_add(&lint1.outfiles, ofn);

	run_cpp(name);
	run_lint1(ofn);

	list_add(&lint2.input_files, ofn);
	free(ofn);
}

static bool
file_is_readable(const char *path)
{
	struct stat sbuf;

	return stat(path, &sbuf) == 0
	    && S_ISREG(sbuf.st_mode)
	    && access(path, R_OK) == 0;
}

static void
find_lib(const char *lib)
{
	char *lfn;

	for (size_t i = 0; i < library_search_path.len; i++) {
		const char *dir = library_search_path.items[i];
		lfn = xasprintf("%s/llib-l%s.ln", dir, lib);
		if (file_is_readable(lfn))
			goto found;
		free(lfn);

		lfn = xasprintf("%s/lint/llib-l%s.ln", dir, lib);
		if (file_is_readable(lfn))
			goto found;
		free(lfn);
	}

	warnx("cannot find llib-l%s.ln", lib);
	return;

found:
	list_add_ref(&lint2.input_libraries, concat2("-l", lfn));
	free(lfn);
}

static void
find_libs(const list *l)
{

	for (size_t i = 0; i < l->len; i++)
		find_lib(l->items[i]);
}

static void
run_lint2(void)
{

	char *abs_lint2 = libexec_dir != NULL
	    ? concat2(libexec_dir, "/lint2")
	    : xasprintf("%s/%slint2", PATH_LIBEXEC, target_prefix);

	list args = { NULL, 0, 0 };
	list_add_ref(&args, abs_lint2);
	list_add_all(&args, &lint2.flags);
	list_add_all(&args, &lint2.input_libraries);
	list_add_all(&args, &lint2.input_files);
	list_add_ref(&args, NULL);

	run_child(abs_lint2, &args, lint2.output_library, -1);
	list_clear(&args);
}

static void
cat(const list *srcs, const char *dest)
{
	int ifd, ofd;
	ssize_t rlen;
	char buf[0x4000];

	if ((ofd = open(dest, O_WRONLY | O_CREAT | O_TRUNC, 0666)) == -1) {
		warn("cannot open %s", dest);
		terminate(-1);
	}

	for (size_t i = 0; i < srcs->len; i++) {
		const char *src = srcs->items[i];
		if ((ifd = open(src, O_RDONLY)) == -1) {
			warn("cannot open %s", src);
			terminate(-1);
		}
		do {
			if ((rlen = read(ifd, buf, sizeof(buf))) == -1) {
				warn("read error on %s", src);
				terminate(-1);
			}
			if (write(ofd, buf, (size_t)rlen) != rlen) {
				warn("write error on %s", dest);
				terminate(-1);
			}
		} while (rlen == sizeof(buf));
		(void)close(ifd);
	}
	(void)close(ofd);
}

int
main(int argc, char *argv[])
{

	setprogname(argv[0]);
	set_tmpdir();

	cpp.outfile = concat2(tmpdir, "lint0.XXXXXX");
	cpp.output_fd = mkstemp(cpp.outfile);
	if (cpp.output_fd == -1) {
		warn("can't make temp");
		terminate(-1);
	}

	list_add(&cpp.flags, "-E");
	list_add(&cpp.flags, "-x");
	list_add(&cpp.flags, "c");
	list_add(&cpp.flags, "-U__GNUC__");
	list_add(&cpp.flags, "-U__PCC__");
	list_add(&cpp.flags, "-U__SSE__");
	list_add(&cpp.flags, "-U__SSE4_1__");
	list_add(&cpp.flags, "-Wp,-CC");
	list_add(&cpp.flags, "-Wcomment");
	list_add(&cpp.flags, "-D__LINT__");
	list_add(&cpp.flags, "-Dlint");	/* XXX don't define with -s */
	list_add(&cpp.flags, "-D__lint");
	list_add(&cpp.flags, "-D__lint__");

	list_add(&default_libraries, "c");

	if (signal(SIGHUP, terminate) == SIG_IGN)
		(void)signal(SIGHUP, SIG_IGN);
	(void)signal(SIGINT, terminate);
	(void)signal(SIGQUIT, terminate);
	(void)signal(SIGTERM, terminate);

	int c;
	while ((c = getopt(argc, argv,
	    "abcd:eghil:no:pq:rstuvwxzA:B:C:D:FHI:L:M:PR:STU:VW:X:Z:")) != -1) {
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
			list_add_flag(&lint1.flags, c);
			break;

		case 'A':
		case 'q':
		case 'R':
		case 'X':
			list_add_flag(&lint1.flags, c);
			list_add(&lint1.flags, optarg);
			break;

		case 'F':
			Fflag = true;
			/* FALLTHROUGH */
		case 'h':
			list_add_flag(&lint1.flags, c);
			list_add_flag(&lint2.flags, c);
			break;

		case 'i':
			if (Cflag)
				usage("%c and %s flags cannot be specified "
				    "together", 'C', "i");
			iflag = true;
			break;

		case 'n':
			list_clear(&default_libraries);
			break;

		case 'p':
			if (default_libraries.len > 0) {
				list_clear(&default_libraries);
				list_add(&default_libraries, "c");
			}
			list_add_flag(&lint1.flags, c);
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
			list_add_flag(&lint1.flags, c);
			list_add_flag(&lint2.flags, c);
			break;

		case 'S':
			if (tflag)
				usage("%c and %s flags cannot be specified "
				    "together", 'S', "t");
			list_add_flag(&lint1.flags, c);
			break;

		case 'T':
			list_add(&cpp.flags, "-I" PATH_STRICT_BOOL_INCLUDE);
			list_add_flag(&lint1.flags, c);
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
			list_add_flag(&lint1.flags, c);
			list_add_flag(&lint2.flags, c);
			break;
#endif

		case 'u':
		case 'x':
		case 'H':
			list_add_flag(&lint2.flags, c);
			break;

		case 'C':
			if (Cflag)
				usage("%c flag already specified", 'C');
			if (output_filename != NULL || iflag)
				usage("%c and %s flags cannot be specified "
				    "together", 'C', "o or i");
			Cflag = true;
			list_add_flag(&lint2.flags, c);
			list_add(&lint2.flags, optarg);
			lint2.output_library = xasprintf("llib-l%s.ln", optarg);
			list_clear(&default_libraries);
			break;

		case 'd':
			if (dflag)
				usage("%c flag already specified", 'd');
			dflag = true;
			list_add(&cpp.flags, "--sysroot");
			list_add(&cpp.flags, optarg);
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
			list_add_unique(&additional_libraries, optarg);
			break;

		case 'o':
			if (output_filename != NULL)
				usage("%c flag already specified", 'o');
			if (Cflag)
				usage("%c and %s flags cannot be specified "
				    "together", 'C', "o");
			output_filename = xstrdup(optarg);
			break;

		case 'L':
			list_add_unique(&library_search_path, optarg);
			break;

		case 'B':
			libexec_dir = xstrdup(optarg);
			break;

		case 'V':
			Vflag = true;
			break;

		case 'Z':
			list_add(&cpp.flags, optarg);
			break;

		default:
			usage("");
			/* NOTREACHED */
		}
	}
	argc -= optind;
	argv += optind;

	/*
	 * To avoid modifying getopt(3)'s state engine midstream, we explicitly
	 * accept just a few options after the first source file.
	 *
	 * In particular, only -l<lib> and -L<libdir> (and these with a space
	 * after -l or -L) are allowed.
	 */
	for (; argc > 0; argc--, argv++) {
		const char *arg = argv[0];

		if (arg[0] == '-') {
			list *lp;

			if (arg[1] == 'l')
				lp = &additional_libraries;
			else if (arg[1] == 'L')
				lp = &library_search_path;
			else
				usage("Unknown late option '%s'", arg);

			if (arg[2] != '\0')
				list_add_unique(lp, arg + 2);
			else if (argc > 1) {
				argc--, argv++;
				list_add_unique(lp, argv[0]);
			} else
				usage("Missing argument for l or L");
		} else {
			handle_filename(arg);
			seen_filename = true;
		}
	}

	if (!seen_filename)
		usage("Missing filename");

	if (iflag)
		terminate(0);

	if (output_filename == NULL) {
		const char *ks = getenv("LIBDIR");
		if (ks == NULL || ks[0] == '\0')
			ks = PATH_LINTLIB;
		list_add(&library_search_path, ks);
		find_libs(&additional_libraries);
		find_libs(&default_libraries);
	}

	run_lint2();

	if (output_filename != NULL)
		cat(&lint2.input_files, output_filename);

	if (Cflag)
		lint2.output_library = NULL;

	terminate(0);
	/* NOTREACHED */
}
