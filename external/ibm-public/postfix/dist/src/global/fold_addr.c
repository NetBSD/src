/*	$NetBSD: fold_addr.c,v 1.1.1.1.2.2 2009/09/15 06:02:43 snj Exp $	*/

/*++
/* NAME
/*	fold_addr 3
/* SUMMARY
/*	address case folding
/* SYNOPSIS
/*	#include <fold_addr.h>
/*
/*	char	*fold_addr(addr, flags)
/*	char	*addr;
/*	int	flags;
/* DESCRIPTION
/*	fold_addr() case folds an address according to the options
/*	specified with \fIflags\fR. The result value is the address
/*	argument.
/*
/*	Arguments
/* .IP addr
/*	Null-terminated writable string with the address.
/* .IP flags
/*	Zero or the bit-wise OR of:
/* .RS
/* .IP FOLD_ADDR_USER
/*	Case fold the address local part.
/* .IP FOLD_ADDR_HOST
/*	Case fold the address domain part.
/* .IP FOLD_ADDR_ALL
/*	Alias for (FOLD_ADDR_USER | FOLD_ADDR_HOST).
/* .RE
/* SEE ALSO
/*	msg(3) diagnostics interface
/* DIAGNOSTICS
/*	Fatal errors: memory allocation problem.
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

/* Utility library. */

#include <stringops.h>

/* Global library. */

#include <fold_addr.h>

/* fold_addr - case fold mail address */

char   *fold_addr(char *addr, int flags)
{
    char   *cp;

    /*
     * Fold the address as appropriate.
     */
    switch (flags & FOLD_ADDR_ALL) {
    case FOLD_ADDR_HOST:
	if ((cp = strrchr(addr, '@')) != 0)
	    lowercase(cp + 1);
	break;
    case FOLD_ADDR_USER:
	if ((cp = strrchr(addr, '@')) != 0) {
	    *cp = 0;
	    lowercase(addr);
	    *cp = '@';
	    break;
	}
	/* FALLTHROUGH */
    case FOLD_ADDR_USER | FOLD_ADDR_HOST:
	lowercase(addr);
	break;
    }
    return (addr);
}
