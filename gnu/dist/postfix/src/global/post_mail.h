#ifndef _POST_MAIL_H_INCLUDED_
#define _POST_MAIL_H_INCLUDED_

/*++
/* NAME
/*	post_mail 3h
/* SUMMARY
/*	convenient mail posting interface
/* SYNOPSIS
/*	#include <post_mail.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <vstream.h>

 /*
  * Global library.
  */
#include <cleanup_user.h>

 /*
  * External interface.
  */
extern VSTREAM *post_mail_fopen(const char *, const char *, int);
extern VSTREAM *post_mail_fopen_nowait(const char *, const char *, int);
extern int PRINTFLIKE(2, 3) post_mail_fprintf(VSTREAM *, const char *,...);
extern int post_mail_fputs(VSTREAM *, const char *);
extern int post_mail_buffer(VSTREAM *, const char *, int);
extern int post_mail_fclose(VSTREAM *);

#define POST_MAIL_BUFFER(v, b) \
	post_mail_buffer((v), vstring_str(b), VSTRING_LEN(b))

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
