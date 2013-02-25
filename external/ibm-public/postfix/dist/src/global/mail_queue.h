/*	$NetBSD: mail_queue.h,v 1.1.1.2.12.1 2013/02/25 00:27:18 tls Exp $	*/

#ifndef _MAIL_QUEUE_H_INCLUDED_
#define _MAIL_QUEUE_H_INCLUDED_

/*++
/* NAME
/*	mail_queue 3h
/* SUMMARY
/*	mail queue access
/* SYNOPSIS
/*	#include <mail_queue.h>
/* DESCRIPTION
/* .nf

 /*
  * System library.
  */
#include <sys/time.h>

 /*
  * Utility library.
  */
#include <vstring.h>
#include <vstream.h>

 /*
  * Mail queue names.
  */
#define MAIL_QUEUE_MAILDROP	"maildrop"
#define MAIL_QUEUE_HOLD		"hold"
#define MAIL_QUEUE_INCOMING	"incoming"
#define MAIL_QUEUE_ACTIVE	"active"
#define MAIL_QUEUE_DEFERRED	"deferred"
#define MAIL_QUEUE_TRACE	"trace"
#define MAIL_QUEUE_DEFER	"defer"
#define MAIL_QUEUE_BOUNCE	"bounce"
#define MAIL_QUEUE_CORRUPT	"corrupt"
#define MAIL_QUEUE_FLUSH	"flush"
#define MAIL_QUEUE_SAVED	"saved"

 /*
  * Queue file modes.
  * 
  * 4.4BSD-like systems don't allow (sticky AND executable) together, so we use
  * group read permission bits instead. These are more portable, but they
  * also are more likely to be turned on by accident. It would not be the end
  * of the world.
  */
#define MAIL_QUEUE_STAT_READY	(S_IRUSR | S_IWUSR | S_IXUSR)
#define MAIL_QUEUE_STAT_CORRUPT	(S_IRUSR)
#ifndef MAIL_QUEUE_STAT_UNTHROTTLE
#define MAIL_QUEUE_STAT_UNTHROTTLE (S_IRGRP)
#endif

extern struct VSTREAM *mail_queue_enter(const char *, mode_t, struct timeval *);
extern struct VSTREAM *mail_queue_open(const char *, const char *, int, mode_t);
extern int mail_queue_rename(const char *, const char *, const char *);
extern int mail_queue_remove(const char *, const char *);
extern const char *mail_queue_dir(VSTRING *, const char *, const char *);
extern const char *mail_queue_path(VSTRING *, const char *, const char *);
extern int mail_queue_mkdirs(const char *);
extern int mail_queue_name_ok(const char *);
extern int mail_queue_id_ok(const char *);

 /*
  * MQID - Mail Queue ID format definitions. Needed only by code that creates
  * or parses queue ID strings.
  */
#ifdef MAIL_QUEUE_INTERNAL

 /*
  * System library.
  */
#include <errno.h>

 /*
  * Global library.
  */
#include <safe_ultostr.h>

 /*
  * The long non-repeating queue ID is encoded in an alphabet of 10 digits,
  * 21 upper-case characters, and 21 or fewer lower-case characters. The
  * alphabet is made "safe" by removing all the vowels (AEIOUaeiou). The ID
  * is the concatenation of:
  * 
  * - the time in seconds (base 52 encoded, six or more chars),
  * 
  * - the time in microseconds (base 52 encoded, exactly four chars),
  * 
  * - the 'z' character to separate the time and inode information,
  * 
  * - the inode number (base 51 encoded so that it contains no 'z').
  */
#define MQID_LG_SEC_BASE	52	/* seconds safe alphabet base */
#define MQID_LG_SEC_PAD	6	/* seconds minumum field width */
#define MQID_LG_USEC_BASE	52	/* microseconds safe alphabet base */
#define MQID_LG_USEC_PAD	4	/* microseconds exact field width */
#define MQID_LG_TIME_PAD	(MQID_LG_SEC_PAD + MQID_LG_USEC_PAD)
#define MQID_LG_INUM_SEP	'z'	/* time-inode separator */
#define MQID_LG_INUM_BASE	51	/* inode safe alphabet base */
#define MQID_LG_INUM_PAD	0	/* no padding needed */

