/*	$NetBSD: makewhatis.c,v 1.21 2002/01/31 22:43:41 tv Exp $	*/

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matthias Scheler.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#if defined(__COPYRIGHT) && !defined(lint)
__COPYRIGHT("@(#) Copyright (c) 1999 The NetBSD Foundation, Inc.\n\
	All rights reserved.\n");
#endif /* not lint */

#if defined(__RCSID) && !defined(lint)
__RCSID("$NetBSD: makewhatis.c,v 1.21 2002/01/31 22:43:41 tv Exp $");
#endif /* not lint */

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <fts.h>
#include <locale.h>
#include <paths.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <zlib.h>

typedef struct manpagestruct manpage;
struct manpagestruct {
	manpage *mp_left,*mp_right;
	ino_t	 mp_inode;
	char	 mp_name[1];
};

typedef struct whatisstruct whatis;
struct whatisstruct {
	whatis	*wi_left,*wi_right;
	char	*wi_data;
};

int	 main(int, char **);
char	*findwhitespace(char *);
char	*strmove(char *,char *);
char	*GetS(gzFile, char *, size_t);
int	 manpagesection(char *);
char	*createsectionstring(char *);
void	 addmanpage(manpage **, ino_t, char *);
void	 addwhatis(whatis **, char *);
char	*replacestring(char *, char *, char *);
void	 catpreprocess(char *);
char	*parsecatpage(gzFile *);
int	 manpreprocess(char *);
char	*nroff(gzFile *);
char	*parsemanpage(gzFile *, int);
char	*getwhatisdata(char *);
void	 processmanpages(manpage **,whatis **);
void	 dumpwhatis(FILE *, whatis *);
void	*emalloc(size_t);
char	*estrdup(const char *);

char *default_manpath[] = {
	"/usr/share/man",
	NULL
};

char sectionext[] = "0123456789ln";
char whatisdb[]	  = "whatis.db";

int
main(int argc, char **argv)
{
	char	**manpath;
	FTS	*fts;
	FTSENT	*fe;
	manpage *source;
	whatis	*dest;
	FILE	*out;

	(void)setlocale(LC_ALL, "");

	manpath = (argc < 2) ? default_manpath : &argv[1];

	if ((fts = fts_open(manpath, FTS_LOGICAL, NULL)) == NULL)
		err(EXIT_FAILURE, "Cannot open `%s'", *manpath);

	source = NULL;
	while ((fe = fts_read(fts)) != NULL) {
		switch (fe->fts_info) {
		case FTS_F:
			if (manpagesection(fe->fts_path) >= 0)
				addmanpage(&source, fe->fts_statp->st_ino,
				    fe->fts_path);
			/*FALLTHROUGH*/
		case FTS_D:
		case FTS_DC:
		case FTS_DEFAULT:
		case FTS_DP:
		case FTS_SLNONE:
			break;
		default:
			errno = fe->fts_errno;
			err(EXIT_FAILURE, "Error reading `%s'", fe->fts_path);
		}
	}

	(void)fts_close(fts);

	dest = NULL;
	processmanpages(&source, &dest);

	if (chdir(manpath[0]) == -1)
		err(EXIT_FAILURE, "Cannot change dir to `%s'", manpath[0]);

	(void)unlink(whatisdb);
	if ((out = fopen(whatisdb, "w")) == NULL)
		err(EXIT_FAILURE, "Cannot open `%s'", whatisdb);

	dumpwhatis(out, dest);
	if (fchmod(fileno(out), S_IRUSR|S_IRGRP|S_IROTH) == -1)
		err(EXIT_FAILURE, "Cannot chmod `%s'", whatisdb);
	if (fclose(out) != 0)
		err(EXIT_FAILURE, "Cannot close `%s'", whatisdb);

	return EXIT_SUCCESS;
}

char *
findwhitespace(char *str)
{
	while (!isspace((unsigned char)*str))
		if (*str++ == '\0') {
			str = NULL;
			break;
		}

	return str;
}

char
*strmove(char *dest,char *src)
{
	return memmove(dest, src, strlen(src) + 1);
}

char *
GetS(gzFile in, char *buffer, size_t length)
{
	char	*ptr;

	if (((ptr = gzgets(in, buffer, (int)length)) != NULL) && (*ptr == '\0'))
		ptr = NULL;

	return ptr;
}

