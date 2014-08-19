/*	$NetBSD: cleanup.h,v 1.4.10.2 2014/08/19 23:59:42 tls Exp $	*/

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
  * System library.
  */
#include <sys/time.h>

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
#include <cleanup_user.h>
#include <header_body_checks.h>
#include <dsn_mask.h>

 /*
  * Milter library.
  */
#include <milter.h>

 /*
  * These state variables are accessed by many functions, and there is only
  * one instance of each per message.
  */
typedef struct CLEANUP_STATE {
    VSTRING *attr_buf;			/* storage for named attribute */
    VSTRING *temp1;			/* scratch buffer, local use only */
    VSTRING *temp2;			/* scratch buffer, local use only */
    VSTRING *stripped_buf;		/* character stripped input */
    VSTREAM *src;			/* current input stream */
    VSTREAM *dst;			/* current output stream */
    MAIL_STREAM *handle;		/* mail stream handle */
    char   *queue_name;			/* queue name */
    char   *queue_id;			/* queue file basename */
    struct timeval arrival_time;	/* arrival time */
    char   *fullname;			/* envelope sender full name */
    char   *sender;			/* envelope sender address */
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
    void    (*action) (struct CLEANUP_STATE *, int, const char *, ssize_t);
    off_t   data_offset;		/* start of message content */
    off_t   body_offset;		/* start of body content */
    off_t   xtra_offset;		/* start of extra segment */
    off_t   cont_length;		/* length including Milter edits */
    off_t   sender_pt_offset;		/* replace sender here */
    off_t   sender_pt_target;		/* record after sender address */
    off_t   append_rcpt_pt_offset;	/* append recipient here */
    off_t   append_rcpt_pt_target;	/* target of above record */
    off_t   append_hdr_pt_offset;	/* append header here */
    off_t   append_hdr_pt_target;	/* target of above record */
    off_t   append_meta_pt_offset;	/* append meta record here */
    off_t   append_meta_pt_target;	/* target of above record */
    ssize_t rcpt_count;			/* recipient count */
    char   *reason;			/* failure reason */
    char   *smtp_reply;			/* failure reason, SMTP-style */
    NVTABLE *attr;			/* queue file attribute list */
    MIME_STATE *mime_state;		/* MIME state engine */
    int     mime_errs;			/* MIME error flags */
    char   *hdr_rewrite_context;	/* header rewrite context */
    char   *filter;			/* from header/body patterns */
    char   *redirect;			/* from header/body patterns */
    char   *dsn_envid;			/* DSN envelope ID */
    int     dsn_ret;			/* DSN full/hdrs */
    int     dsn_notify;			/* DSN never/delay/fail/success */
    char   *dsn_orcpt;			/* DSN original recipient */
    char   *verp_delims;		/* VERP delimiters (optional) */
#ifdef DELAY_ACTION
    int     defer_delay;		/* deferred delivery */
#endif

    /*
     * Miscellaneous Milter support.
     */
    MILTERS *milters;			/* mail filters */
    const char *client_name;		/* real or ersatz client */
    const char *reverse_name;		/* real or ersatz client */
    const char *client_addr;		/* real or ersatz client */
    int     client_af;			/* real or ersatz client */
    const char *client_port;		/* real or ersatz client */
    VSTRING *milter_ext_from;		/* externalized sender */
    VSTRING *milter_ext_rcpt;		/* externalized recipient */
    VSTRING *milter_err_text;		/* milter call-back reply */
    HBC_CHECKS *milter_hbc_checks;	/* Milter header checks */
    VSTRING *milter_hbc_reply;		/* Milter header checks reply */
    VSTRING *milter_orcpt_buf;		/* add_rcpt_par() orcpt */

    /*
     * Support for Milter body replacement requests.
     */
    struct CLEANUP_REGION *free_regions;/* unused regions */
    struct CLEANUP_REGION *body_regions;/* regions with body content */
    struct CLEANUP_REGION *curr_body_region;
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
extern int cleanup_comm_canon_flags;
extern int cleanup_send_canon_flags;
extern int cleanup_rcpt_canon_flags;
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
  * Character filters.
  */
extern VSTRING *cleanup_reject_chars;
extern VSTRING *cleanup_strip_chars;

 /*
  * Milters.
  */
extern MILTERS *cleanup_milters;

 /*
  * Address canonicalization fine control.
  */
#define CLEANUP_CANON_FLAG_ENV_FROM	(1<<0)	/* envelope sender */
#define CLEANUP_CANON_FLAG_ENV_RCPT	(1<<1)	/* envelope recipient */
#define CLEANUP_CANON_FLAG_HDR_FROM	(1<<2)	/* header sender */
#define CLEANUP_CANON_FLAG_HDR_RCPT	(1<<3)	/* header recipient */

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
  * Saved queue file names, so the files can be removed in case of a fatal
  * run-time error.
  */
extern char *cleanup_path;
extern VSTRING *cleanup_trace_path;
extern VSTRING *cleanup_bounce_path;

 /*
  * cleanup_state.c
  */
extern CLEANUP_STATE *cleanup_state_alloc(VSTREAM *);
extern void cleanup_state_free(CLEANUP_STATE *);

 /*
  * cleanup_api.c
  */
extern CLEANUP_STATE *cleanup_open(VSTREAM *);
extern void cleanup_control(CLEANUP_STATE *, int);
extern int cleanup_flush(CLEANUP_STATE *);
extern void cleanup_free(CLEANUP_STATE *);
extern void cleanup_all(void);
extern void cleanup_sig(int);
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
extern void cleanup_out(CLEANUP_STATE *, int, const char *, ssize_t);
extern void cleanup_out_string(CLEANUP_STATE *, int, const char *);
extern void PRINTFLIKE(3, 4) cleanup_out_format(CLEANUP_STATE *, int, const char *,...);
extern void cleanup_out_header(CLEANUP_STATE *, VSTRING *);

#define CLEANUP_OUT_BUF(s, t, b) \
	cleanup_out((s), (t), vstring_str((b)), VSTRING_LEN((b)))

#define CLEANUP_OUT_OK(s) \
	(!((s)->errs & (s)->err_mask) && !((s)->flags & CLEANUP_FLAG_DISCARD))

 /*
  * cleanup_envelope.c
  */
extern void cleanup_envelope(CLEANUP_STATE *, int, const char *, ssize_t);

 /*
  * cleanup_message.c
  */
extern void cleanup_message(CLEANUP_STATE *, int, const char *, ssize_t);

 /*
  * cleanup_extracted.c
  */
extern void cleanup_extracted(CLEANUP_STATE *, int, const char *, ssize_t);

 /*
  * cleanup_final.c
  */
extern void cleanup_final(CLEANUP_STATE *);

 /*
  * cleanup_rewrite.c
  */
extern int cleanup_rewrite_external(const char *, VSTRING *, const char *);
extern int cleanup_rewrite_internal(const char *, VSTRING *, const char *);
extern int cleanup_rewrite_tree(const char *, TOK822 *);

 /*
  * cleanup_map11.c
  */
extern int cleanup_map11_external(CLEANUP_STATE *, VSTRING *, MAPS *, int);
extern int cleanup_map11_internal(CLEANUP_STATE *, VSTRING *, MAPS *, int);
extern int cleanup_map11_tree(CLEANUP_STATE *, TOK822 *, MAPS *, int);

 /*
  * cleanup_map1n.c
  */
ARGV   *cleanup_map1n_internal(CLEANUP_STATE *, const char *, MAPS *, int);

 /*
  * cleanup_masquerade.c
  */
extern int cleanup_masquerade_external(CLEANUP_STATE *, VSTRING *, ARGV *);
extern int cleanup_masquerade_internal(CLEANUP_STATE *, VSTRING *, ARGV *);
extern int cleanup_masquerade_tree(CLEANUP_STATE *, TOK822 *, ARGV *);

 /*
  * cleanup_recipient.c
  */
extern void cleanup_out_recipient(CLEANUP_STATE *, const char *, int, const char *, const char *);

 /*
  * cleanup_addr.c.
  */
extern void cleanup_addr_sender(CLEANUP_STATE *, const char *);
extern void cleanup_addr_recipient(CLEANUP_STATE *, const char *);
extern void cleanup_addr_bcc_dsn(CLEANUP_STATE *, const char *, const char *, int);

#define NO_DSN_ORCPT	((char *) 0)
#define NO_DSN_NOTIFY	DSN_NOTIFY_NEVER
#define DEF_DSN_NOTIFY	(0)

#define cleanup_addr_bcc(state, addr) \
    cleanup_addr_bcc_dsn((state), (addr), NO_DSN_ORCPT, NO_DSN_NOTIFY)

 /*
  * cleanup_bounce.c.
  */
extern int cleanup_bounce(CLEANUP_STATE *);

 /*
  * MSG_STATS compatibility.
  */
#define CLEANUP_MSG_STATS(stats, state) \
    MSG_STATS_INIT1(stats, incoming_arrival, state->arrival_time)

 /*
  * cleanup_milter.c.
  */
extern void cleanup_milter_receive(CLEANUP_STATE *, int);
extern void cleanup_milter_inspect(CLEANUP_STATE *, MILTERS *);
extern void cleanup_milter_emul_mail(CLEANUP_STATE *, MILTERS *, const char *);
extern void cleanup_milter_emul_rcpt(CLEANUP_STATE *, MILTERS *, const char *);
extern void cleanup_milter_emul_data(CLEANUP_STATE *, MILTERS *);

#define CLEANUP_MILTER_OK(s) \
    (((s)->flags & CLEANUP_FLAG_MILTER) != 0 \
	&& (s)->errs == 0 && ((s)->flags & CLEANUP_FLAG_DISCARD) == 0)

 /*
  * cleanup_body_edit.c
  */
typedef struct CLEANUP_REGION {
    off_t   start;			/* start of region */
    off_t   len;			/* length or zero (open-ended) */
    off_t   write_offs;			/* write offset */
    struct CLEANUP_REGION *next;	/* linkage */
} CLEANUP_REGION;

extern void cleanup_region_init(CLEANUP_STATE *);
extern CLEANUP_REGION *cleanup_region_open(CLEANUP_STATE *, ssize_t);
extern void cleanup_region_close(CLEANUP_STATE *, CLEANUP_REGION *);
extern CLEANUP_REGION *cleanup_region_return(CLEANUP_STATE *, CLEANUP_REGION *);
extern void cleanup_region_done(CLEANUP_STATE *);

extern int cleanup_body_edit_start(CLEANUP_STATE *);
extern int cleanup_body_edit_write(CLEANUP_STATE *, int, VSTRING *);
extern int cleanup_body_edit_finish(CLEANUP_STATE *);
extern void cleanup_body_edit_free(CLEANUP_STATE *);

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
