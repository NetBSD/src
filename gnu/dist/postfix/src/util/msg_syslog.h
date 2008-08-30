#ifndef _MSG_SYSLOG_H_INCLUDED_
#define _MSG_SYSLOG_H_INCLUDED_

/*++
/* NAME
/*	msg_syslog 3h
/* SUMMARY
/*	direct diagnostics to syslog daemon
/* SYNOPSIS
/*	#include <msg_syslog.h>
/* DESCRIPTION

 /*
  * System library.
  */
#include <syslog.h>

 /*
  * External interface.
  */
extern void msg_syslog_init(const char *, int, int);
extern int msg_syslog_facility(const char *);

/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*      Wietse Venema
/*      IBM T.J. Watson Research
/*      P.O. Box 704
/*      Yorktown Heights, NY 10598, USA
/*--*/

#endif
