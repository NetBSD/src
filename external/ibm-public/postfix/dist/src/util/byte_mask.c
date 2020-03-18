/*	$NetBSD: byte_mask.c,v 1.2 2020/03/18 19:05:21 christos Exp $	*/

/*++
/* NAME
/*	byte_mask 3
/* SUMMARY
/*	map byte sequence to bit mask
/* SYNOPSIS
/*	#include <byte_mask.h>
/*
/*	typedef struct {
/* .in +4
/*	    int     byte_val;
/*	    int     mask;
/* .in -4
/*	} BYTE_MASK;
/*
/*	int	byte_mask(
/*	const char *context,
/*	const BYTE_MASK *table,
/*	const char *bytes);
/*
/*	const char *str_byte_mask(
/*	const char *context,
/*	const BYTE_MASK *table,
/*	int	mask);
/*
/*	int	byte_mask_opt(
/*	const char *context;
/*	const BYTE_MASK *table,
/*	const char *bytes,
/*	int	flags);
/*
/*	const char *str_byte_mask_opt(
/*	VSTRING	*buf,
/*	const char *context,
/*	const BYTE_MASK *table,
/*	int	mask,
/*	int	flags);
/* DESCRIPTION
/*	byte_mask() takes a null-terminated \fItable\fR with (byte
/*	value, single-bit mask) pairs and computes the bit-wise OR
/*	of the masks that correspond to the byte values pointed to
/*	by the \fIbytes\fR argument.
/*
/*	str_byte_mask() translates a bit mask into its equivalent
/*	bytes. The result is written to a static buffer that is
/*	overwritten upon each call.
/*
/*	byte_mask_opt() and str_byte_mask_opt() are extended versions
/*	with additional fine control.
/*
/*	Arguments:
/* .IP buf
/*	Null pointer or pointer to buffer storage.
/* .IP context
/*	What kind of byte values and bit masks are being manipulated,
/*	reported in error messages. Typically, this would be the
/*	name of a user-configurable parameter or command-line
/*	attribute.
/* .IP table
/*	Table with (byte value, single-bit mask) pairs.
/* .IP bytes
/*	A null-terminated string that is to be converted into a bit
/*	mask.
/* .IP mask
/*	A bit mask that is to be converted into null-terminated
/*	string.
/* .IP flags
/*	Bit-wise OR of one or more of the following. Where features
/*	would have conflicting results (e.g., FATAL versus IGNORE),
/*	the feature that takes precedence is described first.
/*
/*	When converting from string to mask, at least one of the
/*	following must be specified: BYTE_MASK_FATAL, BYTE_MASK_RETURN,
/*	BYTE_MASK_WARN or BYTE_MASK_IGNORE.
/*
/*	When converting from mask to string, at least one of the
/*	following must be specified: BYTE_MASK_FATAL, BYTE_MASK_RETURN,
/*	BYTE_MASK_WARN or BYTE_MASK_IGNORE.
/* .RS
/* .IP BYTE_MASK_FATAL
/*	Require that all values in \fIbytes\fR exist in \fItable\fR,
/*	and require that all bits listed in \fImask\fR exist in
/*	\fItable\fR. Terminate with a fatal run-time error if this
/*	condition is not met. This feature is enabled by default
/*	when calling byte_mask() or str_name_mask().
/* .IP BYTE_MASK_RETURN
/*	Require that all values in \fIbytes\fR exist in \fItable\fR,
/*	and require that all bits listed in \fImask\fR exist in
/*	\fItable\fR. Log a warning, and return 0 (byte_mask()) or
/*	a null pointer (str_byte_mask()) if this condition is not
/*	met. This feature is not enabled by default when calling
/*	byte_mask() or str_name_mask().
/* .IP BYTE_MASK_WARN
/*	Require that all values in \fIbytes\fR exist in \fItable\fR,
/*	and require that all bits listed in \fImask\fR exist in
/*	\fItable\fR. Log a warning if this condition is not met,
/*	continue processing, and return all valid bits or bytes.
/*	This feature is not enabled by default when calling byte_mask()
/*	or str_byte_mask().
/* .IP BYTE_MASK_IGNORE
/*	Silently ignore values in \fIbytes\fR that don't exist in
/*	\fItable\fR, and silently ignore bits listed in \fImask\fR
/*	that don't exist in \fItable\fR. This feature is not enabled
/*	by default when calling byte_mask() or str_byte_mask().
/* .IP BYTE_MASK_ANY_CASE
/*	Enable case-insensitive matching. This feature is not
/*	enabled by default when calling byte_mask(); it has no
/*	effect with str_byte_mask().
/* .RE
/*	The value BYTE_MASK_NONE explicitly requests no features,
/*	and BYTE_MASK_DEFAULT enables the default options.
/* DIAGNOSTICS
/*	Fatal: the \fIbytes\fR argument specifies a name not found in
/*	\fItable\fR, or the \fImask\fR specifies a bit not found in
/*	\fItable\fR.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* HISTORY
/*	This code is a clone of Postfix name_mask(3).
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

/* System library. */

#include <sys_defs.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <ctype.h>

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <stringops.h>
#include <byte_mask.h>
#include <vstring.h>

#define STR(x) vstring_str(x)

/* byte_mask_opt - compute mask corresponding to byte string */

