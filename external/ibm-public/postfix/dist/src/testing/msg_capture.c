/*	$NetBSD: msg_capture.c,v 1.2 2026/05/09 18:49:21 christos Exp $	*/

/*++
/* NAME
/*	msg_capture 3
/* SUMMARY
/*	message capture support
/* SYNOPSIS
/*	#include <msg_capture.h>
/*
/*	typedef struct MSG_CAPTURE MSG_CAPTURE;
/*
/*	MSG_CAPTURE *msg_capt_create(ssize_t init_size)
/*
/*	void	msg_capt_start(MSG_CAPTURE *mp)
/*
/*	void	msg_capt_stop(MSG_CAPTURE *mp)
/*
/*	const char *msg_capt_expose(MSG_CAPTURE *mp)
/*
/*	void	msg_capt_free(MSG_CAPTURE *mp)
/* DESCRIPTION
/*	This module captures msg(3) logging by temporarily redirecting
/*	VSTREAM_ER to a buffer.
/*
/*	msg_capt_create() creates a MSG_CAPTURE instance.
/*
/*	msg_capt_start() starts "recording" msg(3) output. It is an
/*	error to call this function while a recording is in progress.
/*
/*	msg_capt_stop() stops a "recording". It is an error to call this
/*	function while no recording is in progress.
/*
/*	msg_capt_expose() exposes the contents of a recording as a
/*	null-terminated string. It is an error to call this function
/*	while a recording is in progress.
/*
/*	msg_capt_free() destroys a MSG_CAPTURE instance.
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
#include <string.h>

 /*
  * Utility library.
  */
#include <msg.h>
#include <mymalloc.h>
#include <vstream.h>
#include <vstring.h>

 /*
  * Testing library.
  */
#include <msg_capture.h>

 /*
  * TODO(wietse) make this a proper VSTREAM interface or test helper API.
  */

/* vstream_swap - capture output for testing */

static void vstream_swap(VSTREAM *one, VSTREAM *two)
{
    VSTREAM save;

    save = *one;
    *one = *two;
    *two = save;
}

struct MSG_CAPTURE {
    VSTRING *buffer;
    VSTREAM *stream;
};

MSG_CAPTURE *msg_capt_create(ssize_t init_size)
{
    MSG_CAPTURE *mp;

    mp = (MSG_CAPTURE *) mymalloc(sizeof(*mp));
    mp->buffer = vstring_alloc(init_size);
    mp->stream = 0;
    return (mp);
}

void    msg_capt_start(MSG_CAPTURE *mp)
{
    if (mp->stream != 0)
	msg_panic("%s: capture is in progress", __func__);
    VSTRING_RESET(mp->buffer);
    VSTRING_TERMINATE(mp->buffer);
    if ((mp->stream = vstream_memopen(mp->buffer, O_WRONLY)) == 0)
	msg_fatal("open memory stream: %m");
    vstream_swap(VSTREAM_ERR, mp->stream);
}

void    msg_capt_stop(MSG_CAPTURE *mp)
{
    if (mp->stream == 0)
	msg_panic("%s: no capture in progress", __func__);
    vstream_swap(mp->stream, VSTREAM_ERR);
    if (vstream_fclose(mp->stream))
	msg_fatal("close memory stream: %m");
    mp->stream = 0;
    VSTRING_TERMINATE(mp->buffer);
}

const char *msg_capt_expose(MSG_CAPTURE *mp)
{
    if (mp->stream != 0)
	msg_panic("%s: capture is in progress", __func__);
    return (vstring_str(mp->buffer));
}

void    msg_capt_free(MSG_CAPTURE *mp)
{
    if (mp->stream != 0)
	msg_capt_stop(mp);
    vstring_free(mp->buffer);
    myfree((void *) mp);
}
