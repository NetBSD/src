/*	$NetBSD: undist.c,v 1.1 2026/06/11 08:29:49 rumble Exp $	*/

/*
 * Copyright (c) 2006, 2007, 2025 Stephen M. Rumble <rumble@ephemeral.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * This program extracts files from SGI distributions used with the 'inst'
 * program. SGI uses manifests in files suffixed with '.idb' to list
 * the archive contents, as well as various other tidbits such as commands
 * to run after extraction.
 *
 * The formats are rather simple. The .idb plain text files reference
 * objects in other data files. These data files contain a 13-byte header,
 * which appears to be an ID, followed by data entries. Each entry contains
 * a 2-byte big endian string length, followed by a string (which matches
 * part of the .idb file content), and then the data. Data size is
 * determined by the .idb file.
 *
 * Data contents are either compressed with a Lempel-Ziv algorithm
 * (compress(1)), or are uncompressed as indicated by the size() and
 * cmpsize() parameters in the .idb file.
 * 
 * Sometime after IRIX 6.2 an off() parameter was added to the .idb files,
 * making it absolutely trivial to extract files. With earlier versions,
 * however, the offset must be computed. It appears the the file order
 * always follows the .idb file, but I am unsure that this is always the
 * case (though it would be needlessly complicated if it were not). In any
 * event, this code goes through some pains by not assuming proper order.
 */

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>

#include <sys/stat.h>
#include <sys/types.h>

#include "util.h"

#define WS		" \f\n\r\t\v"
#define BUFFER_SIZE	32768
#define MAX_IDB_LINE	BUFFER_SIZE

static int aflag;			/* permit extraction to absolute path */
static int lflag;			/* list contents, do not extract */
static int oflag;			/* ignore idb 'off' & build ourselves */
static int pflag;			/* permit relative paths (e.g. '../') */
static int sflag;			/* skip extract errors and continue */
static int tflag;			/* use idb's temporary directory */
static int vflag;			/* verify integrity, do not extract */
static int xflag;			/* extract archive */
static char *wantmach;			/* -m: only matching mach fields */
static char *fileprefix;		/* -P: only matching file prefixes */
static unsigned int totalfiles;
static unsigned int totalbytes;
static unsigned int totalcompressed;
static char idbfile[FILENAME_MAX];

struct offset_table_entry {
	char *type;
	mode_t mode;
	char *owner;
	char *group;
	char *dstfile;
	char *tmpfile;
	char *package;
	char *mach;
	int   sum;
	int   size;
	int   cmpsize;
	int   off;
	int   flags;
};

#define F_NORQS		0x00000001
#define F_NEEDRQS	0x00000002
#define F_NOSTRIP	0x00000004
#define F_STRIPDSO	0x00000008
#define F_NOHIST	0x00000010
#define F_DELHIST	0x00000020
#define F_NOSHARE	0x00000040

static struct offset_table_entry **offset_table;
static int offset_table_length;

#define INVALID_OFFSET	(-1)

#ifndef PATH_MAX
#define PATH_MAX	1024
#endif

static bool
streql(const char *str1, const char *str2)
{
	if (str1 == NULL || str2 == NULL)
		return (false);

	return (strcmp(str1, str2) == 0);
}

static char *
xstrdup(const char *str)
{
	char *cpy;
	
	if (str == NULL)
		return (NULL);

	cpy = strdup(str);
	if (cpy == NULL) {
		fprintf(stderr, "error: strdup failed: %s\n", strerror(errno));
		exit(1);
	}

	return (cpy);
}

static void *
xmalloc(int bytes)
{
	void *ptr;
	
	ptr = malloc(bytes);
	if (ptr == NULL) {
		fprintf(stderr, "error: malloc failed: %s\n", strerror(errno));
		exit(1);
	}

	return (ptr);
}

static void *
xrealloc(void *oldptr, int bytes)
{
	void *ptr;
	
	ptr = realloc(oldptr, bytes);
	if (ptr == NULL) {
		fprintf(stderr, "error: realloc failed: %s\n", strerror(errno));
		exit(1);
	}

	return (ptr);
}

