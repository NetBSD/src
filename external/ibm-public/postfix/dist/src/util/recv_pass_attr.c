/*	$NetBSD: recv_pass_attr.c,v 1.1.1.1.16.1 2017/04/21 16:52:53 bouyer Exp $	*/

/*++
/* NAME
/*	recv_pass_attr 3
/* SUMMARY
/*	predicate if string is all numerical
/* SYNOPSIS
/*	#include <listen.h>
/*
/*	int	recv_pass_attr(fd, attr, timeout, bufsize)
/*	int	fd;
/*	HTABLE	**attr;
/*	int	timeout;
/*	ssize_t	bufsize;
/* DESCRIPTION
/*	recv_pass_attr() receives named attributes over the specified
/*	descriptor. The result value is zero for success, -1 for error.
/*
/*	Arguments:
/* .IP fd
/*	The file descriptor to read from.
/* .IP attr
/*	Pointer to attribute list pointer. The target is set to
/*	zero on error or when the received attribute list is empty,
/*	otherwise it is assigned a pointer to non-empty attribute
/*	list.
/* .IP timeout
/*	The deadline for receiving all attributes.
/* .IP bufsize
/*	The read buffer size. Specify 1 to avoid reading past the
/*	end of the attribute list.
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

#include <iostuff.h>
#include <htable.h>
#include <vstream.h>
#include <attr.h>
#include <mymalloc.h>
#include <listen.h>

/* recv_pass_attr - receive connection attributes */

int     recv_pass_attr(int fd, HTABLE **attr, int timeout, ssize_t bufsize)
{
    VSTREAM *fp;
    int     stream_err;

    /*
     * Set up a temporary VSTREAM to receive the attributes.
     * 
     * XXX We use one-character reads to simplify the implementation.
     */
    fp = vstream_fdopen(fd, O_RDWR);
    vstream_control(fp,
		    CA_VSTREAM_CTL_BUFSIZE(bufsize),
		    CA_VSTREAM_CTL_TIMEOUT(timeout),
		    CA_VSTREAM_CTL_START_DEADLINE,
		    CA_VSTREAM_CTL_END);
    stream_err = (attr_scan(fp, ATTR_FLAG_NONE,
			    ATTR_TYPE_HASH, *attr = htable_create(1),
			    ATTR_TYPE_END) < 0
		  || vstream_feof(fp) || vstream_ferror(fp));
    vstream_fdclose(fp);

    /*
     * Error reporting and recovery.
     */
    if (stream_err) {
	htable_free(*attr, myfree);
	*attr = 0;
	return (-1);
    } else {
	if ((*attr)->used == 0) {
	    htable_free(*attr, myfree);
	    *attr = 0;
	}
	return (0);
    }
}
