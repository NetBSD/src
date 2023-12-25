/*	$NetBSD: compat_level.c,v 1.2.2.1 2023/12/25 12:43:31 martin Exp $	*/

/*++
/* NAME
/*	compat_level 3
/* SUMMARY
/*	compatibility_level support
/* SYNOPSIS
/*	#include <compat_level.h>
/*
/*	void compat_level_relop_register(void)
/*
/*	long	compat_level_from_string(
/*	const char *str,
/*	void	PRINTFLIKE(1, 2) (*msg_fn)(const char *,...))
/*
/*	long	compat_level_from_numbers(
/*	long	major,
/*	long	minor,
/*	long	patch,
/*	void	PRINTFLIKE(1, 2) (*msg_fn)(const char *,...))
/*
/*	const char *compat_level_to_string(
/*	long	compat_level,
/*	void	PRINTFLIKE(1, 2) (*msg_fn)(const char *,...))
/* AUXULIARY FUNCTIONS
/*	long	compat_level_from_major_minor(
/*	long	major,
/*	long	minor,
/*	void	PRINTFLIKE(1, 2) (*msg_fn)(const char *,...))
/*
/*	long	compat_level_from_major(
/*	long	major,
/*	void	PRINTFLIKE(1, 2) (*msg_fn)(const char *,...))
/* DESCRIPTION
/*	This module supports compatibility level syntax with
/*	"major.minor.patch" but will also accept the shorter forms
/*	"major.minor" and "major" (missing members default to zero).
/*	Compatibility levels with multiple numbers cannot be compared
/*	as strings or as floating-point numbers (for example, "3.10"
/*	would be smaller than "3.9").
/*
/*	The major number can range from [0..2047] inclusive (11
/*	bits) or more, while the minor and patch numbers can range
/*	from [0..1023] inclusive (10 bits).
/*
/*	compat_level_from_string() converts a compatibility level
/*	from string form to numerical form for easy comparison.
/*	Valid input results in a non-negative result. In case of
/*	error, compat_level_from_string() reports the problem with
/*	the provided function, and returns -1 if that function does
/*	not terminate execution.
/*
/*	compat_level_from_numbers() creates an internal-form
/*	compatibility level from distinct numbers. Valid input
/*	results in a non-negative result. In case of error,
/*	compat_level_from_numbers() reports the problem with the
/*	provided function, and returns -1 if that function does not
/*	terminate execution.
/*
/*	The functions compat_level_from_major_minor() and
/*	compat_level_from_major() are helpers that default the missing
/*	information to zeroes.
/*
/*	compat_level_to_string() converts a compatibility level
/*	from numerical form to canonical string form. Valid input
/*	results in a non-null result. In case of error,
/*	compat_level_to_string() reports the problem with the
/*	provided function, and returns a null pointer if that
/*	function does not terminate execution.
/*
/*	compat_level_relop_register() registers a mac_expand() callback
/*	that registers operators such as <=level, >level, that compare
/*	compatibility levels. This function should be called before
/*	loading parameter settings from main.cf.
/* DIAGNOSTICS
/*	info, .., panic: bad compatibility_level syntax.
/* BUGS
/*	The patch and minor fields range from 0..1023 (10 bits) while
/*	the major field ranges from 0..COMPAT_MAJOR_SHIFT47 or more
/*	(11 bits or more).
/*
/*	This would be a great use case for functions returning
/*	StatusOr<compat_level_t> or StatusOr<string>, but is it a bit
/*	late for a port to C++.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
/*--*/

 /*
  * System library.
  */
#include <sys_defs.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>

 /*
  * Utility library.
  */
#include <mac_expand.h>
#include <msg.h>
#include <sane_strtol.h>

 /*
  * For easy comparison we convert a three-number compatibility level into
  * just one number, using different bit ranges for the major version, minor
  * version, and patch level.
  * 
  * We use long integers because standard C guarantees that long has at last 32
  * bits instead of int which may have only 16 bits (though it is unlikely
  * that Postfix would run on such systems). That gives us 11 or more bits
  * for the major version, and 10 bits for minor the version and patchlevel.
  * 
  * Below are all the encoding details in one place. This is easier to verify
  * than wading through code.
  */
#define COMPAT_MAJOR_SHIFT \
	(COMPAT_MINOR_SHIFT + COMPAT_MINOR_WIDTH)

#define COMPAT_MINOR_SHIFT	COMPAT_PATCH_WIDTH
#define COMPAT_MINOR_BITS	0x3ff
#define COMPAT_MINOR_WIDTH	10

#define COMPAT_PATCH_BITS	0x3ff
#define COMPAT_PATCH_WIDTH	10

#define GOOD_MAJOR(m)	((m) >= 0 && (m) <= (LONG_MAX >> COMPAT_MAJOR_SHIFT))
#define GOOD_MINOR(m)	((m) >= 0 && (m) <= COMPAT_MINOR_BITS)
#define GOOD_PATCH(p)	((p) >= 0 && (p) <= COMPAT_PATCH_BITS)

