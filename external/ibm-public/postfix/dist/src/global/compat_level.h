/*	$NetBSD: compat_level.h,v 1.2.2.1 2023/12/25 12:43:31 martin Exp $	*/

#ifndef _COMPAT_LEVEL_H_INCLUDED_
#define _COMPAT_LEVEL_H_INCLUDED_

/*++
/* NAME
/*	compat_level 3h
/* SUMMARY
/*	compatibility_level support
/* SYNOPSIS
/*	#include <compat_level.h>
/* DESCRIPTION
/* .nf

 /*
  * External interface.
  */
extern void compat_level_relop_register(void);
extern long compat_level_from_string(const char *,
		              void PRINTFLIKE(1, 2) (*) (const char *,...));
extern long compat_level_from_numbers(long, long, long,
		              void PRINTFLIKE(1, 2) (*) (const char *,...));
extern const char *compat_level_to_string(long,
			      void PRINTFLIKE(1, 2) (*) (const char *,...));

#define compat_level_from_major(major, msg_fn) \
	compat_level_from_major_minor((major), 0, (msg_fn))
#define compat_level_from_major_minor(major, minor, msg_fn) \
	compat_level_from_numbers((major), (minor), 0, (msg_fn))

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

#endif