static FILE *
xfopen(const char *file, const char *mode)
{
	FILE *infp;

	infp = fopen(file, mode);
	if (infp == NULL) {
		fprintf(stderr, "error: failed to fopen file %s: %s\n",
		    file, strerror(errno));
		exit(1);
	}

	return (infp);
}

static void
xfseek(FILE *fp, long int offset, int whence, const char *filename)
{
	if (fseek(fp, offset, whence) == -1) {
		if (filename != NULL)
			fprintf(stderr, "error: failed to fseek source file "
			    "%s: %s\n", filename, strerror(errno));
		else
			fprintf(stderr, "error: failed to fseek: %s\n",
			    strerror(errno));
		exit(1);
	}
}

static bool
xfeof(FILE *fp)
{
	char foo;
	bool ret;

	ret = (fread(&foo, 1, 1, fp) != 1);
	xfseek(fp, -1, SEEK_CUR, NULL);

	return (ret);
}

static char *
extract_parameter(const char *name, const char *str)
{
	static char buf[512];
	int oparen, cparen;
	const char *start, *end;

	/* ensure we don't confuse 'size(..)' with 'cmpsize(...)' */
	start = strstr(str, name);
	while (start != NULL && start != str && !isspace((int)*(start - 1))) 
		start = strstr(str, name);

	if (start != NULL) {
		start += strlen(name);
		if (*start == '(') {
			end = ++start;
			oparen = 1;
			cparen = 0;
			while (*end != '\0') {
				if (*end == '(')
					oparen++;
				else if (*end == ')')
					cparen++;

				if (oparen == cparen)
					break;

				end++;
			}
			memcpy(buf, start, end - start);
			buf[end - start] = '\0';
			return (buf);
		}
	}

	return (NULL);
}

static unsigned long int
extract_parameter_int(const char *name, const char *str, const int line)
{
	char *n;

	n = extract_parameter(name, str);
	if (n == NULL) {
		fprintf(stderr, "%s: malformed idb file (line %d)\n",
		    (sflag) ? "warning" : "error", line);
		if (!sflag)
			exit(1);
	}

	return (strtoul(n, NULL, 10));
}

/*
 * Given a package name, return the full filename including path.
 */
static const char *
filename_from_package(const char *package)
{
	static char buf[PATH_MAX + 1];

	char file[PATH_MAX + 1];
	struct stat sb;
	char *dot;

	strlcpy(file, package, sizeof(file));
	dot = strchr(file, '.');
	if (dot != NULL) {
		dot = strchr(dot + 1, '.');
		if (dot != NULL)
			*dot = '\0';
	}

	strlcpy(buf, dirname(idbfile), sizeof(buf));
	strlcat(buf, "/", sizeof(buf));
	strlcat(buf, file, sizeof(buf));

	/*
	 * This won't always work. E.g., IRIX 6.5.25m has a "_6525m" suffix,
	 * though it's not reflected in the package name. So, fall back on the
	 * idb file name and munge it to see if we can get what we want.
	 */
	if (stat(buf, &sb) == -1) {
		char *suffix;

		suffix = strrchr(buf, '.');
		if (suffix != NULL) {
			strlcpy(file, suffix, sizeof(file));
			dot = strrchr(idbfile, '.');
			if (dot != NULL) {
				strlcpy(buf, idbfile, sizeof(buf));
				dot = strrchr(buf, '.');
				assert(dot != NULL);
				*dot = '\0';
				strlcat(buf, file, sizeof(buf));
			}
		}
	}

	return (buf);
}


static void
grow_offset_table(int length)
{
	if (offset_table == NULL) {
		offset_table = xmalloc(sizeof(struct offset_table *) * length);
		memset(offset_table, 0, sizeof(struct offset_table *) * length);
	} else {
		offset_table = xrealloc(offset_table,
		    sizeof(struct offset_table *) * length);
		memset(&offset_table[offset_table_length], 0,
		    sizeof(struct offset_table *) *
		    (length - offset_table_length));
	}

	offset_table_length = length;
}

