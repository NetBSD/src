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
  * Global library.
  */
#include <bounce_log.h>

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

 /*
  * bounce_notify_util.c
  */
typedef struct {
    const char *service;		/* bounce or defer */
    const char *queue_name;		/* incoming, etc. */
    const char *queue_id;		/* base name */
    const char *mime_boundary;		/* for MIME */
    int     flush;			/* 0=defer, other=bounce */
    VSTRING *buf;			/* scratch pad */
    VSTREAM *orig_fp;			/* open queue file */
    long    orig_offs;			/* start of content */
    time_t  arrival_time;		/* time of arrival */
    BOUNCE_LOG *log_handle;			/* open logfile */
} BOUNCE_INFO;

extern BOUNCE_INFO *bounce_mail_init(const char *, const char *, const char *, int);
extern void bounce_mail_free(BOUNCE_INFO *);
extern int bounce_header(VSTREAM *, BOUNCE_INFO *, const char *);
extern int bounce_boilerplate(VSTREAM *, BOUNCE_INFO *);
extern int bounce_recipient_log(VSTREAM *, BOUNCE_INFO *);
extern int bounce_diagnostic_log(VSTREAM *, BOUNCE_INFO *);
extern int bounce_header_dsn(VSTREAM *, BOUNCE_INFO *);
extern int bounce_recipient_dsn(VSTREAM *, BOUNCE_INFO *);
extern int bounce_diagnostic_dsn(VSTREAM *, BOUNCE_INFO *);
extern int bounce_original(VSTREAM *, BOUNCE_INFO *, int);

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