int
manpagesection(char *name)
{
	char	*ptr;

	if ((ptr = strrchr(name, '/')) != NULL)
		ptr++;
	else
		ptr = name;

	while ((ptr = strchr(ptr, '.')) != NULL) {
		int section;

		ptr++;
		section=0;
		while (sectionext[section] != '\0')
			if (sectionext[section] == *ptr)
				return section;
			else
				section++;
	}

	return -1;
}

char *
createsectionstring(char *section_id)
{
	char *section = emalloc(strlen(section_id) + 7);
	section[0] = ' ';
	section[1] = '(';
	(void)strcat(strcpy(&section[2], section_id), ") - ");
	return section;
}

void
addmanpage(manpage **tree,ino_t inode,char *name)
{
	manpage *mp;

	while ((mp = *tree) != NULL) {
		if (mp->mp_inode == inode)
			return;
		tree = inode < mp->mp_inode ? &mp->mp_left : &mp->mp_right;
	}

	mp = emalloc(sizeof(manpage) + strlen(name));
	mp->mp_left = NULL;
	mp->mp_right = NULL;
	mp->mp_inode = inode;
	(void)strcpy(mp->mp_name, name);
	*tree = mp;
}

void
addwhatis(whatis **tree, char *data)
{
	whatis *wi;
	int result;

	while (isspace((unsigned char)*data))
		data++;

	if (*data == '/') {
		char *ptr;

		ptr = ++data;
		while ((*ptr != '\0') && !isspace((unsigned char)*ptr))
			if (*ptr++ == '/')
				data = ptr;
	}

	while ((wi = *tree) != NULL) {
		result = strcmp(data, wi->wi_data);
		if (result == 0) return;
		tree = result < 0 ? &wi->wi_left : &wi->wi_right;
	}

	wi = emalloc(sizeof(whatis) + strlen(data));

	wi->wi_left = NULL;
	wi->wi_right = NULL;
	wi->wi_data = data;
	*tree = wi;
}

void
catpreprocess(char *from)
{
	char	*to;

	to = from;
	while (isspace((unsigned char)*from)) from++;

	while (*from != '\0')
		if (isspace((unsigned char)*from)) {
			while (isspace((unsigned char)*++from));
			if (*from != '\0')
				*to++ = ' ';
		}
		else if (*(from + 1) == '\10')
			from += 2;
		else
			*to++ = *from++;

	*to = '\0';
}

char *
replacestring(char *string, char *old, char *new)

{
	char	*ptr, *result;
	size_t	 slength, olength, nlength, pos;

	if (new == NULL)
		return estrdup(string);

	ptr = strstr(string, old);
	if (ptr == NULL)
		return estrdup(string);

	slength = strlen(string);
	olength = strlen(old);
	nlength = strlen(new);
	result = emalloc(slength - olength + nlength + 1);

	pos = ptr - string;
	(void)memcpy(result, string, pos);
	(void)memcpy(&result[pos], new, nlength);
	(void)strcpy(&result[pos + nlength], &string[pos + olength]);

	return result;
}

char *
parsecatpage(gzFile *in)
{
	char	 buffer[8192];
	char	*section, *ptr, *last;
	size_t	 size;

	do {
		if (GetS(in, buffer, sizeof(buffer)) == NULL)
			return NULL;
	}
	while (buffer[0] == '\n');

	section = NULL;
	if ((ptr = strchr(buffer, '(')) != NULL) {
		if ((last = strchr(ptr + 1, ')')) !=NULL) {
			size_t	length;

			length = last - ptr + 1;
			section = emalloc(length + 5);
			*section = ' ';
			(void) memcpy(section + 1, ptr, length);
			(void) strcpy(section + 1 + length, " - ");
		}
	}

	for (;;) {
		if (GetS(in, buffer, sizeof(buffer)) == NULL) {
			free(section);
			return NULL;
		}
		catpreprocess(buffer);
		if (strncmp(buffer, "NAME", 4) == 0)
			break;
	}

	ptr = last = buffer;
	size = sizeof(buffer) - 1;
	while ((size > 0) && (GetS(in, ptr, size) != NULL)) {
		int	 length;

		catpreprocess(ptr);

		length = strlen(ptr);
		if (length == 0) {
			*last = '\0';

			ptr = replacestring(buffer, " - ", section);
			free(section);
			return ptr;
		}
		if ((length > 1) && (ptr[length - 1] == '-') &&
		    isalpha(ptr[length - 2]))
			last = &ptr[--length];
		else {
			last = &ptr[length++];
			*last = ' ';
		}

		ptr += length;
		size -= length;
	}

	free(section);

	return NULL;
}

