/*	$NetBSD: midna_domain.c,v 1.4 2020/05/25 23:47:14 christos Exp $	*/

/*++
/* NAME
/*	midna_domain 3
/* SUMMARY
/*	ASCII/UTF-8 domain name conversion
/* SYNOPSIS
/*	#include <midna_domain.h>
/*
/*	int midna_domain_cache_size;
/*	int midna_domain_transitional;
/*
/*	const char *midna_domain_to_ascii(
/*	const char *name)
/*
/*	const char *midna_domain_to_utf8(
/*	const char *name)
/*
/*	const char *midna_domain_suffix_to_ascii(
/*	const char *name)
/*
/*	const char *midna_domain_suffix_to_utf8(
/*	const char *name)
/* AUXILIARY FUNCTIONS
/*	void midna_domain_pre_chroot(void)
/* DESCRIPTION
/*	The functions in this module transform domain names from/to
/*	ASCII and UTF-8 form. The result is cached to avoid repeated
/*	conversion.
/*
/*	This module builds on the ICU library implementation of the
/*	UTS #46 specification, using default ICU library options
/*	because those are likely best tested: with transitional
/*	processing, with case mapping, with normalization, with
/*	limited IDNA2003 compatibility, without STD3 ASCII rules.
/*
/*	midna_domain_to_ascii() converts an UTF-8 or ASCII domain
/*	name to ASCII.  The result is a null pointer in case of
/*	error.  This function verifies that the result passes
/*	valid_hostname().
/*
/*	midna_domain_to_utf8() converts an UTF-8 or ASCII domain
/*	name to UTF-8.  The result is a null pointer in case of
/*	error.  This function verifies that the result, after
/*	conversion to ASCII, passes valid_hostname().
/*
/*	midna_domain_suffix_to_ascii() and midna_domain_suffix_to_utf8()
/*	take a name that starts with '.' and otherwise perform the
/*	same operations as midna_domain_to_ascii() and
/*	midna_domain_to_utf8().
/*
/*	midna_domain_cache_size specifies the size of the conversion
/*	result cache.  This value is used only once, upon the first
/*	lookup request.
/*
/*	midna_domain_transitional enables transitional conversion
/*	between UTF8 and ASCII labels.
/*
/*	midna_domain_pre_chroot() does some pre-chroot initialization.
/* SEE ALSO
/*	http://unicode.org/reports/tr46/ Unicode IDNA Compatibility processing
/*	msg(3) diagnostics interface
/* DIAGNOSTICS
/*	Fatal errors: memory allocation problem.
/*	Warnings: conversion error or result validation error.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Arnt Gulbrandsen
/*
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*
/*	Wietse Venema
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
/*--*/

 /*
  * System library.
  */
#include <sys_defs.h>
#include <string.h>
#include <ctype.h>

#ifndef NO_EAI
#include <unicode/uidna.h>

 /*
  * Utility library.
  */
#include <mymalloc.h>
#include <msg.h>
#include <ctable.h>
#include <stringops.h>
#include <valid_hostname.h>
#include <name_mask.h>
#include <midna_domain.h>

 /*
  * Application-specific.
  */
#define DEF_MIDNA_CACHE_SIZE	256

int     midna_domain_cache_size = DEF_MIDNA_CACHE_SIZE;
int     midna_domain_transitional = 0;
static VSTRING *midna_domain_buf;	/* x.suffix */

#define STR(x)	vstring_str(x)

/* midna_domain_strerror - pick one for error reporting */

static const char *midna_domain_strerror(UErrorCode error, int info_errors)
{

    /*
     * XXX The UIDNA_ERROR_EMPTY_LABEL etc. names are defined in an ENUM, so
     * we can't use #ifdef to dynamically determine which names exist.
     */
    static LONG_NAME_MASK uidna_errors[] = {
	"UIDNA_ERROR_EMPTY_LABEL", UIDNA_ERROR_EMPTY_LABEL,
	"UIDNA_ERROR_LABEL_TOO_LONG", UIDNA_ERROR_LABEL_TOO_LONG,
	"UIDNA_ERROR_DOMAIN_NAME_TOO_LONG", UIDNA_ERROR_DOMAIN_NAME_TOO_LONG,
	"UIDNA_ERROR_LEADING_HYPHEN", UIDNA_ERROR_LEADING_HYPHEN,
	"UIDNA_ERROR_TRAILING_HYPHEN", UIDNA_ERROR_TRAILING_HYPHEN,
	"UIDNA_ERROR_HYPHEN_3_4", UIDNA_ERROR_HYPHEN_3_4,
	"UIDNA_ERROR_LEADING_COMBINING_MARK", UIDNA_ERROR_LEADING_COMBINING_MARK,
	"UIDNA_ERROR_DISALLOWED", UIDNA_ERROR_DISALLOWED,
	"UIDNA_ERROR_PUNYCODE", UIDNA_ERROR_PUNYCODE,
	"UIDNA_ERROR_LABEL_HAS_DOT", UIDNA_ERROR_LABEL_HAS_DOT,
	"UIDNA_ERROR_INVALID_ACE_LABEL", UIDNA_ERROR_INVALID_ACE_LABEL,
	"UIDNA_ERROR_BIDI", UIDNA_ERROR_BIDI,
	"UIDNA_ERROR_CONTEXTJ", UIDNA_ERROR_CONTEXTJ,
	/* The above errors are defined with ICU 46 and later. */
	0,
    };

    if (info_errors) {
	return (str_long_name_mask_opt((VSTRING *) 0, "idna error",
				       uidna_errors, info_errors,
				       NAME_MASK_NUMBER | NAME_MASK_COMMA));
    } else {
	return u_errorName(error);
    }
}

