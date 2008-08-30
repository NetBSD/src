/*++
/* NAME
/*	ehlo_mask 3
/* SUMMARY
/*	map EHLO keywords to bit mask
/* SYNOPSIS
/*	#include <ehlo_mask.h>
/*
/*	#define EHLO_MASK_8BITMIME	(1<<0)
/*	#define EHLO_MASK_PIPELINING	(1<<1)
/*	#define EHLO_MASK_SIZE		(1<<2)
/*	#define EHLO_MASK_VRFY		(1<<3)
/*	#define EHLO_MASK_ETRN		(1<<4)
/*	#define EHLO_MASK_AUTH		(1<<5)
/*	#define EHLO_MASK_VERP		(1<<6)
/*	#define EHLO_MASK_STARTTLS	(1<<7)
/*	#define EHLO_MASK_XCLIENT	(1<<8)
/*	#define EHLO_MASK_XFORWARD	(1<<9)
/*	#define EHLO_MASK_ENHANCEDSTATUSCODES	(1<<10)
/*	#define EHLO_MASK_DSN		(1<<11)
/*	#define EHLO_MASK_SILENT	(1<<15)
/*
/*	int	ehlo_mask(keyword_list)
/*	const char *keyword_list;
/*
/*	const char *str_ehlo_mask(bitmask)
/*	int	bitmask;
/* DESCRIPTION
/*	ehlo_mask() computes the bit-wise OR of the masks that correspond
/*	to the names listed in the \fIkeyword_list\fR argument, separated by
/*	comma and/or whitespace characters. Undefined names are silently
/*	ignored.
/*
/*	str_ehlo_mask() translates a mask into its equivalent names.
/*	The result is written to a static buffer that is overwritten
/*	upon each call. Undefined bits cause a fatal run-time error.
/* DIAGNOSTICS
/*	Fatal: str_ehlo_mask() found an undefined bit.
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

/* System library.*/

#include <sys_defs.h>

/* Utility library. */

#include <name_mask.h>

/* Global library. */

#include <ehlo_mask.h>

 /*
  * The lookup table.
  */
static const NAME_MASK ehlo_mask_table[] = {
    "8BITMIME", EHLO_MASK_8BITMIME,
    "AUTH", EHLO_MASK_AUTH,
    "ETRN", EHLO_MASK_ETRN,
    "PIPELINING", EHLO_MASK_PIPELINING,
    "SIZE", EHLO_MASK_SIZE,
    "VERP", EHLO_MASK_VERP,
    "VRFY", EHLO_MASK_VRFY,
    "XCLIENT", EHLO_MASK_XCLIENT,
    "XFORWARD", EHLO_MASK_XFORWARD,
    "STARTTLS", EHLO_MASK_STARTTLS,
    "ENHANCEDSTATUSCODES", EHLO_MASK_ENHANCEDSTATUSCODES,
    "DSN", EHLO_MASK_DSN,
    "SILENT-DISCARD", EHLO_MASK_SILENT,	/* XXX In-band signaling */
    0,
};

/* ehlo_mask - string to bit mask */

int     ehlo_mask(const char *mask_str)
{

    /*
     * We allow "STARTTLS" besides "starttls, because EHLO keywords are often
     * spelled in uppercase. We ignore non-existent EHLO keywords so people
     * can switch between Postfix versions without trouble.
     */
    return (name_mask_opt("ehlo string mask", ehlo_mask_table,
			  mask_str, NAME_MASK_ANY_CASE));
}

/* str_ehlo_mask - mask to string */

const char *str_ehlo_mask(int mask_bits)
{

    /*
     * We don't allow non-existent bits. Doing so makes no sense at this
     * time.
     */
    return (str_name_mask("ehlo bitmask", ehlo_mask_table, mask_bits));
}

#ifdef TEST

 /*
  * Stand-alone test program.
  */
#include <stdlib.h>
#include <vstream.h>
#include <vstring.h>
#include <vstring_vstream.h>

int     main(int unused_argc, char **unused_argv)
{
    int     mask_bits;
    VSTRING *buf = vstring_alloc(1);
    const char *mask_string;

    while (vstring_get_nonl(buf, VSTREAM_IN) != VSTREAM_EOF) {
	mask_bits = ehlo_mask(vstring_str(buf));
	mask_string = str_ehlo_mask(mask_bits);
	vstream_printf("%s -> 0x%x -> %s\n", vstring_str(buf), mask_bits,
		       mask_string);
	vstream_fflush(VSTREAM_OUT);
    }
    vstring_free(buf);
    exit(0);
}

#endif
