/*++
/* NAME
/*	dsn_print
/* SUMMARY
/*	write DSN structure to stream
/* SYNOPSIS
/*	#include <dsn_print.h>
/*
/*	int	dsn_print(print_fn, stream, flags, ptr)
/*	ATTR_PRINT_MASTER_FN print_fn;
/*	VSTREAM *stream;
/*	int	flags;
/*	void	*ptr;
/* DESCRIPTION
/*	dsn_print() writes a DSN structure to the named stream using
/*	the specified attribute print routine. dsn_print() is meant
/*	to be passed as a call-back to attr_print(), thusly:
/*
/*	... ATTR_TYPE_FUNC, dsn_print, (void *) dsn, ...
/* DIAGNOSTICS
/*	Fatal: out of memory.
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

/* Utility library. */

#include <attr.h>

/* Global library. */

#include <mail_proto.h>
#include <dsn_print.h>

/* dsn_print - write DSN to stream */

int     dsn_print(ATTR_PRINT_MASTER_FN print_fn, VSTREAM *fp,
		          int flags, void *ptr)
{
    DSN    *dsn = (DSN *) ptr;
    int     ret;

    /*
     * The attribute order is determined by backwards compatibility. It can
     * be sanitized after all the ad-hoc DSN read/write code is replaced.
     */
    ret = print_fn(fp, flags | ATTR_FLAG_MORE,
		   ATTR_TYPE_STR, MAIL_ATTR_DSN_STATUS, dsn->status,
		   ATTR_TYPE_STR, MAIL_ATTR_DSN_DTYPE, dsn->dtype,
		   ATTR_TYPE_STR, MAIL_ATTR_DSN_DTEXT, dsn->dtext,
		   ATTR_TYPE_STR, MAIL_ATTR_DSN_MTYPE, dsn->mtype,
		   ATTR_TYPE_STR, MAIL_ATTR_DSN_MNAME, dsn->mname,
		   ATTR_TYPE_STR, MAIL_ATTR_DSN_ACTION, dsn->action,
		   ATTR_TYPE_STR, MAIL_ATTR_WHY, dsn->reason,
		   ATTR_TYPE_END);
    return (ret);
}
