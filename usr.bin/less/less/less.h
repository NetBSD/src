/*	$NetBSD: less.h,v 1.4 2001/01/04 23:05:55 lukem Exp $	*/

/*
 * Copyright (c) 1984,1985,1989,1994,1995,1996,1999  Mark Nudelman
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice in the documentation and/or other materials provided with 
 *    the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR 
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT 
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR 
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE 
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN 
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


/*
 * Standard include file for "less".
 */

/*
 * Defines for MSDOS_COMPILER.
 */
#define	MSOFTC		1	/* Microsoft C */
#define	BORLANDC	2	/* Borland C */
#define	WIN32C		3	/* Windows (Borland C or Microsoft C) */
#define	DJGPPC		4	/* DJGPP C */

/*
 * Include the file of compile-time options.
 * The <> make cc search for it in -I., not srcdir.
 */
#include <defines.h>

#ifdef _SEQUENT_
/*
 * Kludge for Sequent Dynix systems that have sigsetmask, but
 * it's not compatible with the way less calls it.
 * {{ Do other systems need this? }}
 */
#undef HAVE_SIGSETMASK
#endif

/*
 * Language details.
 */
#if HAVE_VOID
#define	VOID_POINTER	void *
#else
#define	VOID_POINTER	char *
#define	void  int
#endif
#if HAVE_CONST
#define	constant	const
#else
#define	constant
#endif

#define	public		/* PUBLIC FUNCTION */

/* Library function declarations */

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_STDIO_H
#include <stdio.h>
#endif
#if HAVE_FCNTL_H
#include <fcntl.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_CTYPE_H
#include <ctype.h>
#endif
#if STDC_HEADERS
#include <stdlib.h>
#include <string.h>
#endif
#ifdef _OSK
#include <modes.h>
#include <strings.h>
#endif
#if MSDOS_COMPILER==WIN32C || MSDOS_COMPILER==DJGPPC
#include <io.h>
#endif

#if !STDC_HEADERS
char *getenv();
off_t lseek();
VOID_POINTER calloc();
void free();
#endif

#if !HAVE_UPPER_LOWER
#define	isupper(c)	((c) >= 'A' && (c) <= 'Z')
#define	islower(c)	((c) >= 'a' && (c) <= 'z')
#define	toupper(c)	((c) - 'a' + 'A')
#define	tolower(c)	((c) - 'A' + 'a')
#endif

#ifndef NULL
#define	NULL	0
#endif

#ifndef TRUE
#define	TRUE		1
#endif
#ifndef FALSE
#define	FALSE		0
#endif

#define	OPT_OFF		0
#define	OPT_ON		1
#define	OPT_ONPLUS	2

#if !HAVE_MEMCPY
#ifndef memcpy
#define	memcpy(to,from,len)	bcopy((from),(to),(len))
#endif
#endif

#define	BAD_LSEEK	((off_t)-1)

/*
 * Special types and constants.
 */
typedef off_t		POSITION;
#define PR_POSITION	"%lld"
#define PR_CAST		long long
#define MAX_PRINT_POSITION 20
#define MAX_PRINT_INT      10

#define	NULL_POSITION	((POSITION)(-1))

/*
 * Flags for open()
 */
#if MSDOS_COMPILER || OS2
#define	OPEN_READ	(O_RDONLY|O_BINARY)
#else
#ifdef _OSK
#define	OPEN_READ	(S_IREAD)
#else
#ifdef O_RDONLY
#define	OPEN_READ	(O_RDONLY)
#else
#define	OPEN_READ	(0)
#endif
#endif
#endif

#if defined(O_WRONLY) && defined(O_APPEND)
#define	OPEN_APPEND	(O_APPEND|O_WRONLY)
#else
#ifdef _OSK
#define OPEN_APPEND	(S_IWRITE)
#else
#define	OPEN_APPEND	(1)
#endif
#endif

/*
 * Set a file descriptor to binary mode.
 */
#if MSDOS_COMPILER==MSOFTC
#define	SET_BINARY(f)	_setmode(f, _O_BINARY);
#else
#if MSDOS_COMPILER
#define	SET_BINARY(f)	setmode(f, O_BINARY)
#else
#define	SET_BINARY(f)
#endif
#endif

/*
 * Does the shell treat "?" as a metacharacter?
 */
#if MSDOS_COMPILER || OS2 || _OSK
#define	SHELL_META_QUEST 0
#else
#define	SHELL_META_QUEST 1
#endif

#define	SPACES_IN_FILENAMES 1

/*
 * An IFILE represents an input file.
 */
#define	IFILE		VOID_POINTER
#define	NULL_IFILE	((IFILE)NULL)

/*
 * The structure used to represent a "screen position".
 * This consists of a file position, and a screen line number.
 * The meaning is that the line starting at the given file
 * position is displayed on the ln-th line of the screen.
 * (Screen lines before ln are empty.)
 */
struct scrpos
{
	POSITION pos;
	int ln;
};

typedef union parg
{
	char *p_string;
	int p_int;
} PARG;

#define	NULL_PARG	((PARG *)NULL)

struct textlist
{
	char *string;
	char *endstring;
};

#define	EOI		(-1)

#define	READ_INTR	(-2)

/* How quiet should we be? */
#define	NOT_QUIET	0	/* Ring bell at eof and for errors */
#define	LITTLE_QUIET	1	/* Ring bell only for errors */
#define	VERY_QUIET	2	/* Never ring bell */

