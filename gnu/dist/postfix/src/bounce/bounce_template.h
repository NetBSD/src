#ifndef _BOUNCE_TEMPLATE_H_INCLUDED_
#define _BOUNCE_TEMPLATE_H_INCLUDED_

/*++
/* NAME
/*	bounce_template 3h
/* SUMMARY
/*	bounce template support
/* SYNOPSIS
/*	#include <bounce_template.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <vstream.h>

 /*
  * Structure of a single bounce template. Each template is manipulated by
  * itself, without any external markers and delimiters. Applications are not
  * supposed to access BOUNCE_TEMPLATE attributes directly.
  */
typedef struct BOUNCE_TEMPLATE {
    int     flags;
    const char *class;			/* for diagnostics (fixed) */
    const char *origin;			/* built-in or pathname */
    const char *mime_charset;		/* character set (configurable) */
    const char *mime_encoding;		/* 7bit or 8bit (derived) */
    const char *from;			/* originator (configurable) */
    const char *subject;		/* general subject (configurable) */
    const char *postmaster_subject;	/* postmaster subject (configurable) */
    const char **message_text;		/* message text (configurable) */
    const struct BOUNCE_TEMPLATE *prototype;	/* defaults */
    char   *buffer;			/* ripped text */
} BOUNCE_TEMPLATE;

#define BOUNCE_TMPL_FLAG_NEW_BUFFER	(1<<0)

#define BOUNCE_TMPL_CLASS_FAILURE	"failure"
#define BOUNCE_TMPL_CLASS_DELAY		"delay"
#define BOUNCE_TMPL_CLASS_SUCCESS	"success"
#define BOUNCE_TMPL_CLASS_VERIFY	"verify"

#define IS_FAILURE_TEMPLATE(t)	((t)->class[0] == BOUNCE_TMPL_CLASS_FAILURE[0])
#define IS_DELAY_TEMPLATE(t)	((t)->class[0] == BOUNCE_TMPL_CLASS_DELAY[0])
#define IS_SUCCESS_TEMPLATE(t)	((t)->class[0] == BOUNCE_TMPL_CLASS_SUCCESS[0])
#define IS_VERIFY_TEMPLATE(t)	((t)->class[0] == BOUNCE_TMPL_CLASS_verify[0])

#define bounce_template_encoding(t)	((t)->mime_encoding)
#define bounce_template_charset(t)	((t)->mime_charset)

typedef int (*BOUNCE_XP_PRN_FN) (VSTREAM *, const char *, ...);
typedef int (*BOUNCE_XP_PUT_FN) (VSTREAM *, const char *);

extern BOUNCE_TEMPLATE *bounce_template_create(const BOUNCE_TEMPLATE *);
extern void bounce_template_free(BOUNCE_TEMPLATE *);
extern void bounce_template_load(BOUNCE_TEMPLATE *, const char *, const char *);
extern void bounce_template_headers(BOUNCE_XP_PRN_FN, VSTREAM *, BOUNCE_TEMPLATE *, const char *, int);
extern void bounce_template_expand(BOUNCE_XP_PUT_FN, VSTREAM *, BOUNCE_TEMPLATE *);
extern void bounce_template_dump(VSTREAM *, BOUNCE_TEMPLATE *);

#define POSTMASTER_COPY		1	/* postmaster copy */
#define NO_POSTMASTER_COPY	0	/* not postmaster copy */

 /*
  * Structure of a bounce template collection. These templates are read and
  * written in their external representation, with markers and delimiters.
  */
typedef struct {
    BOUNCE_TEMPLATE *failure;
    BOUNCE_TEMPLATE *delay;
    BOUNCE_TEMPLATE *success;
    BOUNCE_TEMPLATE *verify;
} BOUNCE_TEMPLATES;

BOUNCE_TEMPLATES *bounce_templates_create(void);
void    bounce_templates_free(BOUNCE_TEMPLATES *);
void    bounce_templates_load(VSTREAM *, BOUNCE_TEMPLATES *);
void    bounce_templates_expand(VSTREAM *, BOUNCE_TEMPLATES *);
void    bounce_templates_dump(VSTREAM *, BOUNCE_TEMPLATES *);

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
