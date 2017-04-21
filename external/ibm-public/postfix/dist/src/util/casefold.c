/*	$NetBSD: casefold.c,v 1.2.4.2 2017/04/21 16:52:52 bouyer Exp $	*/

/*++
/* NAME
/*	casefold 3
/* SUMMARY
/*	casefold text for caseless comparison
/* SYNOPSIS
/*	#include <stringops.h>
/*
/*	char	*casefold(
/*	VSTRING *dst,
/*	const char *src)
/*
/*	char	*casefold_append(
/*	VSTRING *dst,
/*	const char *src)
/*
/*	char	*casefold_len(
/*	VSTRING *dst,
/*	const char *src,
/*	ssize_t	src_len)
/* AUXILIARY FUNCTIONS
/*	char	*casefoldx(
/*	int	flags,
/*	VSTRING *dst,
/*	const char *src,
/*	ssize_t	src_len)
/* DESCRIPTION
/*	casefold() converts text to a form that is suitable for
/*	caseless comparison, rather than presentation to humans.
/*
/*	When compiled without EAI support or util_utf8_enable is
/*	zero, casefold() implements ASCII case folding, leaving
/*	non-ASCII byte values unchanged.
/*
/*	When compiled with EAI support and util_utf8_enable is
/*	non-zero, casefold() implements UTF-8 case folding using
/*	the en_US locale, as recommended when the conversion result
/*	is not meant to be presented to humans.
/*
/*	casefold_len() implements casefold() with a source length
/*	argument.
/*
/*	casefold_append() implements casefold() without overwriting
/*	the result.
/*
/*	casefoldx() implements a more complex API that implements
/*	all of the above and more.
/*
/*	Arguments:
/* .IP src
/*	Null-terminated input string.
/* .IP dest
/*	Output buffer, null-terminated. Specify a null pointer to
/*	use an internal buffer that is overwritten upon each call.
/* .IP src_len
/*	The string length, -1 to determine the length dynamically.
/* .IP flags
/*	Bitwise OR of zero or more of the following:
/* .RS
/* .IP CASEF_FLAG_UTF8
/*	Enable UTF-8 support. This flag has no effect when compiled
/*	without EAI support.
/* .IP CASEF_FLAG_APPEND
/*	Append the result to the buffer, instead of overwriting it.
/* DIAGNOSTICS
/*	All errors are fatal. There appear to be no input-dependent
/*	errors.
/*
/*	With the ICU 4.8 library, there is no casefold error for
/*	UTF-8 code points U+0000..U+10FFFF (including surrogate
/*	range), not even when running inside an empty chroot jail.
/*	Nor does malformed UTF-8 trigger errors; non-UTF-8 bytes
/*	are copied verbatim. Based on ICU 4.8 source-code review
/*	and experimentation(!) we conclude that UTF-8 casefolding
/*	has no data-dependent error cases, and that it is safe to
/*	treat all casefolding errors as fatal runtime errors.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*--*/

/* System library. */

#include <sys_defs.h>
#include <string.h>
#include <ctype.h>
#ifndef NO_EAI
#include <unicode/ucasemap.h>
#include <unicode/ustring.h>
#include <unicode/uchar.h>
#endif

/* Utility library. */

#include <msg.h>
#include <stringops.h>

#define STR(x) vstring_str(x)
#define LEN(x) VSTRING_LEN(x)

/* casefoldx - casefold an UTF-8 string */