static struct offset_table_entry *
new_offset_table_entry(char *type, mode_t mode, char *owner, char *group,
    char *dstfile, char *tmpfile, char *package, char *mach, int sum, int size,
    int cmpsize, int off, int flags)
{
	struct offset_table_entry *ote;

	ote = xmalloc(sizeof(*ote));
	ote->type = xstrdup(type);
	ote->mode = mode;
	ote->owner = xstrdup(owner);
	ote->group = xstrdup(group);
	ote->dstfile = xstrdup(dstfile);
	ote->tmpfile = xstrdup(tmpfile);
	ote->package = xstrdup(package);
	ote->mach = xstrdup(mach);
	ote->sum = sum;
	ote->size = size;
	ote->cmpsize = cmpsize;
	ote->off = off;
	ote->flags = flags;

	return (ote);
}

/*
 * Helper for discover_file_offsets(). Get the next offset_table entry
 * for 'dstfile'.
 */
static struct offset_table_entry *
get_next_from_dstfile(const char *dstfile, size_t dstlen)
{
	static int lastidx;
	static char buf[1024];
	int i;

	if (!streql(buf, dstfile)) {
		lastidx = -1;
		strlcpy(buf, dstfile, dstlen);
	}

	for (i = lastidx + 1;
	    i < offset_table_length && offset_table[i] != NULL; i++) {
		lastidx = i;
		if (streql(dstfile, offset_table[i]->dstfile))
			return (offset_table[i]);
	}

	return (NULL);
}

static int
get_filename_length(FILE *fp, const char *fname)
{
	unsigned short filename_len;
	if (fread(&filename_len, sizeof(filename_len), 1, fp) != 1) {
		fprintf(stderr, "error: failed to read from file %s: "
		    "%s\n", fname, strerror(errno));
		exit(1);
	}
	return ntohs(filename_len);
}

static void
get_filename(FILE *fp, char *buf, int len)
{
	if (fread(buf, len, 1, fp) != 1) {
		fprintf(stderr, "error: failed to read: %s\n", strerror(errno));
		exit(1);
	}
	buf[len] = '\0';
}

/*
 * Determine if the data at the current position in fp matches the size
 * information in 'ent'. We do this heuristically based on the following
 * rules:
 *	1) If the file is compressed (ent->cmpsize != 0), then the
 *	   EOF or a new valid string length exists ent->cmpsize bytes
 *	   offset from the current position.
 *
 *	2) If the file is not compressed (ent->cmpsize == 0), then the
 *	   EOF or a new valid string length exists ent->size bytes
 *	   offset from the current position.
 *
 * If 1) or 2) is true and we're not in the EOF case, the next string
 * must exist in our offset_table, otherwise something is corrupt or
 * we've read garbage.
 */
static bool
is_entry(FILE *fp, struct offset_table_entry *ent)
{
	int i, len;
	bool ret;
	long int oldoff;
	char buf[PATH_MAX + 1];
	unsigned short filename_len;

	ret = false;
	oldoff = ftell(fp);
	len = (ent->cmpsize == 0) ? ent->size : ent->cmpsize;

	fseek(fp, len, SEEK_CUR);
	if (xfeof(fp)) {
		fseek(fp, -1, SEEK_CUR);
		if (!xfeof(fp)) {
			ret = true;
			goto leave;
		}

		goto leave;
	}

	filename_len = get_filename_length(fp, NULL);
	if (filename_len > PATH_MAX)
		goto leave;
			
	get_filename(fp, buf, filename_len);
	for (i = 0; i < offset_table_length && offset_table[i] != NULL; i++)
		if (streql(buf, offset_table[i]->dstfile))
			ret = true;
leave:
	xfseek(fp, oldoff, SEEK_SET, NULL);
	return (ret);
}

/*
 * 'fp' points to the first length field of a data file. Run through the
 * the and try to match offset_table entries to it, filling in the
 * offsets as we go along.
 */