/* How should we prompt? */
#define	PR_SHORT	0	/* Prompt with colon */
#define	PR_MEDIUM	1	/* Prompt with message */
#define	PR_LONG		2	/* Prompt with longer message */

/* How should we handle backspaces? */
#define	BS_SPECIAL	0	/* Do special things for underlining and bold */
#define	BS_NORMAL	1	/* \b treated as normal char; actually output */
#define	BS_CONTROL	2	/* \b treated as control char; prints as ^H */

/* How should we search? */
#define	SRCH_FORW	000001	/* Search forward from current position */
#define	SRCH_BACK	000002	/* Search backward from current position */
#define	SRCH_NO_MOVE	000004	/* Highlight, but don't move */
#define	SRCH_FIND_ALL	000010	/* Find and highlight all matches */
#define	SRCH_NO_MATCH	000100	/* Search for non-matching lines */
#define	SRCH_PAST_EOF	000200	/* Search past end-of-file, into next file */
#define	SRCH_FIRST_FILE	000400	/* Search starting at the first file */
#define	SRCH_NO_REGEX	001000	/* Don't use regular expressions */

#define	SRCH_REVERSE(t)	(((t) & SRCH_FORW) ? \
				(((t) & ~SRCH_FORW) | SRCH_BACK) : \
				(((t) & ~SRCH_BACK) | SRCH_FORW))

/* */
#define	NO_MCA		0
#define	MCA_DONE	1
#define	MCA_MORE	2

#define	CC_OK		0	/* Char was accepted & processed */
#define	CC_QUIT		1	/* Char was a request to abort current cmd */
#define	CC_ERROR	2	/* Char could not be accepted due to error */
#define	CC_PASS		3	/* Char was rejected (internal) */

/* Special chars used to tell put_line() to do something special */
#define	AT_NORMAL	(0)
#define	AT_UNDERLINE	(1)
#define	AT_BOLD		(2)
#define	AT_BLINK	(3)
#define	AT_INVIS	(4)
#define	AT_STANDOUT	(5)

#if IS_EBCDIC_HOST
/*
 * Long definition for EBCDIC.
 * Since the argument is usually a constant, this macro normally compiles
 * into a constant.
 */
#define CONTROL(c) ( \
	(c)=='[' ? '\047' : \
	(c)=='a' ? '\001' : \
	(c)=='b' ? '\002' : \
	(c)=='c' ? '\003' : \
	(c)=='d' ? '\067' : \
	(c)=='e' ? '\055' : \
	(c)=='f' ? '\056' : \
	(c)=='g' ? '\057' : \
	(c)=='h' ? '\026' : \
	(c)=='i' ? '\005' : \
	(c)=='j' ? '\025' : \
	(c)=='k' ? '\013' : \
	(c)=='l' ? '\014' : \
	(c)=='m' ? '\015' : \
	(c)=='n' ? '\016' : \
	(c)=='o' ? '\017' : \
	(c)=='p' ? '\020' : \
	(c)=='q' ? '\021' : \
	(c)=='r' ? '\022' : \
	(c)=='s' ? '\023' : \
	(c)=='t' ? '\074' : \
	(c)=='u' ? '\075' : \
	(c)=='v' ? '\062' : \
	(c)=='w' ? '\046' : \
	(c)=='x' ? '\030' : \
	(c)=='y' ? '\031' : \
	(c)=='z' ? '\077' : \
	(c)=='A' ? '\001' : \
	(c)=='B' ? '\002' : \
	(c)=='C' ? '\003' : \
	(c)=='D' ? '\067' : \
	(c)=='E' ? '\055' : \
	(c)=='F' ? '\056' : \
	(c)=='G' ? '\057' : \
	(c)=='H' ? '\026' : \
	(c)=='I' ? '\005' : \
	(c)=='J' ? '\025' : \
	(c)=='K' ? '\013' : \
	(c)=='L' ? '\014' : \
	(c)=='M' ? '\015' : \
	(c)=='N' ? '\016' : \
	(c)=='O' ? '\017' : \
	(c)=='P' ? '\020' : \
	(c)=='Q' ? '\021' : \
	(c)=='R' ? '\022' : \
	(c)=='S' ? '\023' : \
	(c)=='T' ? '\074' : \
	(c)=='U' ? '\075' : \
	(c)=='V' ? '\062' : \
	(c)=='W' ? '\046' : \
	(c)=='X' ? '\030' : \
	(c)=='Y' ? '\031' : \
	(c)=='Z' ? '\077' : \
	(c)=='|' ? '\031' : \
	(c)=='\\' ? '\034' : \
	(c)=='^' ? '\036' : \
	(c)&077)
#else
#define	CONTROL(c)	((c)&037)
#endif /* IS_EBCDIC_HOST */

#define	ESC		CONTROL('[')

#if _OSK_MWC32
#define	LSIGNAL(sig,func)	os9_signal(sig,func)
#else
#define	LSIGNAL(sig,func)	signal(sig,func)
#endif

#define	S_INTERRUPT	01
#define	S_STOP		02
#define S_WINCH		04
#define	ABORT_SIGS()	(sigs & (S_INTERRUPT|S_STOP))

#define	QUIT_OK		0
#define	QUIT_ERROR	1
#define	QUIT_SAVED_STATUS (-1)

/* filestate flags */
#define	CH_CANSEEK	001
#define	CH_KEEPOPEN	002
#define	CH_POPENED	004
#define	CH_HELPFILE	010

#define	ch_zero()	((POSITION)0)

#define	FAKE_HELPFILE	"@/\\less/\\help/\\file/\\@"

#include "funcs.h"

