/*++
/* NAME
/*	cleanup 3h
/* SUMMARY
/*	canonicalize and enqueue message
/* SYNOPSIS
/*	#include "cleanup.h"
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <vstring.h>
#include <vstream.h>
#include <argv.h>
#include <nvtable.h>

 /*
  * Global library.
  */
#include <maps.h>
#include <tok822.h>
#include <been_here.h>
#include <mail_stream.h>
#include <mail_conf.h>
#include <mime_state.h>
#include <string_list.h>

 /*
  * These state variables are accessed by many functions, and there is only
  * one instance of each per message.
  */
typedef struct CLEANUP_STATE {
    VSTRING *temp1;			/* scratch buffer, local use only */
    VSTRING *temp2;			/* scratch buffer, local use only */
    VSTREAM *dst;			/* current output stream */
    MAIL_STREAM *handle;		/* mail stream handle */
    char   *queue_name;			/* queue name */
    char   *queue_id;			/* queue file basename */
    time_t  time;			/* posting time */
    char   *fullname;			/* envelope sender full name */
    char   *sender;			/* envelope sender address */
    char   *from;			/* From: address */
    char   *resent_from;		/* Resent-From: address */
    char   *recip;			/* envelope recipient address */
    char   *orig_rcpt;			/* original recipient address */
    char   *return_receipt;		/* return-receipt address */
    char   *errors_to;			/* errors-to address */
    int     flags;			/* processing options, status flags */
    int     qmgr_opts;			/* qmgr processing options */
    int     errs;			/* any badness experienced */
    int     err_mask;			/* allowed badness */
    int     headers_seen;		/* which headers were seen */
    int     hop_count;			/* count of received: headers */
    char   *resent;			/* any resent- header seen */
    BH_TABLE *dups;			/* recipient dup filter */
    void    (*action) (struct CLEANUP_STATE *, int, const char *, int);
    off_t   data_offset;		/* start of message content */
    off_t   xtra_offset;		/* start of extra segment */
    int     rcpt_count;			/* recipient count */
    char   *reason;			/* failure reason */
    NVTABLE *attr;			/* queue file attribute list */
    MIME_STATE *mime_state;		/* MIME state engine */
    int     mime_errs;			/* MIME error flags */
    char   *filter;			/* from header/body patterns */
    char   *redirect;			/* from header/body patterns */
} CLEANUP_STATE;

 /*
  * Status flags. Flags 0-15 are reserved for cleanup_user.h.
  */
#define CLEANUP_FLAG_INRCPT	(1<<16)	/* Processing recipient records */
#define CLEANUP_FLAG_WARN_SEEN	(1<<17)	/* REC_TYPE_WARN record seen */
#define CLEANUP_FLAG_END_SEEN	(1<<18)	/* REC_TYPE_END record seen */

 /*
  * Mappings.
  */
extern MAPS *cleanup_comm_canon_maps;
extern MAPS *cleanup_send_canon_maps;
extern MAPS *cleanup_rcpt_canon_maps;
extern MAPS *cleanup_header_checks;
extern MAPS *cleanup_mimehdr_checks;
extern MAPS *cleanup_nesthdr_checks;
extern MAPS *cleanup_body_checks;
extern MAPS *cleanup_virt_alias_maps;
extern ARGV *cleanup_masq_domains;
extern STRING_LIST *cleanup_masq_exceptions;
extern int cleanup_masq_flags;
extern MAPS *cleanup_send_bcc_maps;
extern MAPS *cleanup_rcpt_bcc_maps;

 /*
  * Address masquerading fine control.
  */
#define CLEANUP_MASQ_FLAG_ENV_FROM	(1<<0)	/* envelope sender */
#define CLEANUP_MASQ_FLAG_ENV_RCPT	(1<<1)	/* envelope recipient */
#define CLEANUP_MASQ_FLAG_HDR_FROM	(1<<2)	/* header sender */
#define CLEANUP_MASQ_FLAG_HDR_RCPT	(1<<3)	/* header recipient */

 /*
  * Restrictions on extension propagation.
  */