int     byte_mask_opt(const char *context, const BYTE_MASK *table,
		              const char *bytes, int flags)
{
    const char myname[] = "byte_mask";
    const char *bp;
    int     result = 0;
    const BYTE_MASK *np;

    if ((flags & BYTE_MASK_REQUIRED) == 0)
	msg_panic("%s: missing BYTE_MASK_FATAL/RETURN/WARN/IGNORE flag",
		  myname);

    /*
     * Iterate over bytes string, and look up each byte in the table. If the
     * byte is found, merge its mask with the result.
     */
    for (bp = bytes; *bp; bp++) {
	int     byte_val = *(const unsigned char *) bp;

	for (np = table; /* void */ ; np++) {
	    if (np->byte_val == 0) {
		if (flags & BYTE_MASK_FATAL) {
		    msg_fatal("unknown %s value \"%c\" in \"%s\"",
			      context, byte_val, bytes);
		} else if (flags & BYTE_MASK_RETURN) {
		    msg_warn("unknown %s value \"%c\" in \"%s\"",
			     context, byte_val, bytes);
		    return (0);
		} else if (flags & BYTE_MASK_WARN) {
		    msg_warn("unknown %s value \"%c\" in \"%s\"",
			     context, byte_val, bytes);
		}
		break;
	    }
	    if ((flags & BYTE_MASK_ANY_CASE) ?
		(TOLOWER(byte_val) == TOLOWER(np->byte_val)) :
		(byte_val == np->byte_val)) {
		if (msg_verbose)
		    msg_info("%s: %c", myname, byte_val);
		result |= np->mask;
		break;
	    }
	}
    }
    return (result);
}

/* str_byte_mask_opt - mask to string */

const char *str_byte_mask_opt(VSTRING *buf, const char *context,
			              const BYTE_MASK *table,
			              int mask, int flags)
{
    const char myname[] = "byte_mask";
    const BYTE_MASK *np;
    static VSTRING *my_buf = 0;

    if ((flags & STR_BYTE_MASK_REQUIRED) == 0)
	msg_panic("%s: missing BYTE_MASK_FATAL/RETURN/WARN/IGNORE flag",
		  myname);

    if (buf == 0) {
	if (my_buf == 0)
	    my_buf = vstring_alloc(1);
	buf = my_buf;
    }
    VSTRING_RESET(buf);

    for (np = table; mask != 0; np++) {
	if (np->byte_val == 0) {
	    if (flags & BYTE_MASK_FATAL) {
		msg_fatal("%s: unknown %s bit in mask: 0x%x",
			  myname, context, mask);
	    } else if (flags & BYTE_MASK_RETURN) {
		msg_warn("%s: unknown %s bit in mask: 0x%x",
			 myname, context, mask);
		return (0);
	    } else if (flags & BYTE_MASK_WARN) {
		msg_warn("%s: unknown %s bit in mask: 0x%x",
			 myname, context, mask);
	    }
	    break;
	}
	if (mask & np->mask) {
	    mask &= ~np->mask;
	    vstring_sprintf_append(buf, "%c", np->byte_val);
	}
    }
    VSTRING_TERMINATE(buf);

    return (STR(buf));
}

#ifdef TEST

 /*
  * Stand-alone test program.
  */
#include <stdlib.h>
#include <vstream.h>
#include <vstring_vstream.h>
#include <name_mask.h>

int     main(int argc, char **argv)
{
    static const BYTE_MASK demo_table[] = {
	'0', 1 << 0,
	'1', 1 << 1,
	'2', 1 << 2,
	'3', 1 << 3,
	0, 0,
    };
    static const NAME_MASK feature_table[] = {
	"DEFAULT", BYTE_MASK_DEFAULT,
	"FATAL", BYTE_MASK_FATAL,
	"ANY_CASE", BYTE_MASK_ANY_CASE,
	"RETURN", BYTE_MASK_RETURN,
	"WARN", BYTE_MASK_WARN,
	"IGNORE", BYTE_MASK_IGNORE,
	0,
    };
    int     in_feature_mask;
    int     out_feature_mask;
    int     demo_mask;
    const char *demo_str;
    VSTRING *out_buf = vstring_alloc(1);
    VSTRING *in_buf = vstring_alloc(1);

    if (argc != 3)
	msg_fatal("usage: %s in-feature-mask out-feature-mask", argv[0]);
    in_feature_mask = name_mask(argv[1], feature_table, argv[1]);
    out_feature_mask = name_mask(argv[2], feature_table, argv[2]);
    while (vstring_get_nonl(in_buf, VSTREAM_IN) != VSTREAM_EOF) {
	demo_mask = byte_mask_opt("name", demo_table,
				  STR(in_buf), in_feature_mask);
	demo_str = str_byte_mask_opt(out_buf, "mask", demo_table,
				     demo_mask, out_feature_mask);
	vstream_printf("%s -> 0x%x -> %s\n",
		       STR(in_buf), demo_mask,
		       demo_str ? demo_str : "(null)");
	demo_mask <<=1;
	demo_str = str_byte_mask_opt(out_buf, "mask", demo_table,
				     demo_mask, out_feature_mask);
	vstream_printf("0x%x -> %s\n",
		       demo_mask, demo_str ? demo_str : "(null)");
	vstream_fflush(VSTREAM_OUT);
    }
    vstring_free(in_buf);
    vstring_free(out_buf);
    exit(0);
}

#endif