int
manpreprocess(char *line)
{
	char	*from, *to;

	to = from = line;
	while (isspace((unsigned char)*from)) from++;
	if (strncmp(from, ".\\\"", 3) == 0)
		return 1;

	while (*from != '\0')
		if (isspace((unsigned char)*from)) {
			while (isspace((unsigned char)*++from));
			if ((*from != '\0') && (*from != ','))
				*to++ = ' ';
		}
		else if (*from == '\\')
			switch (*++from) {
			case '\0':
			case '-':
				break;
			case 'f':
			case 's':
				from++;
				if ((*from=='+') || (*from=='-'))
					from++;
				while (isdigit(*from))
					from++;
				break;
			default:
				from++;
			}
		else
			if (*from == '"')
				from++;
			else
				*to++ = *from++;

	*to = '\0';

	if (strncasecmp(line, ".Xr", 3) == 0) {
		char	*sect;

		from = line + 3;
		if (isspace((unsigned char)*from))
			from++;

		if ((sect = findwhitespace(from)) != NULL) {
			size_t	length;
			char	*trail;

			*sect++ = '\0';
			if ((trail = findwhitespace(sect)) != NULL)
				*trail++ = '\0';
			length = strlen(from);
			(void) memmove(line, from, length);
			line[length++] = '(';
			to = &line[length];
			length = strlen(sect);
			(void) memmove(to, sect, length);
			if (trail == NULL) {
				(void) strcpy(&to[length], ")");
			} else {
				to += length;
				*to++ = ')';
				length = strlen(trail);
				(void) memmove(to, trail, length + 1);
			}
		}
	}

	return 0;
}

char *
nroff(gzFile *in)
{
	char tempname[MAXPATHLEN], buffer[65536], *data;
	int tempfd, bytes, pipefd[2], status;
	static int devnull = -1;
	pid_t child;

	if (gzrewind(in) < 0)
		err(EXIT_FAILURE, "Cannot rewind pipe");

	if ((devnull < 0) &&
	    ((devnull = open(_PATH_DEVNULL, O_WRONLY, 0)) < 0))
		err(EXIT_FAILURE, "Cannot open `/dev/null'");

	(void)strcpy(tempname, _PATH_TMP "makewhatis.XXXXXX");
	if ((tempfd = mkstemp(tempname)) == -1)
		err(EXIT_FAILURE, "Cannot create temp file");

	while ((bytes = gzread(in, buffer, sizeof(buffer))) > 0)
		if (write(tempfd, buffer, (size_t)bytes) != bytes) {
			bytes = -1;
			break;
		}

	if (bytes < 0) {
		(void)close(tempfd);
		(void)unlink(tempname);
		err(EXIT_FAILURE, "Read from pipe failed");
	}
	if (lseek(tempfd, (off_t)0, SEEK_SET) == (off_t)-1) {
		(void)close(tempfd);
		(void)unlink(tempname);
		err(EXIT_FAILURE, "Cannot rewind temp file");
	}
	if (pipe(pipefd) == -1) {
		(void)close(tempfd);
		(void)unlink(tempname);
		err(EXIT_FAILURE, "Cannot create pipe");
	}

	switch (child = vfork()) {
	case -1:
		(void)close(pipefd[1]);
		(void)close(pipefd[0]);
		(void)close(tempfd);
		(void)unlink(tempname);
		err(EXIT_FAILURE, "Fork failed");
		/* NOTREACHED */
	case 0:
		(void)close(pipefd[0]);
		if (tempfd != STDIN_FILENO) {
			(void)dup2(tempfd, STDIN_FILENO);
			(void)close(tempfd);
		}
		if (pipefd[1] != STDOUT_FILENO) {
			(void)dup2(pipefd[1], STDOUT_FILENO);
			(void)close(pipefd[1]);
		}
		if (devnull != STDERR_FILENO) {
			(void)dup2(devnull, STDERR_FILENO);
			(void)close(devnull);
		}
		(void)execlp("nroff", "nroff", "-S", "-man", NULL);
		_exit(EXIT_FAILURE);
		/*NOTREACHED*/
	default:
		(void)close(pipefd[1]);
		(void)close(tempfd);
		break;
	}

	if ((in = gzdopen(pipefd[0], "r")) == NULL) {
		if (errno == 0)
			errno = ENOMEM;
		(void)close(pipefd[0]);
		(void)kill(child, SIGTERM);
		while (waitpid(child, NULL, 0) != child);
		(void)unlink(tempname);
		err(EXIT_FAILURE, "Cannot read from pipe");
	}

	data = parsecatpage(in);
	while (gzread(in, buffer, sizeof(buffer)) > 0);
	(void)gzclose(in);

	while (waitpid(child, &status, 0) != child);
	if ((data != NULL) &&
	    !(WIFEXITED(status) && (WEXITSTATUS(status) == 0))) {
		free(data);
		errx(EXIT_FAILURE, "nroff exited with %d status",
		    WEXITSTATUS(status));
	}

	(void)unlink(tempname);
	return data;
}