/* midna_domain_pre_chroot - pre-chroot initialization */

void    midna_domain_pre_chroot(void)
{
    UErrorCode error = U_ZERO_ERROR;
    UIDNAInfo info = UIDNA_INFO_INITIALIZER;
    UIDNA  *idna;

    idna = uidna_openUTS46(midna_domain_transitional ? UIDNA_DEFAULT
			   : UIDNA_NONTRANSITIONAL_TO_ASCII, &error);
    if (U_FAILURE(error))
	msg_warn("ICU library initialization failed: %s",
		 midna_domain_strerror(error, info.errors));
    uidna_close(idna);
}

/* midna_domain_to_ascii_create - convert domain to ASCII */

static void *midna_domain_to_ascii_create(const char *name, void *unused_context)
{
    static const char myname[] = "midna_domain_to_ascii_create";
    char    buf[1024];			/* XXX */
    UErrorCode error = U_ZERO_ERROR;
    UIDNAInfo info = UIDNA_INFO_INITIALIZER;
    UIDNA  *idna;
    int     anl;

    /*
     * Paranoia: do not expose uidna_*() to unfiltered network data.
     */
    if (allascii(name) == 0 && valid_utf8_string(name, strlen(name)) == 0) {
	msg_warn("%s: Problem translating domain \"%.100s\" to ASCII form: %s",
		 myname, name, "malformed UTF-8");
	return (0);
    }

    /*
     * Perform the requested conversion.
     */
    idna = uidna_openUTS46(midna_domain_transitional ? UIDNA_DEFAULT
			   : UIDNA_NONTRANSITIONAL_TO_ASCII, &error);
    anl = uidna_nameToASCII_UTF8(idna,
				 name, strlen(name),
				 buf, sizeof(buf) - 1,
				 &info,
				 &error);
    uidna_close(idna);

    /*
     * Paranoia: verify that the result passes valid_hostname(). A quick
     * check shows that UTS46 ToASCII by default rejects inputs with labels
     * that start or end in '-', with names or labels that are over-long, or
     * "fake" A-labels, as required by UTS 46 section 4.1, but we rely on
     * valid_hostname() on the output side just to be sure.
     */
    if (U_SUCCESS(error) && info.errors == 0 && anl > 0) {
	buf[anl] = 0;				/* XXX */
	if (!valid_hostname(buf, DONT_GRIPE)) {
	    msg_warn("%s: Problem translating domain \"%.100s\" to ASCII form: %s",
		     myname, name, "malformed ASCII label(s)");
	    return (0);
	}
	return (mystrndup(buf, anl));
    } else {
	msg_warn("%s: Problem translating domain \"%.100s\" to ASCII form: %s",
		 myname, name, midna_domain_strerror(error, info.errors));
	return (0);
    }
}

/* midna_domain_to_utf8_create - convert domain to UTF8 */

static void *midna_domain_to_utf8_create(const char *name, void *unused_context)
{
    static const char myname[] = "midna_domain_to_utf8_create";
    char    buf[1024];			/* XXX */
    UErrorCode error = U_ZERO_ERROR;
    UIDNAInfo info = UIDNA_INFO_INITIALIZER;
    UIDNA  *idna;
    int     anl;

    /*
     * Paranoia: do not expose uidna_*() to unfiltered network data.
     */
    if (allascii(name) == 0 && valid_utf8_string(name, strlen(name)) == 0) {
	msg_warn("%s: Problem translating domain \"%.100s\" to UTF-8 form: %s",
		 myname, name, "malformed UTF-8");
	return (0);
    }

    /*
     * Perform the requested conversion.
     */
    idna = uidna_openUTS46(midna_domain_transitional ? UIDNA_DEFAULT
			   : UIDNA_NONTRANSITIONAL_TO_UNICODE, &error);
    anl = uidna_nameToUnicodeUTF8(idna,
				  name, strlen(name),
				  buf, sizeof(buf) - 1,
				  &info,
				  &error);
    uidna_close(idna);

    /*
     * Paranoia: UTS46 toUTF8 by default accepts and produces an over-long
     * name or a name that contains an over-long NR-LDH label (and perhaps
     * other invalid forms that are not covered in UTS 46, section 4.1). We
     * rely on midna_domain_to_ascii() to validate the output.
     */
    if (U_SUCCESS(error) && info.errors == 0 && anl > 0) {
	buf[anl] = 0;				/* XXX */
	if (midna_domain_to_ascii(buf) == 0)
	    return (0);
	return (mystrndup(buf, anl));
    } else {
	msg_warn("%s: Problem translating domain \"%.100s\" to UTF8 form: %s",
		 myname, name, midna_domain_strerror(error, info.errors));
	return (0);
    }
}

