/*
 * Copyright (c) 1992, Brian Berliner and Jeff Polk
 * Copyright (c) 1989-1992, Brian Berliner
 * Copyright (c) 2004, Derek R. Price and Ximbiot <http://ximbiot.com>
 * Copyright (c) 1989-2004 The Free Software Foundation <http://gnu.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef SUBR_H
# define SUBR_H

void expand_string (char **, size_t *, size_t);
char *Xreadlink (const char *link, size_t size);
void xrealloc_and_strcat (char **, size_t *, const char *);
int strip_trailing_newlines (char *str);
int pathname_levels (const char *path);
void free_names (int *pargc, char *argv[]);
void line2argv (int *pargc, char ***argv, char *line, char *sepchars);
int numdots (const char *s);
int compare_revnums (const char *, const char *);
char *increment_revnum (const char *);
char *getcaller (void);
char *previous_rev (RCSNode *rcs, const char *rev);
char *gca (const char *rev1, const char *rev2);
void check_numeric (const char *, int, char **);
char *make_message_rcsvalid (const char *message);
int file_has_markers (const struct file_info *);
void get_file (const char *, const char *, const char *,
               char **, size_t *, size_t *);
void resolve_symlink (char **filename);
char *backup_file (const char *file, const char *suffix);
char *shell_escape (char *buf, const char *str);
void sleep_past (time_t desttime);

/* for format_cmdline function - when a list variable is bound to a user string,
 * we need to pass some data through walklist into the callback function.
 * We use this struct.
 */
struct format_cmdline_walklist_closure
{
    const char *format;	/* the format string the user passed us */
    char **buf;		/* *dest = our NUL terminated and possibly too short
			 * destination string
			 */
    size_t *length;	/* *dlen = how many bytes have already been allocated to
			 * *dest.
			 */
    char **d;		/* our pointer into buf where the next char should go */
    char quotes;	/* quotes we are currently between, if any */
#ifdef SUPPORT_OLD_INFO_FMT_STRINGS
    int onearg;
    int firstpass;
    const char *srepos;
#endif /* SUPPORT_OLD_INFO_FMT_STRINGS */
    void *closure;	/* our user defined closure */
};
char *cmdlinequote (char quotes, char *s);
char *cmdlineescape (char quotes, char *s);
#ifdef SUPPORT_OLD_INFO_FMT_STRINGS
char *format_cmdline (bool oldway, const char *srepos, const char *format, ...);
#else /* SUPPORT_OLD_INFO_FMT_STRINGS */
char *format_cmdline (const char *format, ...);
#endif /* SUPPORT_OLD_INFO_FMT_STRINGS */

/* Many, many CVS calls to xstrdup depend on it to return NULL when its
 * argument is NULL.
 */
#define xstrdup Xstrdup
char *Xstrdup (const char *str)
	__attribute__ ((__malloc__));

char *Xasprintf (const char *format, ...)
	__attribute__ ((__malloc__, __format__ (__printf__, 1, 2)));
char *Xasnprintf (char *resultbuf, size_t *lengthp, const char *format, ...)
        __attribute__ ((__malloc__, __format__ (__printf__, 3, 4)));
bool readBool (const char *infopath, const char *option,
	       const char *p, bool *val);

FILE *xfopen (const char *, const char *);
char *xcanonicalize_file_name (const char *path);
bool isThisHost (const char *otherhost);
bool isSamePath (const char *path1_in, const char *path2_in);
#endif /* !SUBR_H */