static void
discover_all(FILE *fp, const char *fname)
{
	int off;
	char buf[PATH_MAX + 1];
	unsigned short filename_len;
	struct offset_table_entry *nextent;

	while (!xfeof(fp)) {
		off = ftell(fp);

		filename_len = get_filename_length(fp, fname);
		if (filename_len > PATH_MAX) {
			fprintf(stderr, "error: absurdly long embedded string "
			    "(%d bytes) in %s\n", filename_len, fname);
			exit(1);
		}

		get_filename(fp, buf, filename_len);

		/*
		 * 'buf' may match several filenames (e.g. if 'mach')
		 * is used. We could probably assume that the first matching
		 * entry in offset_table (and thus in the idb file) belongs,
		 * but we'll be a bit more careful and ensure that things
		 * line up in 'is_entry()'.
		 *
		 * XXX - duplicate files with same size. should we do a
		 *	 cksum too?
		 */
		while (true) {
			nextent = get_next_from_dstfile(buf, sizeof(buf));
			if (nextent == NULL)
				return;

			if (nextent->off == INVALID_OFFSET &&
			    is_entry(fp, nextent)) {
				nextent->off = off;
				fseek(fp, (nextent->cmpsize == 0) ?
				    nextent->size : nextent->cmpsize, SEEK_CUR);
				break;
			}
		}
	}
}

/*
 * After our offset_table has been built, go through it and try to
 * determine the offset for all files.
 *
 * The algorithm does not assume that SGI built the idb file in order,
 * though I suspect they always do.
 *
 * The first 13 bytes of each data file appear to be a special
 * identification tag, for instance: "im001V530P00\0". Immediately
 * following is a 16-bit big endian string length, the string itself,
 * and then the data. The string is the same as the 'dstfile' entry of
 * the idb file, and is not nul-terminated.
 */
static void
discover_file_offsets(void)
{
	int i;
	FILE *fp;
	const char *fname;
	struct offset_table_entry *ent;

	for (i = 0; i < offset_table_length && offset_table[i] != NULL; i++) {
		ent = offset_table[i];

		if (ent->off != INVALID_OFFSET)
			continue;

		fname = filename_from_package(ent->package);
		fp = xfopen(fname, "r");
		xfseek(fp, 13, SEEK_SET, NULL);
		discover_all(fp, fname);
		fclose(fp);
	}

	for (i = 0; i < offset_table_length && offset_table[i] != NULL; i++) {
		if (offset_table[i]->off == INVALID_OFFSET) {
			fprintf(stderr, "%s: failed to resolve offset for "
			    "file %s\n", (sflag) ? "warning" : "error",
			    offset_table[i]->dstfile);
			if (!sflag)
				exit(1);
		}
	}
}

static int
cntchr(const char *str, char chr)
{
	int cnt;

	cnt = 0;
	while (*str != '\0')
		if (*str++ == chr)
			cnt++;

	return (cnt);
}

/*
 * Tokenise based on whitespace, with one exception: if we have a
 * parameter, do not stop on whitespace, but only on a closing parenthesis.
 *
 * Since parameters themselves can take parentheses, we need to only stop
 * on outter ones.
 */
static char *
idbtok(char *str, bool *isparameter)
{
	static char *next;
	char tmp, *start, *end;
	int i, oparens, cparens;

	if (next == NULL && str == NULL)
		return (NULL);

	if (str != NULL)
		next = str;

	if (*next == '\0')
		return (NULL);

	*isparameter = false;

	start = next + strspn(next, WS);
	end = start + strcspn(start, WS);
	if (end > start) {
		tmp = *end;

		if (tmp == '\0') {
			next = NULL;
			oparens = cntchr(start, '(');
			cparens = cntchr(start, ')');
		} else {
			*end = '\0';

			oparens = cntchr(start, '(');
			cparens = cntchr(start, ')');
			if (oparens != cparens) {
				*end = tmp;

				oparens = cparens = 0;
				for (i = 1; start[i] != '\0'; i++) {
					if (start[i] == '(')
						oparens++;
					else if (start[i] == ')')
						cparens++;
					else
						continue;

					if (oparens != 0 && oparens == cparens){
						end = start + i + 1;
						break;
					}
				}

				if (*end == '\0')
					next = NULL;
				else
					*end = '\0';
			}
		}

		if (oparens == cparens && oparens != 0 && *start != '(')
			*isparameter = true;
	} else
		next = NULL;

	if (next != NULL)
		next = end + 1;

	return (start);
}