char *
parsemanpage(gzFile *in, int defaultsection)
{
	char	*section, buffer[8192], *ptr;

	section = NULL;
	do {
		if (GetS(in, buffer, sizeof(buffer) - 1) == NULL) {
			free(section);
			return NULL;
		}
		if (manpreprocess(buffer))
			continue;
		if (strncasecmp(buffer, ".Dt", 3) == 0) {
			char	*end;

			ptr = &buffer[3];
			if (isspace((unsigned char)*ptr))
				ptr++;
			if ((ptr = findwhitespace(ptr)) == NULL)
				continue;

			if ((end = findwhitespace(++ptr)) != NULL)
				*end = '\0';

			free(section);
			section = createsectionstring(ptr);
		}
		else if (strncasecmp(buffer, ".TH", 3) == 0) {
			ptr = &buffer[3];
			while (isspace((unsigned char)*ptr))
				ptr++;
			if ((ptr = findwhitespace(ptr)) != NULL) {
				char *next;

				while (isspace((unsigned char)*ptr))
					ptr++;
				if ((next = findwhitespace(ptr)) != NULL)
					*next = '\0';
				free(section);
				section = createsectionstring(ptr);
			}
		}
		else if (strncasecmp(buffer, ".Ds", 3) == 0) {
			free(section);
			return NULL;
		}
	} while (strncasecmp(buffer, ".Sh NAME", 8) != 0);

	do {
		if (GetS(in, buffer, sizeof(buffer) - 1) == NULL) {
			free(section);
			return NULL;
		}
	} while (manpreprocess(buffer));

	if (strncasecmp(buffer, ".Nm", 3) == 0) {
		size_t	length, offset;

		ptr = &buffer[3];
		while (isspace((unsigned char)*ptr))
			ptr++;

		length = strlen(ptr);
		if ((length > 1) && (ptr[length - 1] == ',') &&
		    isspace((unsigned char)ptr[length - 2])) {
			ptr[--length] = '\0';
			ptr[length - 1] = ',';
		}
		(void) memmove(buffer, ptr, length + 1);

		offset = length + 3;
		ptr = &buffer[offset];
		for (;;) {
			size_t	 more;

			if ((sizeof(buffer) == offset) ||
			    (GetS(in, ptr, sizeof(buffer) - offset)
			       == NULL)) {
				free(section);
				return NULL;
			}
			if (manpreprocess(ptr))
				continue;

			if (strncasecmp(ptr, ".Nm", 3) != 0) break;

			ptr += 3;
			if (isspace((unsigned char)*ptr))
				ptr++;

			buffer[length++] = ' ';
			more = strlen(ptr);
			if ((more > 1) && (ptr[more - 1] == ',') &&
			    isspace((unsigned char)ptr[more - 2])) {
				ptr[--more] = '\0';
				ptr[more - 1] = ',';
			}

			(void) memmove(&buffer[length], ptr, more + 1);
			length += more;
			offset = length + 3;

			ptr = &buffer[offset];
		}

		if (strncasecmp(ptr, ".Nd", 3) == 0) {
			(void) strcpy(&buffer[length], " -");

			while (strncasecmp(ptr, ".Sh", 3) != 0) {
				int	 more;

				if (*ptr == '.') {
					char	*space;

					if (strncasecmp(ptr, ".Nd", 3) != 0) {
						free(section);
						return NULL;
					}
					space = findwhitespace(ptr);
					if (space == NULL)
						ptr = "";
					else {
						space++;
						(void) strmove(ptr, space);
					}
				}

				if (*ptr != '\0') {
					buffer[offset - 1] = ' ';
					more = strlen(ptr) + 1;
					offset += more;
				}
				ptr = &buffer[offset];
				if ((sizeof(buffer) == offset) ||
				    (GetS(in, ptr, sizeof(buffer) - offset)
					== NULL)) {
					free(section);
					return NULL;
				}
				if (manpreprocess(ptr))
					*ptr = '\0';
			}
		}
	}
	else {
		int	 offset;

		if (*buffer == '.') {
			char	*space;

			if ((space = findwhitespace(&buffer[1])) == NULL) {
				free(section);
				return NULL;
			}
			space++;
			(void) strmove(buffer, space);
		}

		offset = strlen(buffer) + 1;
		for (;;) {
			int	 more;

			ptr = &buffer[offset];
			if ((sizeof(buffer) == offset) ||
			    (GetS(in, ptr, sizeof(buffer) - offset)
				== NULL)) {
				free(section);
				return NULL;
			}
			if (manpreprocess(ptr) || (*ptr == '\0'))
				continue;

			if ((strncasecmp(ptr, ".Sh", 3) == 0) ||
			    (strncasecmp(ptr, ".Ss", 3) == 0))
				break;

			if (*ptr == '.') {
				char	*space;

				if ((space = findwhitespace(ptr)) == NULL) {
					continue;
				}

				space++;
				(void) memmove(ptr, space, strlen(space) + 1);
			}

			buffer[offset - 1] = ' ';
			more = strlen(ptr);
			if ((more > 1) && (ptr[more - 1] == ',') &&
			    isspace((unsigned char)ptr[more - 2])) {
				ptr[more - 1] = '\0';
				ptr[more - 2] = ',';
			}
			else more++;
			offset += more;
		}
	}

	if (section == NULL) {
		char sectionbuffer[24];

		(void) sprintf(sectionbuffer, " (%c) - ",
			sectionext[defaultsection]);
		ptr = replacestring(buffer, " - ", sectionbuffer);
	}
	else {
		ptr = replacestring(buffer, " - ", section);
		free(section);
	}
	return ptr;
}

