/*	$NetBSD: gzip.c,v 1.2 2001/01/04 16:17:15 lukem Exp $	*/

/*
 * Copyright (c) 1997, 1998 Matthew R. Green
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * gzip.c -- GPL free gzip using zlib.
 *
 * Very minor portions of this code are (very loosely) derived from
 * the minigzip.c in the zlib distribution.
 */

#include <sys/param.h>
#include <sys/stat.h>

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <err.h>
#include <errno.h>
#include <zlib.h>
#include <fts.h>

#ifndef GZ_SUFFIX
# define GZ_SUFFIX ".gz"
#endif

#define SUFFIX_LEN sizeof(GZ_SUFFIX)

#define BUFLEN 4096

extern	char	*__progname;	/* XXX */

static	char	gzip_version[] = "NetBSD gzip 1.0";

static	char	gzipflags[3] = "";	/* `w' or `r', possible with [1-9] */
static	int	cflag;			/* stdout mode */
static	int	dflag;			/* decompress mode */
static	int	fflag;			/* force mode */
static	int	qflag;			/* quiet mode */
static	int	rflag;			/* recursive mode */
static	int	tflag;			/* test */
static	int	vflag;			/* verbose mode */
static	char	*Sflag = GZ_SUFFIX;	/* suffix (.gz) */

static	int	suffix_len = SUFFIX_LEN; /* length of suffix; includes nul */
static	char	*newfile;		/* name of newly created file */
static	char	*infile;		/* name of file coming in */

static	void	usage __P((void));
static	void	display_version __P((void));
static	void	gz_compress __P((FILE *, gzFile));
static	off_t	gz_uncompress __P((gzFile, FILE *));
static	ssize_t	file_compress __P((char *));
static	ssize_t	file_uncompress __P((char *));
static	void	handle_pathname __P((char *));
static	void	handle_file __P((char *, struct stat *));
static	void	handle_dir __P((char *, struct stat *));
static	void	handle_stdin __P((void));
static	void	handle_stdout __P((void));
static	void	print_verbage __P((char *, char *, ssize_t, ssize_t));
static	void	print_test __P((char *, int));

int main __P((int, char *p[]));

int
main(argc, argv)
	int argc;
	char *argv[];
{
	int ch;

	gzipflags[0] = 'w';
	gzipflags[1] = '\0';

	/*
	 * XXX
	 * handle being called `gunzip', `zcat' and `gzcat'
	 */
	if (strcmp(__progname, "gunzip") == 0)
		dflag = 1;
	else if (strcmp(__progname, "zcat") == 0 ||
		 strcmp(__progname, "gzcat") == 0)
		dflag = cflag = 1;

	while ((ch = getopt(argc, argv, "cdfhnqrS:tvV123456789")) != -1)
		switch(ch) {
		case 'c':
			cflag = 1;
			break;
		case 'd':
			dflag = 1;
			break;
		case 'f':
			fflag = 1;
			break;
		case 'h':
			usage();
			/* NOTREACHED */
		case 'q':
			qflag = 1;
			break;
		case 'r':
			rflag = 1;
			break;
		case 'S':
			Sflag = optarg;
			suffix_len = strlen(Sflag) + 1;
			break;
		case 't':
			cflag = 1;
			tflag = 1;
			dflag = 1;
			break;
		case 'v':
			vflag = 1;
			break;
		case 'V':
			display_version();
			/* NOTREACHED */
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			gzipflags[1] = (char)ch;
			gzipflags[2] = '\0';
			break;
		}
	argv += optind;
	argc -= optind;
	if (dflag)
		gzipflags[0] = 'r';

	if (argc == 0) {
		if (dflag)	/* stdin mode */
			handle_stdin();
		else		/* stdout mode */
			handle_stdout();
	} else {
		do {
			handle_pathname(argv[0]);
		} while (argv++, --argc);
	}
	exit(0);
}

/* compress input to output then close both files */
static void
gz_compress(in, out)
	FILE   *in;
	gzFile out;
{
	char buf[BUFLEN];
	ssize_t len;
	int i;

	for (;;) {
		len = fread(buf, 1, sizeof(buf), in);
		if (ferror(in)) {
			if (qflag)
				exit(1);
			err(1, "fread");
		}
		if (len == 0)
			break;

		if ((ssize_t)gzwrite(out, buf, len) != len) {
			if (qflag)
				exit(1);
			err(1, gzerror(out, &i));
		}
	}
	if (fclose(in) < 0) {
		if (qflag)
			exit(1);
		err(1, "failed fclose");
	}
	if (gzclose(out) != Z_OK) {
		if (qflag)
			exit(1);
		err(1, "failed gzclose");
	}
}