char   *casefoldx(int flags, VSTRING *dest, const char *src, ssize_t len)
{
    size_t  old_len;

#ifdef NO_EAI

    /*
     * ASCII mode only.
     */
    if (len < 0)
	len = strlen(src);
    if ((flags & CASEF_FLAG_APPEND) == 0)
	VSTRING_RESET(dest);
    old_len = VSTRING_LEN(dest);
    vstring_strncat(dest, src, len);
    lowercase(STR(dest) + old_len);
    return (STR(dest));
#else

    /*
     * Unicode mode.
     */
    const char myname[] = "casefold";
    static VSTRING *fold_buf = 0;
    static UCaseMap *csm = 0;
    UErrorCode error;
    ssize_t space_needed;
    int     n;

    /*
     * Handle special cases.
     */
    if (len < 0)
	len = strlen(src);
    if (dest == 0)
	dest = (fold_buf != 0 ? fold_buf : (fold_buf = vstring_alloc(100)));
    if ((flags & CASEF_FLAG_APPEND) == 0)
	VSTRING_RESET(dest);
    old_len = VSTRING_LEN(dest);

    /*
     * All-ASCII input, or ASCII mode only.
     */
    if ((flags & CASEF_FLAG_UTF8) == 0 || allascii(src)) {
	vstring_strncat(dest, src, len);
	lowercase(STR(dest) + old_len);
	return (STR(dest));
    }

    /*
     * ICU 4.8 ucasemap_utf8FoldCase() does not complain about UTF-8 syntax
     * errors. XXX Based on source-code review we conclude that non-UTF-8
     * bytes are copied verbatim, and experiments confirm this. Given that
     * this behavior is intentional, we assume that it will stay that way.
     */
#if 0
    if (valid_utf8_string(src, len) == 0) {
	if (err)
	    *err = "malformed UTF-8 or invalid codepoint";
	return (0);
    }
#endif

    /*
     * One-time initialization. With ICU 4.8 this works while chrooted.
     */
    if (csm == 0) {
	error = U_ZERO_ERROR;
	csm = ucasemap_open("en_US", U_FOLD_CASE_DEFAULT, &error);
	if (U_SUCCESS(error) == 0)
	    msg_fatal("ucasemap_open error: %s", u_errorName(error));
    }

    /*
     * Fold the input, adjusting the buffer size if needed. Safety: don't
     * loop forever.
     * 
     * Note: the requested amount of space for casemapped output (as reported
     * with space_needed below) does not include storage for the null
     * terminator. The terminator is written only when the output buffer is
     * large enough. This is why we overallocate space when the output does
     * not fit. But if the output fits exactly, then the ouput will be
     * unterminated, and we have to terminate the output ourselves.
     */
    for (n = 0; n < 3; n++) {
	error = U_ZERO_ERROR;
	space_needed = ucasemap_utf8FoldCase(csm, STR(dest) + old_len,
				     vstring_avail(dest), src, len, &error);
	if (U_SUCCESS(error)) {
	    VSTRING_AT_OFFSET(dest, old_len + space_needed);
	    if (vstring_avail(dest) == 0)	/* exact fit, no terminator */
		VSTRING_TERMINATE(dest);	/* add terminator */
	    break;
	} else if (error == U_BUFFER_OVERFLOW_ERROR) {
	    VSTRING_SPACE(dest, space_needed + 1);	/* for terminator */
	} else {
	    msg_fatal("%s: conversion error for \"%s\": %s",
		      myname, src, u_errorName(error));
	}
    }
    return (STR(dest));
#endif						/* NO_EAI */
}

#ifdef TEST

static void encode_utf8(VSTRING *buffer, int codepoint)
{
    const char myname[] = "encode_utf8";

    VSTRING_RESET(buffer);
    if (codepoint < 0x80) {
	VSTRING_ADDCH(buffer, codepoint);
    } else if (codepoint < 0x800) {
	VSTRING_ADDCH(buffer, 0xc0 | (codepoint >> 6));
	VSTRING_ADDCH(buffer, 0x80 | (codepoint & 0x3f));
    } else if (codepoint < 0x10000) {
	VSTRING_ADDCH(buffer, 0xe0 | (codepoint >> 12));
	VSTRING_ADDCH(buffer, 0x80 | ((codepoint >> 6) & 0x3f));
	VSTRING_ADDCH(buffer, 0x80 | (codepoint & 0x3f));
    } else if (codepoint <= 0x10FFFF) {
	VSTRING_ADDCH(buffer, 0xf0 | (codepoint >> 18));
	VSTRING_ADDCH(buffer, 0x80 | ((codepoint >> 12) & 0x3f));
	VSTRING_ADDCH(buffer, 0x80 | ((codepoint >> 6) & 0x3f));
	VSTRING_ADDCH(buffer, 0x80 | (codepoint & 0x3f));
    } else {
	msg_panic("%s: out-of-range codepoint U+%X", myname, codepoint);
    }
    VSTRING_TERMINATE(buffer);
}

