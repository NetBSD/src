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
extern int bounce_append_service(int, char *, char *, char *, char *, long, char *, char *, char *);

 /*
  * bounce_notify_service.c
  */
extern int bounce_notify_service(int, char *, char *, char *, char *, char *);

 /*
  * bounce_warn_service.c
  */
extern int bounce_warn_service(int, char *, char *, char *, char *, char *);

 /*
  * bounce_trace_service.c
  */
extern int bounce_trace_service(int, char *, char *, char *, char *, char *);

 /*
  * bounce_notify_verp.c
  */
extern int bounce_notify_verp(int, char *, char *, char *, char *, char *, char *);

 /*
  * bounce_one_service.c
  */
extern int bounce_one_service(int, char *, char *, char *, char *, char *, char *, long, char *, char *, char *);

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
    const char *mime_encoding;		/* null or encoding */
    const char *mime_boundary;		/* for MIME */
    int     flush;			/* 0=defer, other=bounce */
    VSTRING *buf;			/* scratch pad */
    VSTRING *sender;			/* envelope sender */
    VSTREAM *orig_fp;			/* open queue file */
    long    orig_offs;			/* start of content */
    time_t  arrival_time;		/* time of arrival */
    BOUNCE_LOG *log_handle;		/* open logfile */
    char   *mail_name;			/* $mail_name, cooked */
} BOUNCE_INFO;

extern BOUNCE_INFO *bounce_mail_init(const char *, const char *, const char *, const char *, int);
extern BOUNCE_INFO *bounce_mail_one_init(const char *, const char *, const char *, const char *, const char *, long, const char *, const char *, const char *);
extern void bounce_mail_free(BOUNCE_INFO *);
extern int bounce_header(VSTREAM *, BOUNCE_INFO *, const char *);
extern int bounce_boilerplate(VSTREAM *, BOUNCE_INFO *);
extern int bounce_recipient_log(VSTREAM *, BOUNCE_INFO *);
extern int bounce_diagnostic_log(VSTREAM *, BOUNCE_INFO *);
extern int bounce_header_dsn(VSTREAM *, BOUNCE_INFO *);
extern int bounce_recipient_dsn(VSTREAM *, BOUNCE_INFO *);
extern int bounce_diagnostic_dsn(VSTREAM *, BOUNCE_INFO *);
extern int bounce_original(VSTREAM *, BOUNCE_INFO *, int);
extern void bounce_delrcpt(BOUNCE_INFO *);
extern void bounce_delrcpt_one(BOUNCE_INFO *);

#define BOUNCE_MSG_FAIL		0
#define BOUNCE_MSG_WARN		1
#define BOUNCE_MSG_STATUS	2

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