/* uncompress input to output then close the input */
static off_t
gz_uncompress(in, out)
	gzFile in;
	FILE   *out;
{
	char buf[BUFLEN];
	off_t size;
	ssize_t len;
	int i;

	for (size = 0;;) {
		len = gzread(in, buf, sizeof(buf));

		if (len < 0) {
			if (tflag) {
				print_test(infile, 0);
				return (0);
			} else {
				if (qflag)
					exit(1);
				err(1, gzerror(in, &i));
			}
		} else if (len == 0) {
			if (tflag)
				print_test(infile, 1);
			break;
		}

		size += len;

		/* don't write anything with -t */
		if (tflag)
			continue;

		if (fwrite(buf, 1, (unsigned)len, out) != (ssize_t)len) {
			if (qflag)
				exit(1);
			err(1, "failed fwrite");
		}
	}
	if (gzclose(in) != Z_OK) {
		if (qflag)
			exit(1);
		err(1, "failed gzclose");
	}
	if (tflag)
		return (size);
	return(0);
}

/*
 * compress the given file: create a corresponding .gz file and remove the
 * original.
 */
static ssize_t
file_compress(file)
	char  *file;
{
	char outfile[MAXPATHLEN];
	FILE  *in;
	gzFile out;
	struct stat sb;
	ssize_t size;

	if (cflag == 0) {
		(void)strncpy(outfile, file, MAXPATHLEN - suffix_len);
		(void)strcat(outfile, Sflag);

		if (fflag == 0) {
			if (stat(outfile, &sb) == 0) {
				warnx("%s already exists -- skipping", outfile);
				goto lose;
			}
			if (stat(file, &sb) == 0 && sb.st_nlink > 1) {
				warnx("%s has %d other link%s -- skipping", file,
				    sb.st_nlink-1, sb.st_nlink == 1 ? "" : "s");
				goto lose;
			}
		}
	}
	in = fopen(file, "r");
	if (in == 0) {
		if (qflag)
			exit(1);
		err(1, "can't fopen %s", file);
	}

	if (cflag == 0)
		out = gzopen(outfile, gzipflags);
	else
		out = gzdopen(STDOUT_FILENO, gzipflags);

	if (out == 0) {
		if (qflag)
			exit(1);
		err(1, "can't gz%sopen %s",
		    cflag ? "d"         : "",
		    cflag ? "stdout"    : outfile);
	}

	gz_compress(in, out);

	/*
	 * if we compressed to stdout, we don't know the size and
	 * we don't know the new file name, punt.  if we can't stat
	 * the file, whine, otherwise set the size from the stat
	 * buffer.  we only blow away the file if we can stat the
	 * output, just in case.
	 */
	if (cflag == 0) {
		if (stat(outfile, &sb) < 0 && qflag != 0) {
			warn("couldn't stat: %s", outfile);
			warnx("leaving original %s", newfile);
			size = 0;
		} else {
			unlink(file);
			size = sb.st_size;
		}
		newfile = outfile;
	} else {
lose:
		size = 0;
		newfile = 0;
	}
	return (size);
}

/* uncompress the given file and remove the original */
static ssize_t
file_uncompress(file)
	char  *file;
{
	struct stat sb;
	char buf[MAXPATHLEN];
	char *outfile = buf, *s;
	FILE  *out;
	gzFile in;
	off_t size;
	ssize_t len = strlen(file);

	if (cflag == 0) {
		s = &file[len - suffix_len + 1];
		if (strncmp(s, Sflag, suffix_len) == 0) {
			(void)strncpy(outfile, file, len - suffix_len + 1);
			outfile[len - suffix_len + 1] = '\0';
		} else if (qflag == 0)
			err(1, "unknown suffix %s", s);

		if (fflag == 0) {
			if (stat(outfile, &sb) == 0) {
				warnx("%s already exists -- skipping", outfile);
				goto lose;
			}
			if (stat(file, &sb) == 0 && sb.st_nlink > 1) {
				warnx("%s has %d other link%s -- skipping", file,
				    sb.st_nlink-1, sb.st_nlink == 1 ? "" : "s");
				goto lose;
			}
		}
	}
	in = gzopen(file, gzipflags);
	if (in == NULL) {
		if (qflag)
			exit(1);
		err(1, "can't gzopen %s", file);
	}

	if (cflag == 0) {
		out = fopen(outfile, "w");
		if (out == NULL) {
			if (qflag)
				exit(1);
			err(1, "can't fopen %s", outfile);
		}
	} else
		out = stdout;

	if ((size = gz_uncompress(in, out)) == 0)
		goto lose;

	/* if testing, or we uncompressed to stdout, this is all we need */
	if (tflag || cflag)
		return (size);

	/*
	 * if we create a file...
	 */
	if (cflag == 0) {

		/* close the file */
		if (fclose(out)) {
			if (qflag)
				exit(1);
			err(1, "failed fclose");
		}

		/*
		 * if we can't stat the file, or we are uncompressing to
		 * stdin, don't remove the file.
		 */
		if (stat(outfile, &sb) < 0) {
			if (qflag == 0)
	warn("couldn't stat (leaving original): %s", outfile);
			goto lose;
		}
		newfile = outfile;
		if (sb.st_size != size) {
	warn("stat gave different size: %lld != %lld (leaving original)",
			    (long long)size, (long long)sb.st_size);
			goto lose;
		}
		unlink(file);
		size = sb.st_size;
	}
	return (size);

lose:
	newfile = 0;
	return (0);
}

