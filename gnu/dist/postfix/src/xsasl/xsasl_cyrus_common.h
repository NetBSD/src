#ifndef _CYRUS_COMMON_H_INCLUDED_
#define _CYRUS_COMMON_H_INCLUDED_

/*++
/* NAME
/*	cyrus_common 3h
/* SUMMARY
/*	Cyrus SASL plug-in helpers
/* SYNOPSIS
/*	#include <cyrus_common.h>
/* DESCRIPTION
/* .nf

 /*
  * External interface.
  */
#if defined(USE_SASL_AUTH) && defined(USE_CYRUS_SASL)

#define NO_SASL_LANGLIST	((const char *) 0)
#define NO_SASL_OUTLANG		((const char **) 0)
#define xsasl_cyrus_strerror(status) \
	sasl_errstring((status), NO_SASL_LANGLIST, NO_SASL_OUTLANG)
extern int xsasl_cyrus_log(void *, int, const char *);
extern int xsasl_cyrus_security_parse_opts(const char *);

#endif

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