/*
 * Construct a table of file offsets for earlier versions of IRIX, which
 * do not include an 'off()' parameter in the idb file.
 */
static void
build_offset_table(FILE *fp)
{
	mode_t mode;
	char *buf;
	bool isparam;
	char parambuf[1024];
	int line, flags, tblidx, off;
	int size, sum, cmpsize, state;
	char *type, *smode, *owner, *token, *group;
	char *dstfile, *tmpfile, *package, *mach, *p;

	tblidx = 0;
	grow_offset_table(32);
	buf = xmalloc(MAX_IDB_LINE);

	/*
	 * Most idb archives follow a strict, fixed order. E.g.:
	 *   "f 0755 root sys /dst /tmp foo.sw.bar sum(0) size(0) cmpsize(0)"
	 *
	 * Unfortunately, it appears that in some idb files the 'package'
	 * comes after the parameters, making things more complex than they
	 * should be. E.g.:
	 *   "f 0755 root sys /dst /tmp sum(0) size(0) cmpsize(0) foo.sw.bar"
	 *
	 * We'll be extra robust here, allowing parameters (distinguished
	 * by having '(' and ')' in their strings) to appear anywhere.
	 */
	for (line = 1; fgets(buf, MAX_IDB_LINE, fp) != NULL; line++) {
		/* immediately skip comments */
		if (buf[0] == '#')
			continue;

		*parambuf = '\0';
		flags = state = 0;
		tmpfile = package = NULL;
		type = smode = owner = group = dstfile = NULL;

		for (p = buf; (token = idbtok(p, &isparam)) != NULL; p = NULL) {
			if (isparam) {
				strlcat(parambuf, " ", sizeof(parambuf));
				strlcat(parambuf, token, sizeof(parambuf));
			} else if (streql(token, "norqs")) {
				flags |= F_NORQS;
			} else if (streql(token, "needrqs")) {
				flags |= F_NEEDRQS;
			} else if (streql(token, "nostrip")) {
				flags |= F_NOSTRIP;
			} else if (streql(token, "stripdso")) {
				flags |= F_STRIPDSO;
			} else if (streql(token, "nohist")) {
				flags |= F_NOHIST;
			} else if (streql(token, "delhist")) {
				flags |= F_DELHIST;
			} else if (streql(token, "noshare")) {
				flags |= F_NOSHARE;
			} else {
				switch (state++) {
				case 0:	type = token;		break;
				case 1: smode = token;		break;
				case 2: owner = token;		break;
				case 3: group = token;		break;
				case 4: dstfile = token;	break;
				case 5: tmpfile = token;	break;
				case 6: package = token;	break;
				default:
					fprintf(stderr, "warning: unknown flag "
					    "\"%s\" (line %d)\n", token, line);
				}
			}
		}

		if (state < 7) {
			fprintf(stderr, "%s: malformed idb file (line %d)\n",
			    (sflag) ? "warning" : "error", line);
			if (sflag)
				continue;
			else
				exit(1);
		}

		/*
		 * Skip block/char device, directory, and symbolic link
		 * entries. Be very liberal otherwise, and do not bother to
		 * proof their format.
		 */
		if (streql(type, "b") || streql(type, "c") ||
		    streql(type, "d") || streql(type, "l"))
			continue;

		if (!streql(type, "f") || smode == NULL ||
		    owner == NULL || group == NULL || dstfile == NULL ||
		    tmpfile == NULL || package == NULL || parambuf[0] == '\0') {
			fprintf(stderr, "%s: malformed idb file (line %d)\n",
			    (sflag) ? "warning" : "error", line);
			if (sflag)
				continue;
			else
				exit(1);
		}

		mode = strtoul(smode, NULL, 8);

		sum	= extract_parameter_int("sum", parambuf, line);
		size	= extract_parameter_int("size", parambuf, line);
		cmpsize	= extract_parameter_int("cmpsize", parambuf, line);

		if (oflag || extract_parameter("off", parambuf) == NULL)
			off = INVALID_OFFSET;	/* to be discovered */
		else
			off = extract_parameter_int("off", parambuf, line);

		if (tblidx == offset_table_length)
			grow_offset_table(offset_table_length * 2);

		mach = extract_parameter("mach", parambuf);

		offset_table[tblidx++] = new_offset_table_entry(type, mode,
		    owner, group, dstfile, tmpfile, package, mach, sum, size,
		    cmpsize, off, flags);
	}

	free(buf);
}