#define ENCODE_MAJOR(m)	((m) << COMPAT_MAJOR_SHIFT)
#define ENCODE_MINOR(m)	((m) << COMPAT_MINOR_SHIFT)
#define ENCODE_PATCH(p)	(p)

#define DECODE_MAJOR(l)	((l) >> COMPAT_MAJOR_SHIFT)
#define DECODE_MINOR(l)	(((l) >> COMPAT_MINOR_SHIFT) & COMPAT_MINOR_BITS)
#define DECODE_PATCH(l)	((l) & COMPAT_PATCH_BITS)

 /*
  * Global library.
  */
#include <compat_level.h>

/* compat_level_from_string - convert major[.minor] to comparable type */

long    compat_level_from_string(const char *str,
		         void PRINTFLIKE(1, 2) (*msg_fn) (const char *,...))
{
    long    major, minor, patch, res = 0;
    const char *start;
    char   *remainder;

    start = str;
    major = sane_strtol(start, &remainder, 10);
    if (start < remainder && (*remainder == 0 || *remainder == '.')
	&& errno != ERANGE && GOOD_MAJOR(major)) {
	res = ENCODE_MAJOR(major);
	if (*remainder == 0)
	    return res;
	start = remainder + 1;
	minor = sane_strtol(start, &remainder, 10);
	if (start < remainder && (*remainder == 0 || *remainder == '.')
	    && errno != ERANGE && GOOD_MINOR(minor)) {
	    res |= ENCODE_MINOR(minor);
	    if (*remainder == 0)
		return (res);
	    start = remainder + 1;
	    patch = sane_strtol(start, &remainder, 10);
	    if (start < remainder && *remainder == 0 && errno != ERANGE
		&& GOOD_PATCH(patch)) {
		return (res | ENCODE_PATCH(patch));
	    }
	}
    }
    msg_fn("malformed compatibility level syntax: \"%s\"", str);
    return (-1);
}

/* compat_level_from_numbers - internal form from numbers */

long    compat_level_from_numbers(long major, long minor, long patch,
		         void PRINTFLIKE(1, 2) (*msg_fn) (const char *,...))
{
    const char myname[] = "compat_level_from_numbers";

    /*
     * Sanity checks.
     */
    if (!GOOD_MAJOR(major)) {
	msg_fn("%s: bad major version: %ld", myname, major);
	return (-1);
    }
    if (!GOOD_MINOR(minor)) {
	msg_fn("%s: bad minor version: %ld", myname, minor);
	return (-1);
    }
    if (!GOOD_PATCH(patch)) {
	msg_fn("%s: bad patch level: %ld", myname, patch);
	return (-1);
    }

    /*
     * Conversion.
     */
    return (ENCODE_MAJOR(major) | ENCODE_MINOR(minor) | ENCODE_PATCH(patch));
}

/* compat_level_to_string - pretty-print a compatibility level */

const char *compat_level_to_string(long compat_level,
		         void PRINTFLIKE(1, 2) (*msg_fn) (const char *,...))
{
    const char myname[] = "compat_level_to_string";
    static VSTRING *buf;
    long    major;
    long    minor;
    long    patch;

    /*
     * Sanity check.
     */
    if (compat_level < 0) {
        msg_fn("%s: bad compatibility level: %ld", myname, compat_level);
        return (0);
    }

    /*
     * Compatibility levels 0..2 have no minor or patch level.
     */
    if (buf == 0)
        buf = vstring_alloc(10);
    major = DECODE_MAJOR(compat_level);
    if (!GOOD_MAJOR(major)) {
        msg_fn("%s: bad compatibility major level: %ld", myname, compat_level);
        return (0);
    }
    vstring_sprintf(buf, "%ld", major);
    if (major > 2) {

        /*
         * Expect that major.minor will be common.
         */
        minor = DECODE_MINOR(compat_level);
        vstring_sprintf_append(buf, ".%ld", minor);

        /*
         * Expect that major.minor.patch will be rare.
         */
        patch = DECODE_PATCH(compat_level);
        if (patch)
            vstring_sprintf_append(buf, ".%ld", patch);
    }
    return (vstring_str(buf));
}

/* compat_relop_eval - mac_expand callback */

static MAC_EXP_OP_RES compat_relop_eval(const char *left_str, int relop,
					        const char *rite_str)
{
    const char myname[] = "compat_relop_eval";
    long    left_val, rite_val, delta;

    /*
     * Negative result means error.
     */
    if ((left_val = compat_level_from_string(left_str, msg_warn)) < 0
	|| (rite_val = compat_level_from_string(rite_str, msg_warn)) < 0)
	return (MAC_EXP_OP_RES_ERROR);

    /*
     * Valid result. The difference between non-negative numbers will no
     * overflow.
     */
    delta = left_val - rite_val;

    switch (relop) {
    case MAC_EXP_OP_TOK_EQ:
	return (mac_exp_op_res_bool[delta == 0]);
    case MAC_EXP_OP_TOK_NE:
	return (mac_exp_op_res_bool[delta != 0]);
    case MAC_EXP_OP_TOK_LT:
	return (mac_exp_op_res_bool[delta < 0]);
    case MAC_EXP_OP_TOK_LE:
	return (mac_exp_op_res_bool[delta <= 0]);
    case MAC_EXP_OP_TOK_GE:
	return (mac_exp_op_res_bool[delta >= 0]);
    case MAC_EXP_OP_TOK_GT:
	return (mac_exp_op_res_bool[delta > 0]);
    default:
	msg_panic("%s: unknown operator: %d",
		  myname, relop);
    }
}

