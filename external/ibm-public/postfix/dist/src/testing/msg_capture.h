/*	$NetBSD: msg_capture.h,v 1.2 2026/05/09 18:49:21 christos Exp $	*/

#ifndef _MSB_CAPTURE_H_INCLUDED_
#define _MSB_CAPTURE_H_INCLUDED_

/*++
/* NAME
/*	msg_capture 3h
/* SUMMARY
/*	message capture support
/* SYNOPSIS
/*	#include <msg_capture.h>
/* DESCRIPTION
/* .nf

 /*
  * External API.
  */
typedef struct MSG_CAPTURE MSG_CAPTURE;

extern MSG_CAPTURE *msg_capt_create(ssize_t init_size);
extern void msg_capt_start(MSG_CAPTURE *mp);
extern void msg_capt_stop(MSG_CAPTURE *mp);
extern const char *msg_capt_expose(MSG_CAPTURE *mp);
extern void msg_capt_free(MSG_CAPTURE *mp);

/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	porcupine.org
/*--*/

#endif
