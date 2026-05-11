/*	$NetBSD: mock_open_as.c,v 1.2.2.2 2026/05/11 17:13:59 martin Exp $	*/

/*++
/* NAME
/*	mock_open_as 3
/* SUMMARY
/*	test scaffolding
/* SYNOPSIS
/*	#include <mock_open_as.h>
/*
/*	typedef struct MOCK_OPEN_AS_REQ {
/* in +4
/*	const char *want_path,
/*	uid_t	want_uid,
/*	gid_t	want_gid,
/*	const char *out_data,
/*	int	out_errno)
/* .in -4
/*	} MOCK_OPEN_AS_REQ;
/*
/*	void	setup_mock_vstream_fopen_as(const MOCK_OPEN_AS_REQ *request)
/* DESCRIPTION
/*	This module provides a mock vstream_fopen_as() implementation
/*	that returns static results for expected calls.
/*
/*	setup_mock_vstream_fopen_as() makes a shallow copy of its input:
/* .IP want_path, want_uid, want_gid
/*	The expected vstream_fopen_as() arguments. If the mock
/*	vstream_fopen_as() is called with an unexpected argument, the
/*	mock terminates the test with a fatal error.
/* .IP out_data
/*	The data that can be read from the stream returned by
/*	vstream_fopen_as().
/*  .IP out_errno
/*	If not NULL, vstream_fopen_as() will return NULL and set errno
/*	to the value of this argument.
/* SEE ALSO
/*	open_as(3), the real thing
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	porcupine.org
/*--*/

 /*
  * System library.
  */
#include <sys_defs.h>
#include <errno.h>
#include <string.h>

 /*
  * Utility library.
  */
#include <msg.h>
#include <open_as.h>
#include <vstring.h>

 /*
  * Application-specific.
  */
#include <mock_open_as.h>

static MOCK_OPEN_AS_REQ mock_req;

/* setup_mock_vstream_fopen_as - set up expectation */

void    setup_mock_vstream_fopen_as(const MOCK_OPEN_AS_REQ *request)
{
    mock_req = *request;
}

/* vstream_fopen_as - fake implementation */

VSTREAM *vstream_fopen_as(const char *path, int flags, mode_t mode,
			          uid_t uid, gid_t gid)
{
    VSTREAM *stream;
    VSTRING *data_buffer;

    /* TODO(wietse) replace with match_str() and match_decimal(). */
    if (strcmp(path, mock_req.want_path) != 0)
	msg_fatal("vstream_fopen: got path '%s', want '%s'",
		  path, mock_req.want_path);
    if (uid != mock_req.want_uid)
	msg_fatal("vstream_fopen: got uid '%d', want '%d'",
		  (int) uid, (int) mock_req.want_uid);
    if (gid != mock_req.want_gid)
	msg_fatal("vstream_fopen: got gid '%d', want '%d'",
		  (int) gid, (int) mock_req.want_gid);

    if (mock_req.out_errno != 0) {
	errno = mock_req.out_errno;
	return (0);
    }
    data_buffer = vstring_alloc(strlen(mock_req.out_data));
    vstring_strcpy(data_buffer, mock_req.out_data);
    stream = vstream_memopen(data_buffer, flags);
    vstream_control(stream, CA_VSTREAM_CTL_OWN_VSTRING, CA_VSTREAM_CTL_END);
    return (stream);
}
