/*	$NetBSD: unix_pass_fd_fix.c,v 1.1.1.1.2.2 2010/11/21 18:31:37 riz Exp $	*/

/*++
/* NAME
/*	unix_pass_fd_fix 3
/* SUMMARY
/*	file descriptor passing bug workarounds
/* SYNOPSIS
/*	#include <iostuff.h>
/*
/*	void	set_unix_pass_fd_fix(workarounds)
/*	const char *workarounds;
/* DESCRIPTION
/*	This module supports programmatic control over workarounds
/*	for sending or receiving file descriptors over UNIX-domain
/*	sockets.
/*
/*	set_unix_pass_fd_fix() takes a list of workarouds in external
/*	form, and stores their internal representation. The result
/*	is used by unix_send_fd() and unix_recv_fd().
/*
/*	Arguments:
/* .IP workarounds
/*	List of zero or more of the following, separated by comma
/*	or whitespace.
/* .RS
/* .IP cmsg_len
/*	Send the CMSG_LEN of the file descriptor, instead of
/*	the total message buffer length.
/* .RE
/* SEE ALSO
/*	unix_send_fd(3) send file descriptor
/*	unix_recv_fd(3) receive file descriptor
/* DIAGNOSTICS
/*	Fatal errors: non-existent workaround.
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
#include <name_mask.h>

int     unix_pass_fd_fix = 0;

/* set_unix_pass_fd_fix - set workaround programmatically */

void    set_unix_pass_fd_fix(const char *workarounds)
{
    const static NAME_MASK table[] = {
	"cmsg_len", UNIX_PASS_FD_FIX_CMSG_LEN,
	0,
    };

    unix_pass_fd_fix = name_mask("descriptor passing workarounds",
				 table, workarounds);
}
