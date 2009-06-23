/*	$NetBSD: header_body_checks.h,v 1.1.1.1 2009/06/23 10:08:46 tron Exp $	*/

#ifndef _HBC_H_INCLUDED_
#define _HBC_H_INCLUDED_

/*++
/* NAME
/*	header_body_checks 3h
/* SUMMARY
/*	delivery agent header/body checks
/* SYNOPSIS
/*	#include <header_body_checks.h>
/* DESCRIPTION
/* .nf

 /*
  * Global library.
  */
#include <mime_state.h>
#include <maps.h>

 /*
  * Postfix < 2.5 compatibility.
  */
#ifndef MIME_HDR_FIRST
#define MIME_HDR_FIRST		(1)
#define MIME_HDR_LAST		(3)
#endif

 /*
  * External interface.
  */
typedef struct {
    const char *map_class;		/* parameter name */
    MAPS   *maps;			/* map handle */
} HBC_MAP_INFO;

typedef struct {
    void    (*logger) (void *, const char *, const char *, const char *, const char *);
    void    (*prepend) (void *, int, const char *, ssize_t, off_t);
    char   *(*extend) (void *, const char *, int, const char *, const char *, const char *, ssize_t, off_t);
} HBC_CALL_BACKS;

typedef struct {
    HBC_CALL_BACKS *call_backs;
    HBC_MAP_INFO map_info[1];		/* actually, a bunch */
} HBC_CHECKS;

#define HBC_CHECKS_STAT_IGNORE	((char *) 0)
#define HBC_CHECKS_STAT_UNKNOWN	(&hbc_checks_unknown)

extern HBC_CHECKS *hbc_header_checks_create(const char *, const char *,
					         const char *, const char *,
					         const char *, const char *,
					            HBC_CALL_BACKS *);
extern HBC_CHECKS *hbc_body_checks_create(const char *, const char *,
					          HBC_CALL_BACKS *);
extern char *hbc_header_checks(void *, HBC_CHECKS *, int, const HEADER_OPTS *,
			               VSTRING *, off_t);
extern char *hbc_body_checks(void *, HBC_CHECKS *, const char *, ssize_t, off_t);

#define hbc_header_checks_free(hbc) _hbc_checks_free((hbc), HBC_HEADER_SIZE)
#define hbc_body_checks_free(hbc) _hbc_checks_free((hbc), 1)

 /*
  * The following are NOT part of the external API.
  */
#define HBC_HEADER_SIZE	(MIME_HDR_LAST - MIME_HDR_FIRST + 1)
extern void _hbc_checks_free(HBC_CHECKS *, ssize_t);
extern const char hbc_checks_unknown;

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

#endif
