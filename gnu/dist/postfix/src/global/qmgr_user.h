#ifndef _QMGR_USER_H_INCLUDED_
#define _QMGR_USER_H_INCLUDED_

/*++
/* NAME
/*	qmgr_user 3h
/* SUMMARY
/*	qmgr user interface codes
/* SYNOPSIS
/*	#include <qmgr_user.h>
/* DESCRIPTION
/* .nf

 /*
  * Queue file read options. Flags 16- are reserved by qmgr.h.
  */
#define QMGR_READ_FLAG_NONE		0	/* No special features */
#define QMGR_READ_FLAG_MIXED_RCPT_OTHER	(1<<0)	/* Mixed recipient/other */

#define QMGR_READ_FLAG_USER	(QMGR_READ_FLAG_MIXED_RCPT_OTHER)

 /*
  * Backwards compatibility.
  */
#define QMGR_READ_FLAG_DEFAULT	(QMGR_READ_FLAG_MIXED_RCPT_OTHER)

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
