/*++
/* NAME
/*	bounce_service 3h
/* SUMMARY
/*	bounce message service
/* SYNOPSIS
/*	#include <bounce_service.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <vstring.h>

 /*
  * bounce_append_service.c
  */
extern int bounce_append_service(char *, char *, char *, char *);

 /*
  * bounce_notify_service.c
  */
extern int bounce_notify_service(char *, char *, char *, char *, int);

 /*
  * bounce_cleanup.c
  */
extern VSTRING *bounce_cleanup_path;
extern void bounce_cleanup_register(char *, char *);
extern void bounce_cleanup_log(void);
extern void bounce_cleanup_unregister(void);

#define bounce_cleanup_registered() (bounce_cleanup_path != 0)

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
