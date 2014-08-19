/*	$NetBSD: mail_version.h,v 1.1.1.15.2.2 2014/08/19 23:59:42 tls Exp $	*/

#ifndef _MAIL_VERSION_H_INCLUDED_
#define _MAIL_VERSION_H_INCLUDED_

/*++
/* NAME
/*	mail_version 3h
/* SUMMARY
/*	globally configurable parameters
/* SYNOPSIS
/*	#include <mail_version.h>
/* DESCRIPTION
/* .nf

 /*
  * Version of this program. Official versions are called a.b.c, and
  * snapshots are called a.b-yyyymmdd, where a=major release number, b=minor
  * release number, c=patchlevel, and yyyymmdd is the release date:
  * yyyy=year, mm=month, dd=day.
  *
  * Patches change both the patchlevel and the release date. Snapshots have no
  * patchlevel; they change the release date only.
  */
#define MAIL_RELEASE_DATE	"20140507"
#define MAIL_VERSION_NUMBER	"2.11.1"

#ifdef SNAPSHOT
#define MAIL_VERSION_DATE	"-" MAIL_RELEASE_DATE
#else
#define MAIL_VERSION_DATE	""
#endif

#ifdef NONPROD
#define MAIL_VERSION_PROD	"-nonprod"
#else
#define MAIL_VERSION_PROD	""
#endif

#define VAR_MAIL_VERSION	"mail_version"
#define DEF_MAIL_VERSION	MAIL_VERSION_NUMBER MAIL_VERSION_DATE MAIL_VERSION_PROD

extern char *var_mail_version;

 /*
  * Release date.
  */
#define VAR_MAIL_RELEASE	"mail_release_date"
#define DEF_MAIL_RELEASE	MAIL_RELEASE_DATE
extern char *var_mail_release;

 /*
  * The following macros stamp executable files as well as core dumps. This
  * information helps to answer the following questions:
  * 
  * - What Postfix versions(s) are installed on this machine?
  * 
  * - Is this installation mixing multiple Postfix versions?
  * 
  * - What Postfix version generated this core dump?
  * 
  * To find out: strings -f file... | grep mail_version=
  */
#include <string.h>

#define MAIL_VERSION_STAMP_DECLARE \
    char *mail_version_stamp

#define MAIL_VERSION_STAMP_ALLOCATE \
    mail_version_stamp = strdup(VAR_MAIL_VERSION "=" DEF_MAIL_VERSION)

 /*
  * Mail version string parser, plus support to compare the compile-time
  * version string of a Postfix program with the run-time version string of a
  * Postfix shared library. When programs are not updated, they may fail in
  * erratic ways when linked against a newer run-time library. Of course the
  * right solution is so-number versioning of the Postfix run-time library.
  */
typedef struct {
    char   *program;			/* postfix */
    int     major;			/* 2 */
    int     minor;			/* 9 */
    int     patch;			/* null */
    char   *snapshot;			/* 20111209-nonprod */
} MAIL_VERSION;

extern MAIL_VERSION *mail_version_parse(const char *, const char **);
extern void mail_version_free(MAIL_VERSION *);
extern const char *get_mail_version(void);
extern void check_mail_version(const char *);

#define MAIL_VERSION_CHECK \
    check_mail_version(DEF_MAIL_VERSION)

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
