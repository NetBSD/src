/*	$NetBSD: mail_version.h,v 1.1.1.12.2.4 2013/01/07 15:41:56 riz Exp $	*/

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
#define MAIL_RELEASE_DATE	"20121213"
#define MAIL_VERSION_NUMBER	"2.8.13"

#ifdef SNAPSHOT
# define MAIL_VERSION_DATE	"-" MAIL_RELEASE_DATE
#else
# define MAIL_VERSION_DATE	""
#endif

#ifdef NONPROD
# define MAIL_VERSION_PROD	"-nonprod"
#else
# define MAIL_VERSION_PROD	""
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