#define MQID_FIND_LG_INUM_SEPARATOR(cp, path) \
	(((cp) = strrchr((path), MQID_LG_INUM_SEP)) != 0 \
	    && ((cp) - (path) >= MQID_LG_TIME_PAD))

#define MQID_GET_INUM(path, inum, long_form, error) do { \
	char *_cp; \
	if (((long_form) = MQID_FIND_LG_INUM_SEPARATOR(_cp, (path))) != 0) { \
	    MQID_LG_DECODE_INUM(_cp + 1, (inum), (error)); \
	} else { \
	    MQID_SH_DECODE_INUM((path) + MQID_SH_USEC_PAD, (inum), (error)); \
	} \
    } while (0)

#define MQID_LG_ENCODE_SEC(buf, val) \
	MQID_LG_ENCODE((buf), (val), MQID_LG_SEC_BASE, MQID_LG_SEC_PAD)

#define MQID_LG_ENCODE_USEC(buf, val) \
	MQID_LG_ENCODE((buf), (val), MQID_LG_USEC_BASE, MQID_LG_USEC_PAD)

#define MQID_LG_ENCODE_INUM(buf, val) \
	MQID_LG_ENCODE((buf), (val), MQID_LG_INUM_BASE, MQID_LG_INUM_PAD)

#define MQID_LG_DECODE_USEC(str, ulval, error) \
	MQID_LG_DECODE((str), (ulval), MQID_LG_USEC_BASE, (error))

#define MQID_LG_DECODE_INUM(str, ulval, error) \
	MQID_LG_DECODE((str), (ulval), MQID_LG_INUM_BASE, (error))

#define MQID_LG_ENCODE(buf, val, base, padlen) \
	safe_ultostr((buf), (unsigned long) (val), (base), (padlen), '0')

#define MQID_LG_DECODE(str, ulval, base, error) do { \
	char *_end; \
	errno = 0; \
	(ulval) = safe_strtoul((str), &_end, (base)); \
	(error) = (*_end != 0 || ((ulval) == ULONG_MAX && errno == ERANGE)); \
    } while (0)

#define MQID_LG_GET_HEX_USEC(bp, zp) do { \
	int _error; \
	unsigned long _us_val; \
	vstring_strncpy((bp), (zp) - MQID_LG_USEC_PAD, MQID_LG_USEC_PAD); \
	MQID_LG_DECODE_USEC(STR(bp), _us_val, _error); \
	(void) MQID_SH_ENCODE_USEC((bp), _us_val); \
    } while (0)

 /*
  * The short repeating queue ID is encoded in upper-case hexadecimal, and is
  * the concatenation of:
  * 
  * - the time in microseconds (exactly five chars),
  * 
  * - the inode number.
  */
#define MQID_SH_USEC_PAD	5	/* microseconds exact field width */

#define MQID_SH_ENCODE_USEC(buf, usec) \
	vstring_str(vstring_sprintf((buf), "%05X", (int) (usec)))

#define MQID_SH_ENCODE_INUM(buf, inum) \
	vstring_str(vstring_sprintf((buf), "%lX", (unsigned long) (inum)))

#define MQID_SH_DECODE_INUM(str, ulval, error) do { \
        char *_end; \
	errno = 0; \
	(ulval) = strtoul((str), &_end, 16); \
	(error) = (*_end != 0 || ((ulval) == ULONG_MAX && errno == ERANGE)); \
    } while (0)

#endif					/* MAIL_QUEUE_INTERNAL */

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

#endif					/* _MAIL_QUEUE_H_INCLUDED_ */
