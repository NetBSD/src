/* $NetBSD: conffile.c,v 1.4.4.2 2008/01/22 19:06:03 bouyer Exp $ */

/*
 * Copyright © 2006 Alistair Crooks.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>

#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "conffile.h"

/* start of split routines */

/* open a file */
int
conffile_open(conffile_t *sp, const char *f, const char *mode, const char *sep, const char *comment)
{
	(void) memset(sp, 0x0, sizeof(*sp));
	if ((sp->fp = fopen(f, mode)) == NULL) {
		(void) fprintf(stderr, "can't open `%s' `%s' (%s)\n", f, mode, strerror(errno));
		return 0;
	}
	(void) strlcpy(sp->name, f, sizeof(sp->name));
	sp->sep = sep;
	sp->comment = comment;
	sp->readonly = (strcmp(mode, "r") == 0);
	return 1;
}

/* close a file */
void
conffile_close(conffile_t *sp)
{
	(void) fclose(sp->fp);
}

/* read the next line from the file */
static char *
read_line(conffile_t *sp, ent_t *ep)
{
	char	*from;

	if (fgets(ep->buf, sizeof(ep->buf), sp->fp) == NULL) {
		return NULL;
	}
	sp->lineno += 1;
	for (from = ep->buf ; *from && isspace((unsigned int)*from) ; from++) {
	}
	return from;
}

/* return 1 if this line contains no entry */
static int
iscomment(conffile_t *sp, char *from)
{
	return (*from == 0x0 || *from == '\n' || strchr(sp->comment, *from) != NULL);
}

/* split the entry up into fields */
static int
split_split(conffile_t *sp, ent_t *ep, char *from)
{
	char	*to;
	char	 was;
	int	 sepseen;
	int	 cc;

	for (ep->sv.c = 0 ; *from && *from != '\n' ; ) {
		for (to = from, sepseen = 0 ; *to && *to != '\n' && strchr(sp->sep, *to) == NULL ; to++) {
			if (*to == '\\') {
				if (*(to + 1) == '\n') {
					cc = (int)(to - ep->buf);
					if (fgets(&ep->buf[cc], sizeof(ep->buf) - cc, sp->fp) != NULL) {
						sp->lineno += 1;
					}
				} else {
					sepseen = 1;
					to++;
				}
			}
		}
		ALLOC(char *, ep->sv.v, ep->sv.size, ep->sv.c, 14, 14, "conffile_getent", exit(EXIT_FAILURE));
		ep->sv.v[ep->sv.c++] = from;
		was = *to;
		*to = 0x0;
		if (sepseen) {
			char	*cp;

			for (cp = from ; *cp ; cp++) {
				if (strchr(sp->sep, *cp) != NULL) {
					(void) strcpy(cp - 1, cp);
				}
			}
		}
		if (was == 0x0 || was == '\n') {
			break;
		}
		for (from = to + 1 ; *from && *from != '\n' && strchr(sp->sep, *from) != NULL ; from++) {
		}
	}
	return 1;
}

/* get the next entry */
int
conffile_getent(conffile_t *sp, ent_t *ep)
{
	char	*from;

	for (;;) {
		if ((from = read_line(sp, ep)) == NULL) {
			return 0;
		}
		if (iscomment(sp, from)) {
			continue;
		}
		return split_split(sp, ep, from);
	}
}

/* return the line number */
int
conffile_get_lineno(conffile_t *sp)
{
	return sp->lineno;
}

/* return the name */
char *
conffile_get_name(conffile_t *sp)
{
	return sp->name;
}

/* return the entry based upon the contents of field `f' */
int
conffile_get_by_field(conffile_t *sp, ent_t *ep, int f, char *val)
{
	while (conffile_getent(sp, ep)) {
		if (ep->sv.c > f && strcmp(ep->sv.v[f], val) == 0) {
			return 1;
		}
	}
	return 0;
}

/* check that we wrote `cc' chars of `buf' to `fp' */
static int
safe_write(FILE *fp, char *buf, int cc)
{
	return fwrite(buf, sizeof(char), cc, fp) == cc;
}

#if 0
/* check that we wrote the entry correctly */
static int
safe_write_ent(FILE *fp, conffile_t *sp, ent_t *ep)
{
	char	buf[BUFSIZ];
	int	cc;
	int	i;

	for (cc = i = 0 ; i < ep->sv.c ; i++) {
		cc += snprintf(&buf[cc], sizeof(buf) - cc, "%s%1.1s", ep->sv.v[i], (i == ep->sv.c - 1) ? "\n" : sp->sep);
	}
	return safe_write(fp, buf, cc);
}
#endif

/* report an error and clear up */
static int
report_error(FILE *fp, char *name, const char *fmt, ...)
{
	va_list	vp;

	va_start(vp, fmt);
	(void) vfprintf(stderr, fmt, vp);
	va_end(vp);
	if (fp)
		(void) fclose(fp);
	(void) unlink(name);
	return 0;
}

/* put the new entry (in place of ent[f] == val, if val is non-NULL) */
int
conffile_putent(conffile_t *sp, int f, char *val, char *newent)
{
	ent_t	 e;
	FILE	*fp;
	char	 name[MAXPATHLEN];
	char	*from;
	int	 fd;

	(void) strlcpy(name, "/tmp/split.XXXXXX", sizeof(name));
	if ((fd = mkstemp(name)) < 0) {
		(void) fprintf(stderr, "can't mkstemp `%s' (%s)\n", name, strerror(errno));
		return 0;
	}
	fp = fdopen(fd, "w");
	(void) memset(&e, 0x0, sizeof(e));
	for (;;) {
		if ((from = read_line(sp, &e)) == NULL) {
			break;
		}
		if (iscomment(sp, from)) {
			if (!safe_write(fp, e.buf, strlen(e.buf))) {
				return report_error(fp, name, "Short write 1 to `%s' (%s)\n", name, strerror(errno));
			}
		}
		(void) split_split(sp, &e, from);
		if (val != NULL && f < e.sv.c && strcmp(val, e.sv.v[f]) == 0) {
			/* replace it */
			if (!safe_write(fp, newent, strlen(newent))) {
				return report_error(fp, name, "Short write 2 to `%s' (%s)\n", name, strerror(errno));
			}
		} else {
			if (!safe_write(fp, e.buf, strlen(e.buf))) {
				return report_error(fp, name, "Short write 3 to `%s' (%s)\n", name, strerror(errno));
			}
		}
	}
	if (val == NULL && !safe_write(fp, newent, strlen(newent))) {
		return report_error(fp, name, "Short write 4 to `%s' (%s)\n", name, strerror(errno));
	}
	(void) fclose(fp);
	if (rename(name, sp->name) < 0) {
		return report_error(NULL, name, "can't rename %s to %s (%s)\n", name, sp->name, strerror(errno));
	}
	return 1;
}

/* print the entry on stdout */
void
conffile_printent(ent_t *ep)
{
	int	i;

	for (i = 0 ; i < ep->sv.c ; i++) {
		printf("(%d `%s') ", i, ep->sv.v[i]);
	}
	printf("\n");
}
