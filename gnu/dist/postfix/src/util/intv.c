/*++
/* NAME
/*	intv 3
/* SUMMARY
/*	integer array utilities
/* SYNOPSIS
/*	#include <intv.h>
/*
/*	INTV	*intv_alloc(len)
/*	int	len;
/*
/*	INTV	*intv_free(intvp)
/*	INTV	*intvp;
/*
/*	void	intv_add(intvp, count, arg, ...)
/*	INTV	*intvp;
/*	int	count;
/*	int	*arg;
/* DESCRIPTION
/*	The functions in this module manipulate arrays of integers.
/*	An INTV structure contains the following members:
/* .IP len
/*	The actual length of the \fIintv\fR array.
/* .IP intc
/*	The number of \fIintv\fR elements used.
/* .IP intv
/*	An array of integer values.
/* .PP
/*	intv_alloc() returns an empty integer array of the requested
/*	length. The result is ready for use by intv_add().
/*
/*	intv_add() copies zero or more integers and adds them to the
/*	specified integer array.
/*
/*	intv_free() releases storage for an integer array, and conveniently
/*	returns a null pointer.
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

/* System libraries. */

#include <sys_defs.h>
#include <stdlib.h>			/* 44BSD stdarg.h uses abort() */
#include <stdarg.h>

/* Application-specific. */

#include "mymalloc.h"
#include "msg.h"
#include "intv.h"

/* intv_free - destroy integer array */

INTV   *intv_free(INTV *intvp)
{
    myfree((char *) intvp->intv);
    myfree((char *) intvp);
    return (0);
}

/* intv_alloc - initialize integer array */

INTV   *intv_alloc(int len)
{
    INTV   *intvp;

    /*
     * Sanity check.
     */
    if (len < 1)
	msg_panic("intv_alloc: bad array length %d", len);

    /*
     * Initialize.
     */
    intvp = (INTV *) mymalloc(sizeof(*intvp));
    intvp->len = len;
    intvp->intv = (int *) mymalloc(intvp->len * sizeof(intvp->intv[0]));
    intvp->intc = 0;
    return (intvp);
}

/* intv_add - add integer to vector */

void    intv_add(INTV *intvp, int count,...)
{
    va_list ap;

    /*
     * Make sure that always intvp->intc < intvp->len.
     */
    va_start(ap, count);
    while (count-- > 0) {
	if (intvp->intc >= intvp->len) {
	    intvp->len *= 2;
	    intvp->intv = (int *) myrealloc((char *) intvp->intv,
					     intvp->len * sizeof(int));
	}
	intvp->intv[intvp->intc++] = va_arg(ap, int);
    }
    va_end(ap);
}