/* compat_level_register - register comparison operators */

void    compat_level_relop_register(void)
{
    int     compat_level_relops[] = {
	MAC_EXP_OP_TOK_EQ, MAC_EXP_OP_TOK_NE,
	MAC_EXP_OP_TOK_GT, MAC_EXP_OP_TOK_GE,
	MAC_EXP_OP_TOK_LT, MAC_EXP_OP_TOK_LE,
	0,
    };
    static int register_done;

    if (register_done++ == 0)
	mac_expand_add_relop(compat_level_relops, "level", compat_relop_eval);
}

#ifdef TEST
#include <unistd.h>

#include <htable.h>
#include <mymalloc.h>
#include <stringops.h>
#include <vstring.h>
#include <vstream.h>
#include <vstring_vstream.h>

static const char *lookup(const char *name, int unused_mode, void *context)
{
    HTABLE *table = (HTABLE *) context;

    return (htable_find(table, name));
}

static void test_expand(void)
{
    VSTRING *buf = vstring_alloc(100);
    VSTRING *result = vstring_alloc(100);
    char   *cp;
    char   *name;
    char   *value;
    HTABLE *table;
    int     stat;

    /*
     * Add relops that compare string lengths instead of content.
     */
    compat_level_relop_register();

    /*
     * Loop over the inputs.
     */
    while (!vstream_feof(VSTREAM_IN)) {

	table = htable_create(0);

	/*
	 * Read a block of definitions, terminated with an empty line.
	 */
	while (vstring_get_nonl(buf, VSTREAM_IN) != VSTREAM_EOF) {
	    vstream_printf("<< %s\n", vstring_str(buf));
	    vstream_fflush(VSTREAM_OUT);
	    if (VSTRING_LEN(buf) == 0)
		break;
	    cp = vstring_str(buf);
	    name = mystrtok(&cp, CHARS_SPACE "=");
	    value = mystrtok(&cp, CHARS_SPACE "=");
	    htable_enter(table, name, value ? mystrdup(value) : 0);
	}

	/*
	 * Read a block of patterns, terminated with an empty line or EOF.
	 */
	while (vstring_get_nonl(buf, VSTREAM_IN) != VSTREAM_EOF) {
	    vstream_printf("<< %s\n", vstring_str(buf));
	    vstream_fflush(VSTREAM_OUT);
	    if (VSTRING_LEN(buf) == 0)
		break;
	    VSTRING_RESET(result);
	    stat = mac_expand(result, vstring_str(buf), MAC_EXP_FLAG_NONE,
			      (char *) 0, lookup, (void *) table);
	    vstream_printf("stat=%d result=%s\n", stat, vstring_str(result));
	    vstream_fflush(VSTREAM_OUT);
	}
	htable_free(table, myfree);
	vstream_printf("\n");
    }

    /*
     * Clean up.
     */
    vstring_free(buf);
    vstring_free(result);
}

static void test_convert(void)
{
    VSTRING *buf = vstring_alloc(100);
    long    compat_level;
    const char *as_string;

    /*
     * Read compatibility level.
     */
    while (vstring_get_nonl(buf, VSTREAM_IN) != VSTREAM_EOF) {
	if ((compat_level = compat_level_from_string(vstring_str(buf),
						     msg_warn)) < 0)
	    continue;
	msg_info("%s -> 0x%lx", vstring_str(buf), compat_level);
	errno = ERANGE;
	if ((as_string = compat_level_to_string(compat_level,
						msg_warn)) == 0)
	    continue;
	msg_info("0x%lx->%s", compat_level, as_string);
    }
    vstring_free(buf);
}

static NORETURN usage(char **argv)
{
    msg_fatal("usage: %s option\n-c (convert)\n-c (expand)", argv[0]);
}

int     main(int argc, char **argv)
{
    int     ch;
    int     mode = 0;

#define MODE_EXPAND	(1<<0)
#define MODE_CONVERT	(1<<1)

    while ((ch = GETOPT(argc, argv, "cx")) > 0) {
	switch (ch) {
	case 'c':
	    mode |= MODE_CONVERT;
	    break;
	case 'v':
	    msg_verbose++;
	    break;
	case 'x':
	    mode |= MODE_EXPAND;
	    break;
	default:
	    usage(argv);
	}
    }
    switch (mode) {
    case MODE_CONVERT:
	test_convert();
	break;
    case MODE_EXPAND:
	test_expand();
	break;
    default:
	usage(argv);
    }
    exit(0);
}

#endif