/* midna_domain_cache_free - cache element destructor */

static void midna_domain_cache_free(void *value, void *unused_context)
{
    if (value)
	myfree(value);
}

/* midna_domain_to_ascii - convert name to ASCII */

const char *midna_domain_to_ascii(const char *name)
{
    static CTABLE *midna_domain_to_ascii_cache = 0;

    if (midna_domain_to_ascii_cache == 0)
	midna_domain_to_ascii_cache = ctable_create(midna_domain_cache_size,
					       midna_domain_to_ascii_create,
						    midna_domain_cache_free,
						    (void *) 0);
    return (ctable_locate(midna_domain_to_ascii_cache, name));
}

/* midna_domain_to_utf8 - convert name to UTF8 */

const char *midna_domain_to_utf8(const char *name)
{
    static CTABLE *midna_domain_to_utf8_cache = 0;

    if (midna_domain_to_utf8_cache == 0)
	midna_domain_to_utf8_cache = ctable_create(midna_domain_cache_size,
						midna_domain_to_utf8_create,
						   midna_domain_cache_free,
						   (void *) 0);
    return (ctable_locate(midna_domain_to_utf8_cache, name));
}

/* midna_domain_suffix_to_ascii - convert .name to ASCII */

const char *midna_domain_suffix_to_ascii(const char *suffix)
{
    const char *cache_res;

    /*
     * If prepending x to .name causes the result to become too long, then
     * the suffix is bad.
     */
    if (midna_domain_buf == 0)
	midna_domain_buf = vstring_alloc(100);
    vstring_sprintf(midna_domain_buf, "x%s", suffix);
    if ((cache_res = midna_domain_to_ascii(STR(midna_domain_buf))) == 0)
	return (0);
    else
	return (cache_res + 1);
}

/* midna_domain_suffix_to_utf8 - convert .name to UTF8 */

const char *midna_domain_suffix_to_utf8(const char *name)
{
    const char *cache_res;

    /*
     * If prepending x to .name causes the result to become too long, then
     * the suffix is bad.
     */
    if (midna_domain_buf == 0)
	midna_domain_buf = vstring_alloc(100);
    vstring_sprintf(midna_domain_buf, "x%s", name);
    if ((cache_res = midna_domain_to_utf8(STR(midna_domain_buf))) == 0)
	return (0);
    else
	return (cache_res + 1);
}

#ifdef TEST

 /*
  * Test program - reads names from stdin, reports invalid names to stderr.
  */
#include <unistd.h>
#include <stdlib.h>
#include <locale.h>

#include <stringops.h>			/* XXX util_utf8_enable */
#include <vstring.h>
#include <vstream.h>
#include <vstring_vstream.h>
#include <msg_vstream.h>

int     main(int argc, char **argv)
{
    VSTRING *buffer = vstring_alloc(1);
    const char *bp;
    const char *ascii;
    const char *utf8;

    if (setlocale(LC_ALL, "C") == 0)
	msg_fatal("setlocale(LC_ALL, C) failed: %m");

    msg_vstream_init(argv[0], VSTREAM_ERR);
    /* msg_verbose = 1; */
    util_utf8_enable = 1;

    if (geteuid() == 0) {
	midna_domain_pre_chroot();
	if (chroot(".") != 0)
	    msg_fatal("chroot(\".\"): %m");
    }
    while (vstring_fgets_nonl(buffer, VSTREAM_IN)) {
	bp = STR(buffer);
	msg_info("> %s", bp);
	while (ISSPACE(*bp))
	    bp++;
	if (*bp == '#' || *bp == 0)
	    continue;
	msg_info("unconditional conversions:");
	utf8 = midna_domain_to_utf8(bp);
	msg_info("\"%s\" ->utf8 \"%s\"", bp, utf8 ? utf8 : "(error)");
	ascii = midna_domain_to_ascii(bp);
	msg_info("\"%s\" ->ascii \"%s\"", bp, ascii ? ascii : "(error)");
	msg_info("conditional conversions:");
	if (!allascii(bp)) {
	    if (ascii != 0) {
		utf8 = midna_domain_to_utf8(ascii);
		msg_info("\"%s\" ->ascii \"%s\" ->utf8 \"%s\"",
			 bp, ascii, utf8 ? utf8 : "(error)");
		if (utf8 != 0) {
		    if (strcmp(utf8, bp) != 0)
			msg_warn("\"%s\" != \"%s\"", bp, utf8);
		}
	    }
	} else {
	    if (utf8 != 0) {
		ascii = midna_domain_to_ascii(utf8);
		msg_info("\"%s\" ->utf8 \"%s\" ->ascii \"%s\"",
			 bp, utf8, ascii ? ascii : "(error)");
		if (ascii != 0) {
		    if (strcmp(ascii, bp) != 0)
			msg_warn("\"%s\" != \"%s\"", bp, ascii);
		}
	    }
	}
    }
    exit(0);
}

#endif					/* TEST */

#endif					/* NO_EAI */