static void
handle_stdin()
{
	gzFile *file;

	if (fflag == 0 && isatty(STDIN_FILENO)) {
		if (qflag == 0)
			warnx("standard input is a terminal -- ignoring");
		return;
	}
	file = gzdopen(STDIN_FILENO, gzipflags);
	if (file == NULL) {
		if (qflag)
			exit(1);
		err(1, "can't gzdopen stdin");
	}
	(void)gz_uncompress(file, stdout);
}

static void
handle_stdout()
{
	gzFile *file;

	if (fflag == 0 && isatty(STDOUT_FILENO)) {
		if (qflag == 0)
			warnx("standard output is a terminal -- ignoring");
		return;
	}
	file = gzdopen(STDOUT_FILENO, gzipflags);
	if (file == NULL) {
		if (qflag)
			exit(1);
		err(1, "can't gzdopen stdout");
	}
	gz_compress(stdin, file);
}

/* do what is asked for, for the path name */
static void
handle_pathname(path)
	char *path;
{
	char *opath = path, *s = 0;
	ssize_t len;
	struct stat sb;

	/* check for stdout/stdin */
	if (path[0] == '-' && path[1] == '\0') {
		if (dflag)
			handle_stdin();
		else
			handle_stdout();
	}
retry:
	if (stat(path, &sb) < 0) {
		/* lets try <path>.gz if we're decompressing */
		if (dflag && s == 0 && errno == ENOENT) {
			len = strlen(path);
			s = malloc(len + suffix_len);
			if (s == 0) {
				if (qflag)
					exit(1);
				err(1, "malloc");
			}
			memmove(s, path, len);
			memmove(&s[len], Sflag, suffix_len);
			path = s;
			goto retry;
		}
		if (qflag == 0)
			warn("can't stat: %s", opath);
		goto out;
	}

	if (S_ISDIR(sb.st_mode)) {
		if (rflag)
			handle_dir(path, &sb);
		else if (qflag == 0)
			warn("%s is a directory", path);
		goto out;
	}

	if (S_ISREG(sb.st_mode))
		handle_file(path, &sb);

out:
	if (s)
		free(s);
	return;
}

/* compress/decompress a file */
static void
handle_file(file, sbp)
	char *file;
	struct stat *sbp;
{
	ssize_t usize, gsize;

	infile = file;
	if (dflag) {
		usize = file_uncompress(file);
		if (usize == 0)
			return;
		gsize = sbp->st_size;
	} else {
		gsize = file_compress(file);
		if (gsize == 0)
			return;
		usize = sbp->st_size;
	}

	if (vflag)
		print_verbage(file, cflag ? newfile : 0, usize, gsize);
}

/* this is used with -r to recursively decend directories */
static void
handle_dir(dir, sbp)
	char *dir;
	struct stat *sbp;
{
	char	*path_argv[2];
	FTS	*fts;
	FTSENT	*entry;

	path_argv[0] = dir;
	path_argv[1] = 0;
	fts = fts_open(path_argv, FTS_PHYSICAL, NULL);
	if (fts == NULL) {
		warn("couldn't fts_open %s", dir);
		return;
	}

	while ((entry = fts_read(fts))) {
		switch(entry->fts_info) {
		case FTS_D:
		case FTS_DP:
			continue;

		case FTS_DNR:
		case FTS_ERR:
		case FTS_NS:
			if (qflag == 0) {
				warn("%s", entry->fts_path);
				continue;
			}
		case FTS_F:
			handle_file(entry->fts_name, entry->fts_statp);
		}
	}
	(void)fts_close(fts);
}

/* print compression statistics, and the new name (if there is one!) */
static void
print_verbage(file, newfile, usize, gsize)
	char *file, *newfile;
	ssize_t usize, gsize;
{
	float percent = 100.0 - (100.0 * gsize / usize);

	fprintf(stderr, "%s:%s  %4.1f%%", file,
	    strlen(file) < 7 ? "\t\t" : "\t", percent);
	if (newfile)
		fprintf(stderr, " -- replaced with %s", newfile);
	fprintf(stderr, "\n");
	fflush(stderr);
}

/* print test results */
static void
print_test(file, ok)
	char *file;
	int ok;
{

	fprintf(stderr, "%s:%s  %s\n", file,
	    strlen(file) < 7 ? "\t\t" : "\t", ok ? "OK" : "NOT OK");
	fflush(stderr);
}

/* display the usage of NetBSD gzip */
static void
usage()
{

	fprintf(stderr,
	    "Usage: %s [-cdfhnqrtVv123456789] [<file> [<file> ...]]\n",
	    __progname);
	fflush(stderr);
	exit(0);
}

/* display the version of NetBSD gzip */
static void
display_version()
{

	printf("%s\n", gzip_version);
	fflush(stdout);
	exit(0);
}
