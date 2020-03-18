/*	$NetBSD: info_log_addr_form.h,v 1.2 2020/03/18 19:05:16 christos Exp $	*/

#ifndef _INFO_LOG_ADDR_FORM_H_INCLUDED_
#define _INFO_LOG_ADDR_FORM_H_INCLUDED_

/*++
/* NAME
/*	info_log_addr_form 3h
/* SUMMARY
/*	format mail address for info logging
/* SYNOPSIS
/*	#include <info_log_addr_form.h>
/* DESCRIPTION
/* .nf

 /*
  * External interface.
  */
extern const char *info_log_addr_form_recipient(const char *);
extern const char *info_log_addr_form_sender(const char *);

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
