#ifndef _MAIL_FLOW_H_INCLUDED_
#define _MAIL_FLOW_H_INCLUDED_

/*++
/* NAME
/*	mail_flow 3h
/* SUMMARY
/*	global mail flow control
/* SYNOPSIS
/*	#include <mail_flow.h>
/* DESCRIPTION
/* .nf

 /*
  * Functional interface.
  */
extern int mail_flow_get(int);
extern int mail_flow_put(int);
extern int mail_flow_count(void);

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