/*
 * Open a file, creating all directories along the way if we have to.
 */
static int
createpath(const char *file, int flags, mode_t filemode, mode_t dirmode)
{
	int fd;
	char *start, *slash;

	fd = open(file, flags, filemode);
	if (fd != -1)
		return (fd);

	switch (errno) {
	case EACCES:
		if (unlink(file) == 0) {
			fd = open(file, flags, filemode);
			if (fd != -1)
				return (fd);
		}
		return (-1);
		break;

	case ENOENT:
		break;

	default:
		return (-1);
	}

	start = (char *)file;
	while (true) {
		slash = strchr(start, '/');
		if (slash == NULL)
			break;

		*slash = '\0';
		if (mkdir(file, dirmode) != 0 && errno != EEXIST)
			return (-1);
		*slash = '/';
		start = slash + 1;
	}

	fd = open(file, flags, filemode);

	return (fd);
}

static bool
fcopy(FILE *infp, FILE *outfp, int extractbytes, int *cksum)
{
	uint32_t extract;
	u_char buf[BUFFER_SIZE];

	*cksum = 0;
	while (extractbytes > 0) {
		if (sizeof(buf) > extractbytes)
			extract = extractbytes;
		else
			extract = sizeof(buf);

		if (fread(buf, extract, 1, infp) != 1)
			return (false);

		if (outfp != NULL) {
			if (fwrite(buf, extract, 1, outfp) != 1)
				return (false);
		}

		extractbytes -= extract;
		*cksum = sum1(*cksum, buf, extract);
	}

	return (true);
}

/*
 * NB: only returns on success.
 *     modifies globals 'total{files,bytes}' and 'totalcompressed'.
 */
static void
extract_file(struct offset_table_entry *ent)
{
	char *s;
	int nsum;
	int outfd;
	bool success;
	FILE *infp, *outfp;
	const char *src, *dst;

	dst = tflag ? ent->tmpfile : ent->dstfile;
	src = filename_from_package(ent->package);
	infp = xfopen(src, "r");

	/* do not allow absolute paths unless explicitly permitted */
	if (!aflag) {
		while (*dst == '/')
			dst++;
	}

	/* chew relative paths unless explicitly permitted */
	if (!pflag) {
		while ((s = strstr(dst, "../")) != NULL)
			memcpy(s, "///", 3);
	}

	if (vflag) {
		outfp = NULL;
	} else {
		outfd = createpath(dst, O_WRONLY | O_CREAT, ent->mode, 0755);
		if (outfd == -1) {
			fprintf(stderr, "error: failed to open destination "
			    "file %s: %s\n", dst, strerror(errno));
			exit(1);
		}

		outfp = fdopen(outfd, "w");
		if (outfp == NULL) {
			fprintf(stderr, "error: fdopen failed: %s\n",
			    strerror(errno));
			exit(1);
		}
	}

	xfseek(infp, ent->off + 2 + strlen(ent->dstfile), SEEK_SET, src);

	errno = 0;
	nsum = 0;
	if (ent->cmpsize == 0)
		success = fcopy(infp, outfp, ent->size, &nsum);
	else {
		success = (decompress(infp, outfp,
		    ent->cmpsize, &nsum) == ent->size);
		totalcompressed++;
	}

	if (!success) {
		if (errno == 0)
			fprintf(stderr, "%s: %s and/or %s corrupt!\n",
			    (sflag) ? "warning" : "error", idbfile, src);
		else
			fprintf(stderr, "%s: failed to extract %s: %s\n",
			    (sflag) ? "warning" : "error", dst,
			    strerror(errno));

		if (!sflag)
			exit(1);
	}

	if (nsum != ent->sum) {
		fprintf(stderr, "%s: checksum failed for file %s%s%s%s\n",
		    (sflag) ? "warning" : "error", dst,
		    (ent->mach != NULL) ? " [mach(" : "",
		    (ent->mach != NULL) ? ent->mach : "",
		    (ent->mach != NULL) ? ")]" : "");
		    
		if (!sflag)
			exit(1);
		success = false;
	}

	fclose(infp);
	if (outfp != NULL)
		fclose(outfp);

	totalfiles++;
	totalbytes += ent->size;

	if (success) {
		if (ent->mach == NULL) {
			printf("  %s: %s\n", (vflag) ? "verified" : "extracted",
			    dst);
		} else {
			printf("  %s: %s [mach(%s)]\n",
			    (vflag) ? "verified" : "extracted", dst, ent->mach);
		}
	}
}

