/*	$NetBSD: grep.h,v 1.2 2004/05/05 15:06:33 cjep Exp $	*/

/*-
 * Copyright (c) 1999 James Howard and Dag-Erling Coïdan Smørgrav
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/types.h>

#include <regex.h>
#include <stdio.h>
#include <zlib.h>

#define VERSION "20040505"

#define	GREP_READ	0
#define GREP_SKIP	1
#define GREP_RECURSE	2

#define BIN_FILE_BIN	0
#define BIN_FILE_SKIP	1
#define BIN_FILE_TEXT	2

#define LINK_EXPLICIT	0
#define LINK_FOLLOW	1
#define LINK_SKIP	2

#define MAX_LINE_MATCHES	30

typedef struct {
	size_t len;
	int line_no;
	int off;
	char *file;
	char *dat;
} str_t;

/* Flags passed to regcomp() and regexec() */
extern int cflags, eflags;

/* Command line flags */
extern int Aflag, Bflag, Fflag, Lflag, bflag, cflag, lflag, mflag, 
	nflag, oflag, qflag, sflag, vflag, wflag, xflag;

extern int colours;
extern const char *grep_colour;		 
extern char fn_endchar, fn_colonchar, fn_dashchar;
extern char line_endchar;

extern int output_filenames;

extern int maxcount;

/* Argv[0] flags */
extern int zgrep;

extern int binbehave, dirbehave, devbehave; 
/* extern int linkbehave; */
extern char *stdin_label;

extern int first, matchall, patterns, tail;
extern char **pattern;
extern regex_t *r_pattern;

/* 
 * For regex errors, 512 seems to be big enough
 */
#define RE_ERROR_BUF 	512

extern char re_error[RE_ERROR_BUF + 1];

/* util.c */
int procfile(char *fn);
int grep_tree(char **argv);

void *grep_malloc(size_t size);
void *grep_realloc(void *ptr, size_t size);
void printline(str_t *line, int sep, regmatch_t *matches, int m);

/* queue.c */
void initqueue(void);
void enqueue(str_t *x);
void printqueue(void);
void clearqueue(void);

/* mmfile.c */
typedef struct mmfile {
	int fd;
	size_t len;
	char *base, *end, *ptr;
} mmf_t;

mmf_t *mmopen(char *fn, char *mode);
void mmclose(mmf_t *mmf);
char *mmfgetln(mmf_t *mmf, size_t *l);
void mmrewind(mmf_t *mmf);

/* file.c */
struct file;
typedef struct file file_t;

file_t *grep_fdopen(int fd, char *mode);
file_t *grep_open(char *path, char *mode);

int grep_bin_file(file_t *f);
char *grep_fgetln(file_t *f, size_t *l);
void grep_close(file_t *f);

/* binary.c */
int bin_file(FILE * f);
int gzbin_file(gzFile * f);
int mmbin_file(mmf_t *f);

