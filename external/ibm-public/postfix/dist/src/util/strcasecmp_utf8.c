/*	$NetBSD: strcasecmp_utf8.c,v 1.2.4.2 2017/04/21 16:52:53 bouyer Exp $	*/

/*++
/* NAME
/*	strcasecmp_utf8 3
/* SUMMARY
/*	caseless string comparison
/* SYNOPSIS
/*	#include <stringops.h>
/*
/*	int	strcasecmp_utf8(
/*	const char *s1,
/*	const char *s2)
/*
/*	int	strncasecmp_utf8(
/*	const char *s1,
/*	const char *s2,
/*	ssize_t	len)
/* AUXILIARY FUNCTIONS
/*	int	strcasecmp_utf8x(
/*	int	flags,
/*	const char *s1,
/*	const char *s2)
/*
/*	int	strncasecmp_utf8x(
/*	int	flags,
/*	const char *s1,
/*	const char *s2,
/*	ssize_t	len)
/* DESCRIPTION
/*	strcasecmp_utf8() implements caseless string comparison for
/*	UTF-8 text, with an API similar to strcasecmp(). Only ASCII
/*	characters are casefolded when the code is compiled without
/*	EAI support or when util_utf8_enable is zero.
/*
/*	strncasecmp_utf8() implements caseless string comparison
/*	for UTF-8 text, with an API similar to strncasecmp(). Only
/*	ASCII characters are casefolded when the code is compiled
/*	without EAI support or when util_utf8_enable is zero.
/*
/*	strcasecmp_utf8x() and strncasecmp_utf8x() implement a more
/*	complex API that provides the above functionality and more.
/*
/*	Arguments:
/* .IP "s1, s2"
/*	Null-terminated strings to be compared.
/* .IP len
/*	String length before casefolding.
/* .IP flags
/*	Zero or CASEF_FLAG_UTF8. The latter flag enables UTF-8 case
/*	folding instead of folding only ASCII characters. This flag
/*	is ignored when compiled without EAI support.
/* SEE ALSO
/*	casefold(), casefold text for caseless comparison.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*
/*	Wietse Venema
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
/*--*/

 /*
  * System library.
  */
#include <sys_defs.h>
#include <string.h>

#ifdef STRCASECMP_IN_STRINGS_H
#include <strings.h>
#endif

 /*
  * Utility library.
  */
#include <stringops.h>

#define STR(x)	vstring_str(x)

static VSTRING *f1;			/* casefold result for s1 */
static VSTRING *f2;			/* casefold result for s2 */

/* strcasecmp_utf8_init - initialize */

static void strcasecmp_utf8_init(void)
{
    f1 = vstring_alloc(100);
    f2 = vstring_alloc(100);
}

/* strcasecmp_utf8x - caseless string comparison */

int     strcasecmp_utf8x(int flags, const char *s1, const char *s2)
{

    /*
     * Short-circuit optimization for ASCII-only text. This may be slower
     * than using a cache for all results. We must not expose strcasecmp(3)
     * to non-ASCII text.
     */
    if (allascii(s1) && allascii(s2))
	return (strcasecmp(s1, s2));

    if (f1 == 0)
	strcasecmp_utf8_init();

    /*
     * Cross our fingers and hope that strcmp() remains agnostic of
     * charactersets and locales.
     */
    flags &= CASEF_FLAG_UTF8;
    casefoldx(flags, f1, s1, -1);
    casefoldx(flags, f2, s2, -1);
    return (strcmp(STR(f1), STR(f2)));
}

/* strncasecmp_utf8x - caseless string comparison */

int     strncasecmp_utf8x(int flags, const char *s1, const char *s2,
			          ssize_t len)
{

    /*
     * Consider using a cache for all results.
     */
    if (f1 == 0)
	strcasecmp_utf8_init();

    /*
     * Short-circuit optimization for ASCII-only text. This may be slower
     * than using a cache for all results. See comments above for limitations
     * of strcasecmp().
     */
    if (allascii_len(s1, len) && allascii_len(s2, len))
	return (strncasecmp(s1, s2, len));

    /*
     * Caution: casefolding may change the number of bytes. See comments
     * above for concerns about strcmp().
     */
    flags &= CASEF_FLAG_UTF8;
    casefoldx(flags, f1, s1, len);
    casefoldx(flags, f2, s2, len);
    return (strcmp(STR(f1), STR(f2)));
}

#ifdef TEST
#include <stdio.h>
#include <stdlib.h>
#include <vstream.h>
#include <vstring_vstream.h>
#include <msg_vstream.h>
#include <argv.h>

int     main(int argc, char **argv)
{
    VSTRING *buffer = vstring_alloc(1);
    ARGV   *cmd;
    char  **args;
    int     len;
    int     flags;
    int     res;

    msg_vstream_init(argv[0], VSTREAM_ERR);
    flags = CASEF_FLAG_UTF8;
    util_utf8_enable = 1;
    while (vstring_fgets_nonl(buffer, VSTREAM_IN)) {
	vstream_printf("> %s\n", STR(buffer));
	cmd = argv_split(STR(buffer), CHARS_SPACE);
	if (cmd->argc == 0 || cmd->argv[0][0] == '#')
	    continue;
	args = cmd->argv;

	/*
	 * Compare two strings.
	 */
	if (strcmp(args[0], "compare") == 0 && cmd->argc == 3) {
	    res = strcasecmp_utf8x(flags, args[1], args[2]);
	    vstream_printf("\"%s\" %s \"%s\"\n",
			   args[1],
			   res < 0 ? "<" : res == 0 ? "==" : ">",
			   args[2]);
	}

	/*
	 * Compare two substrings.
	 */
	else if (strcmp(args[0], "compare-len") == 0 && cmd->argc == 4
		 && sscanf(args[3], "%d", &len) == 1 && len >= 0) {
	    res = strncasecmp_utf8x(flags, args[1], args[2], len);
	    vstream_printf("\"%.*s\" %s \"%.*s\"\n",
			   len, args[1],
			   res < 0 ? "<" : res == 0 ? "==" : ">",
			   len, args[2]);
	}

	/*
	 * Usage.
	 */
	else {
	    vstream_printf("Usage: %s compare <s1> <s2> | compare-len <s1> <s2> <len>\n",
			   argv[0]);
	}
	vstream_fflush(VSTREAM_OUT);
	argv_free(cmd);
    }
    exit(0);
}

#endif					/* TEST */