#include <stdlib.h>
#include <stdio.h>
#include <locale.h>

#include <vstream.h>
#include <vstring_vstream.h>
#include <msg_vstream.h>

int     main(int argc, char **argv)
{
    VSTRING *buffer = vstring_alloc(1);
    VSTRING *dest = vstring_alloc(1);
    char   *bp;
    char   *conv_res;
    char   *cmd;
    int     codepoint, first, last;
    VSTREAM *fp;

    if (setlocale(LC_ALL, "C") == 0)
	msg_fatal("setlocale(LC_ALL, C) failed: %m");

    msg_vstream_init(argv[0], VSTREAM_ERR);

    util_utf8_enable = 1;

    VSTRING_SPACE(buffer, 256);			/* chroot/file pathname */

    while (vstring_fgets_nonl(buffer, VSTREAM_IN)) {
	bp = STR(buffer);
	vstream_printf("> %s\n", bp);
	cmd = mystrtok(&bp, CHARS_SPACE);
	if (cmd == 0 || *cmd == '#')
	    continue;
	while (ISSPACE(*bp))
	    bp++;

	/*
	 * Null-terminated string.
	 */
	if (strcmp(cmd, "fold") == 0) {
	    conv_res = casefold(dest, bp);
	    vstream_printf("\"%s\" ->fold \"%s\"\n", bp, conv_res);
	}

	/*
	 * Codepoint range.
	 */
	else if (strcmp(cmd, "range") == 0
		 && sscanf(bp, "%i %i", &first, &last) == 2
		 && first <= last) {
	    for (codepoint = first; codepoint <= last; codepoint++) {
		if (codepoint >= 0xD800 && codepoint <= 0xDFFF) {
		    vstream_printf("skipping surrogate range\n");
		    codepoint = 0xDFFF;
		} else {
		    encode_utf8(buffer, codepoint);
		    if (msg_verbose)
			vstream_printf("U+%X -> %s\n", codepoint, STR(buffer));
		    if (valid_utf8_string(STR(buffer), LEN(buffer)) == 0)
			msg_fatal("bad utf-8 encoding for U+%X\n", codepoint);
		    casefold(dest, STR(buffer));
		}
	    }
	    vstream_printf("range completed: 0x%x..0x%x\n", first, last);
	}

	/*
	 * Chroot directory.
	 */
	else if (strcmp(cmd, "chroot") == 0
		 && sscanf(bp, "%255s", STR(buffer)) == 1) {
	    if (geteuid() == 0) {
		if (chdir(STR(buffer)) < 0)
		    msg_fatal("chdir(%s): %m", STR(buffer));
		if (chroot(STR(buffer)) < 0)
		    msg_fatal("chroot(%s): %m", STR(buffer));
		vstream_printf("chroot %s completed\n", STR(buffer));
	    }
	}

	/*
	 * File.
	 */
	else if (strcmp(cmd, "file") == 0
		 && sscanf(bp, "%255s", STR(buffer)) == 1) {
	    if ((fp = vstream_fopen(STR(buffer), O_RDONLY, 0)) == 0)
		msg_fatal("open(%s): %m", STR(buffer));
	    while (vstring_fgets_nonl(buffer, fp))
		vstream_printf("%s\n", casefold(dest, STR(buffer)));
	    vstream_fclose(fp);
	}

	/*
	 * Verbose.
	 */
	else if (strcmp(cmd, "verbose") == 0
		 && sscanf(bp, "%i", &msg_verbose) == 1) {
	     /* void */ ;
	}

	/*
	 * Usage
	 */
	else {
	    vstream_printf("Usage: %s chroot <path> | file <path> | fold <text> | range <first> <last> | verbose <int>\n",
			   argv[0]);
	}
	vstream_fflush(VSTREAM_OUT);
    }
    vstring_free(buffer);
    vstring_free(dest);
    exit(0);
}

#endif					/* TEST */