static void
extract_files(void)
{
	int i;

	for (i = 0; i < offset_table_length && offset_table[i] != NULL; i++) {
		struct offset_table_entry *ent = offset_table[i];

		if (wantmach != NULL && ent->mach != NULL &&
		    !streql(wantmach, ent->mach))
			continue;

		if (fileprefix != NULL &&
		    strncmp(ent->dstfile, fileprefix, strlen(fileprefix))) {
			continue;
		}

		if (ent->off != INVALID_OFFSET)
			extract_file(ent);
	}

	printf("%d bytes in %d files (%d compressed, %d uncompressed)\n",
	    totalbytes, totalfiles, totalcompressed,
	    totalfiles - totalcompressed);
}

static void
list_files(void)
{
	int i;

	for (i = 0; i < offset_table_length && offset_table[i] != NULL; i++) {
		if (wantmach != NULL && offset_table[i]->mach != NULL &&
		    !streql(wantmach, offset_table[i]->mach))
			continue;

		if (offset_table[i]->off != INVALID_OFFSET) {
			if (offset_table[i]->mach == NULL) {
				printf("  %s\n", offset_table[i]->dstfile);
			} else {
				printf("  %s [mach(%s)]\n",
				    offset_table[i]->dstfile,
				    offset_table[i]->mach);
			}

			totalfiles++;
			totalbytes += offset_table[i]->size;
			if (offset_table[i]->cmpsize != 0)
				totalcompressed++;
		}
	}

	printf("%d bytes in %d files (%d compressed, %d uncompressed)\n",
	    totalbytes, totalfiles, totalcompressed,
	    totalfiles - totalcompressed);
}

static void
usage(void)
{
	fprintf(stderr, "usage: %s [-l|-v|-x] [-aopst] [-m machtype] "
	    "[-f prefix] dist.idb\n", getprogname());
	exit(1);
}

int
main(int argc, char **argv)
{
	int ch;
	FILE *idb;
	extern int optind;
	extern char *optarg;

	while ((ch = getopt(argc, argv, "alm:opP:stvx")) != -1) {
		switch (ch) {
		case 'a':
			aflag = 1;
			break;

		case 'l':
			lflag = 1;
			break;

		case 'm':
			wantmach = optarg;
			break;

		case 'o':
			oflag = 1;
			break;

		case 'p':
			pflag = 1;
			break;

		case 'P':
			fileprefix = optarg;
			break;

		case 's':
			sflag = 1;
			break;

		case 't':
			tflag = 1;
			break;

		case 'v':
			vflag = 1;
			break;

		case 'x':
			xflag = 1;
			break;

		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	if (argc == 0)
		usage();

	switch (lflag + vflag + xflag) {
	case 0:
		fprintf(stderr, "error: no -l, -v or -x operation specified\n");
		usage();

	case 1:
		break;

	default:
		fprintf(stderr, "error: 'l', 'v' and 'x' flags may not be used "
		    "in conjunction\n");
		usage();
	}

	strlcpy(idbfile, argv[0], sizeof(idbfile));
	idb = xfopen(idbfile, "r");
	build_offset_table(idb);
	fclose(idb);

	discover_file_offsets();

	if (lflag) {
		list_files();
	} else {
		extract_files();
	}

	return (0);
}