extern int cleanup_ext_prop_mask;

 /*
  * Saved queue file name, so the file can be removed in case of a fatal
  * run-time error.
  */
extern char *cleanup_path;

 /*
  * cleanup_state.c
  */
extern CLEANUP_STATE *cleanup_state_alloc(void);
extern void cleanup_state_free(CLEANUP_STATE *);

 /*
  * cleanup_api.c
  */
extern CLEANUP_STATE *cleanup_open(void);
extern void cleanup_control(CLEANUP_STATE *, int);
extern int cleanup_flush(CLEANUP_STATE *);
extern void cleanup_free(CLEANUP_STATE *);
extern void cleanup_all(void);
extern void cleanup_pre_jail(char *, char **);
extern void cleanup_post_jail(char *, char **);
extern CONFIG_BOOL_TABLE cleanup_bool_table[];
extern CONFIG_INT_TABLE cleanup_int_table[];
extern CONFIG_BOOL_TABLE cleanup_bool_table[];
extern CONFIG_STR_TABLE cleanup_str_table[];
extern CONFIG_TIME_TABLE cleanup_time_table[];

#define CLEANUP_RECORD(s, t, b, l)	((s)->action((s), (t), (b), (l)))

 /*
  * cleanup_out.c
  */
extern void cleanup_out(CLEANUP_STATE *, int, const char *, int);
extern void cleanup_out_string(CLEANUP_STATE *, int, const char *);
extern void PRINTFLIKE(3, 4) cleanup_out_format(CLEANUP_STATE *, int, const char *,...);

#define CLEANUP_OUT_BUF(s, t, b) \
	cleanup_out((s), (t), vstring_str((b)), VSTRING_LEN((b)))

#define CLEANUP_OUT_OK(s) \
	(!((s)->errs & (s)->err_mask) && !((s)->flags & CLEANUP_FLAG_DISCARD))

 /*
  * cleanup_envelope.c
  */
extern void cleanup_envelope(CLEANUP_STATE *, int, const char *, int);

 /*
  * cleanup_message.c
  */
extern void cleanup_message(CLEANUP_STATE *, int, const char *, int);

 /*
  * cleanup_extracted.c
  */
extern void cleanup_extracted(CLEANUP_STATE *, int, const char *, int);

 /*
  * cleanup_rewrite.c
  */
extern void cleanup_rewrite_external(VSTRING *, const char *);
extern void cleanup_rewrite_internal(VSTRING *, const char *);
extern void cleanup_rewrite_tree(TOK822 *);

 /*
  * cleanup_map11.c
  */
extern void cleanup_map11_external(CLEANUP_STATE *, VSTRING *, MAPS *, int);
extern void cleanup_map11_internal(CLEANUP_STATE *, VSTRING *, MAPS *, int);
extern void cleanup_map11_tree(CLEANUP_STATE *, TOK822 *, MAPS *, int);

 /*
  * cleanup_map1n.c
  */
ARGV   *cleanup_map1n_internal(CLEANUP_STATE *, const char *, MAPS *, int);

 /*
  * cleanup_masquerade.c
  */
extern void cleanup_masquerade_external(VSTRING *, ARGV *);
extern void cleanup_masquerade_internal(VSTRING *, ARGV *);
extern void cleanup_masquerade_tree(TOK822 *, ARGV *);

 /*
  * cleanup_recipient.c
  */
extern void cleanup_out_recipient(CLEANUP_STATE *, const char *, const char *);

 /*
  * cleanup_addr.c.
  */
extern void cleanup_addr_sender(CLEANUP_STATE *, const char *);
extern void cleanup_addr_recipient(CLEANUP_STATE *, const char *);
extern void cleanup_addr_bcc(CLEANUP_STATE *, const char *);

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