char *
getwhatisdata(char *name)
{
	gzFile	*in;
	char	*data;
	int	 section;

	if ((in = gzopen(name, "r")) == NULL) {
		if (errno == 0)
			errno = ENOMEM;
		err(EXIT_FAILURE, "Cannot open `%s'", name);
		/* NOTREACHED */
	}

	section = manpagesection(name);
	if (section == 0)
		data = parsecatpage(in);
	else {
		data = parsemanpage(in, section);
		if (data == NULL)
			data = nroff(in);
	}

	(void) gzclose(in);
	return data;
}

void
processmanpages(manpage **source, whatis **dest)
{
	manpage *mp;

	mp = *source;
	*source = NULL;

	while (mp != NULL) {
		manpage *obsolete;
		char *data;

		if (mp->mp_left != NULL)
			processmanpages(&mp->mp_left,dest);

		if ((data = getwhatisdata(mp->mp_name)) != NULL)
			addwhatis(dest,data);

		obsolete = mp;
		mp = mp->mp_right;
		free(obsolete);
	}
}

void
dumpwhatis(FILE *out, whatis *tree)
{
	while (tree != NULL) {
		if (tree->wi_left)
			dumpwhatis(out, tree->wi_left);

		if ((fputs(tree->wi_data, out) == EOF) ||
		    (fputc('\n', out) == EOF))
			err(EXIT_FAILURE, "Write failed");

		tree = tree->wi_right;
	}
}

void *
emalloc(size_t len)
{
	void *ptr;
	if ((ptr = malloc(len)) == NULL)
		err(EXIT_FAILURE, "malloc %lu failed", (unsigned long)len);
	return ptr;
}

char *
estrdup(const char *str)
{
	char *ptr;
	if ((ptr = strdup(str)) == NULL)
		err(EXIT_FAILURE, "strdup failed");
	return ptr;
}
