/*	$NetBSD: quote_flags.c,v 1.2 2020/03/18 19:05:16 christos Exp $	*/

/*++
/* NAME
/*	quote_flags 3
/* SUMMARY
/*	quote rfc 821/822 local part
/* SYNOPSIS
/*	#include <quote_flags.h>
/*
/*	int	quote_flags_from_string(const char *string)
/*
/*	const char *quote_flags_to_string(VSTRING *res_buf, int mask)
/* DESCRIPTION
/*	quote_flags_from_string() converts symbolic flag names into
/*	the corresponding internal bitmask. This logs a warning and
/*	returns zero if an unknown symbolic name is specified.
/*
/*	quote_flags_to_string() converts from internal bitmask to
/*	the corresponding symbolic names. This logs a warning and
/*	returns a null pointer if an unknown bitmask is specified.
/*
/*	Arguments:
/* .IP string
/*	Symbolic representation of a quote_flags bitmask, for
/*	example: \fB8bitclean | bare_localpart\fR. The conversion
/*	is case-insensitive.
/* .IP res_buf
/*	Storage for the quote_flags_to_string() result, which has
/*	the same form as the string argument. If a null pointer is
/*	specified, quote_flags_to_string() uses storage that is
/*	overwritten with each call.
/* .IP mask
/*	Binary representation of quote_flags.
/* DIAGNOSTICS
/*	Fatal error: out of memory; or unknown bitmask name or value.
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

 /*
  * Utility library.
  */
#include <name_mask.h>

 /*
  * Global library.
  */
#include <quote_flags.h>

static const NAME_MASK quote_flags_table[] = {
    "8bitclean", QUOTE_FLAG_8BITCLEAN,
    "expose_at", QUOTE_FLAG_EXPOSE_AT,
    "append", QUOTE_FLAG_APPEND,
    "bare_localpart", QUOTE_FLAG_BARE_LOCALPART,
    0,
};

/* quote_flags_from_string - symbolic quote flags to internal form */

int     quote_flags_from_string(const char *quote_flags_string)
{
    return (name_mask_delim_opt("quote_flags_from_string", quote_flags_table,
				quote_flags_string, "|",
				NAME_MASK_WARN | NAME_MASK_ANY_CASE));
}

/* quote_flags_to_string - internal form to symbolic quote flags */

const char *quote_flags_to_string(VSTRING *res_buf, int quote_flags_mask)
{
    static VSTRING *my_buf;

    if (res_buf == 0 && (res_buf = my_buf) == 0)
	res_buf = my_buf = vstring_alloc(20);
    return (str_name_mask_opt(res_buf, "quote_flags_to_string",
			      quote_flags_table, quote_flags_mask,
			      NAME_MASK_WARN | NAME_MASK_PIPE));
}
