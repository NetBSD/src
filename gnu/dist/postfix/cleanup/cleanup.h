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

 /*
  * Global library.
  */
#include <maps.h>
#include <tok822.h>
#include <been_here.h>
#include <mail_stream.h>

 /*
  * These state variables are accessed by many functions, and there is only
  * one instance of each. Rather than passing around lots and lots of
  * parameters, or passing them around as part of a huge structure, we just
  * make the variables global, because that is what they are.
  */
extern VSTRING *cleanup_inbuf;		/* read buffer */
extern VSTRING *cleanup_temp1;		/* scratch buffer, local use only */
extern VSTRING *cleanup_temp2;		/* scratch buffer, local use only */
extern VSTREAM *cleanup_src;		/* current input stream */
extern VSTREAM *cleanup_dst;		/* current output stream */
extern MAIL_STREAM *cleanup_handle;	/* mail stream handle */
extern char *cleanup_queue_id;		/* queue file basename */
extern time_t cleanup_time;		/* posting time */
extern char *cleanup_fullname;		/* envelope sender full name */
extern char *cleanup_sender;		/* envelope sender address */
extern char *cleanup_from;		/* From: address */
extern char *cleanup_resent_from;	/* Resent-From: address */
extern char *cleanup_recip;		/* envelope recipient address */
extern char *cleanup_return_receipt;	/* return-receipt address */
extern char *cleanup_errors_to;		/* errors-to address */
extern int cleanup_flags;		/* processing options */
extern int cleanup_errs;		/* any badness experienced */
extern int cleanup_err_mask;		/* allowed badness */
extern VSTRING *cleanup_header_buf;	/* multi-record header */
extern int cleanup_headers_seen;	/* which headers were seen */
extern int cleanup_hop_count;		/* count of received: headers */
extern ARGV *cleanup_recipients;	/* recipients from regular headers */
extern ARGV *cleanup_resent_recip;	/* recipients from resent headers */
extern char *cleanup_resent;		/* any resent- header seen */
extern BH_TABLE *cleanup_dups;		/* recipient dup filter */

 /*
  * Mappings.
  */
extern MAPS *cleanup_comm_canon_maps;
extern MAPS *cleanup_send_canon_maps;
extern MAPS *cleanup_rcpt_canon_maps;
extern MAPS *cleanup_header_checks;
extern MAPS *cleanup_body_checks;
extern MAPS *cleanup_virtual_maps;
extern ARGV *cleanup_masq_domains;

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
extern void cleanup_state_alloc(void);
extern void cleanup_state_free(void);

 /*
  * cleanup_out.c
  */
extern void cleanup_out(int, char *, int);
extern void cleanup_out_string(int, char *);
extern void cleanup_out_format(int, char *,...);

#define CLEANUP_OUT_BUF(t, b) \
	cleanup_out((t), vstring_str((b)), VSTRING_LEN((b)))

#define CLEANUP_OUT_OK() \
	((cleanup_errs & cleanup_err_mask) == 0)

 /*
  * cleanup_envelope.c
  */
extern void cleanup_envelope(void);

 /*
  * cleanup_message.c
  */
extern void cleanup_message(void);

 /*
  * cleanup_extracted.c
  */
extern void cleanup_extracted(void);

 /*
  * cleanup_skip.o
  */
extern void cleanup_skip(void);

 /*
  * cleanup_rewrite.c
  */
extern void cleanup_rewrite_external(VSTRING *, const char *);
extern void cleanup_rewrite_internal(VSTRING *, const char *);
extern void cleanup_rewrite_tree(TOK822 *);

 /*
  * cleanup_map11.c
  */
extern void cleanup_map11_external(VSTRING *, MAPS *, int);
extern void cleanup_map11_internal(VSTRING *, MAPS *, int);
extern void cleanup_map11_tree(TOK822 *, MAPS *, int);

 /*
  * cleanup_map1n.c
  */
ARGV   *cleanup_map1n_internal(char *, MAPS *, int);

 /*
  * cleanup_masquerade.c
  */
extern void cleanup_masquerade_external(VSTRING *, ARGV *);
extern void cleanup_masquerade_internal(VSTRING *, ARGV *);
extern void cleanup_masquerade_tree(TOK822 *, ARGV *);

 /*
  * Cleanup_recipient.c
  */
extern void cleanup_out_recipient(char *);

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
