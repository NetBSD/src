/*++
/* NAME
/*	local 3h
/* SUMMARY
/*	local mail delivery
/* SYNOPSIS
/*	#include "local.h"
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <htable.h>
#include <vstream.h>
#include <vstring.h>

 /*
  * Global library.
  */
#include <been_here.h>
#include <tok822.h>
#include <deliver_request.h>
#include <mbox_conf.h>
#include <maps.h>
#include <dsn_buf.h>
#include <dsn.h>
#include <delivered_hdr.h>

 /*
  * User attributes: these control the privileges for delivery to external
  * commands, external files, or mailboxes, and the initial environment of
  * external commands.
  */
typedef struct USER_ATTR {
    uid_t   uid;			/* file/command access */
    gid_t   gid;			/* file/command access */
    char   *home;			/* null or home directory */
    char   *logname;			/* null or login name */
    char   *shell;			/* null or login shell */
} USER_ATTR;

 /*
  * Critical macros. Not for obscurity, but to ensure consistency.
  */
#define RESET_USER_ATTR(usr_attr, level) { \
	usr_attr.uid = 0; usr_attr.gid = 0; usr_attr.home = 0; \
	usr_attr.logname = 0; usr_attr.shell = 0; \
	if (msg_verbose) \
	    msg_info("%s[%d]: reset user_attr", myname, level); \
    }

#define SET_USER_ATTR(usr_attr, pwd, level) { \
	usr_attr.uid = pwd->pw_uid; usr_attr.gid = pwd->pw_gid; \
	usr_attr.home = pwd->pw_dir; usr_attr.logname = pwd->pw_name; \
	usr_attr.shell = pwd->pw_shell; \
	if (msg_verbose) \
	    msg_info("%s[%d]: set user_attr: %s", \
		myname, level, pwd->pw_name); \
    }

 /*
  * The delivery attributes are inherited from files, from aliases, and from
  * whatnot. Some of the information is changed on the fly. DELIVER_ATTR
  * structures are therefore passed by value, so there is no need to undo
  * changes.
  */
typedef struct DELIVER_ATTR {
    int     level;			/* recursion level */
    VSTREAM *fp;			/* open queue file */
    char   *queue_name;			/* mail queue id */
    char   *queue_id;			/* mail queue id */
    long    offset;			/* data offset */
    char   *encoding;			/* MIME encoding */
    const char *sender;			/* taken from envelope */
    char   *dsn_envid;			/* DSN envelope ID */
    int     dsn_ret;			/* DSN headers/full */
    RECIPIENT rcpt;			/* from delivery request */
    char   *domain;			/* recipient domain */
    char   *local;			/* recipient full localpart */
    char   *user;			/* recipient localpart, base name */
    char   *extension;			/* recipient localpart, extension */
    char   *unmatched;			/* unmatched extension */
    const char *owner;			/* null or list owner */
    const char *delivered;		/* for loop detection */
    char   *relay;			/* relay host */
    MSG_STATS msg_stats;		/* time profile */
    int     exp_type;			/* expansion type. see below */
    char   *exp_from;			/* expanded_from */
    DELIVER_REQUEST *request;		/* the kitchen sink */
    DSN_BUF *why;			/* delivery status */
} DELIVER_ATTR;

extern void deliver_attr_init(DELIVER_ATTR *);
extern void deliver_attr_dump(DELIVER_ATTR *);
extern void deliver_attr_free(DELIVER_ATTR *);

#define EXPAND_TYPE_ALIAS	(1<<0)
#define EXPAND_TYPE_FWD		(1<<1)
#define EXPAND_TYPE_INCL	(1<<2)

 /*
  * Rather than schlepping around dozens of arguments, here is one that has
  * all. Well, almost. The user attributes are just a bit too sensitive, so
  * they are passed around separately.
  */
typedef struct LOCAL_STATE {
    int     level;			/* nesting level, for logging */
    DELIVER_ATTR msg_attr;		/* message attributes */
    BH_TABLE *dup_filter;		/* internal duplicate filter */
    DELIVERED_HDR_INFO *loop_info;	/* external loop filter */
    DELIVER_REQUEST *request;		/* as from queue manager */
} LOCAL_STATE;

#define RESET_OWNER_ATTR(msg_attr, level) { \
	msg_attr.owner = 0; \
	if (msg_verbose) \
	    msg_info("%s[%d]: reset owner attr", myname, level); \
    }

#define SET_OWNER_ATTR(msg_attr, who, level) { \
	msg_attr.sender = msg_attr.owner = who; \
	if (msg_verbose) \
	    msg_info("%s[%d]: set owner attr: %s", \
		    myname, level, who); \
    }

 /*
  * Bundle up some often-user attributes.
  */
#define BOUNCE_FLAGS(request)	DEL_REQ_TRACE_FLAGS((request)->flags)

#define BOUNCE_ATTR(attr) \
	attr.queue_id, &attr.msg_stats, &attr.rcpt, attr.relay, \
	DSN_FROM_DSN_BUF(attr.why)
#define BOUNCE_ONE_ATTR(attr) \
	attr.queue_name, attr.queue_id, attr.encoding, \
	attr.sender, attr.dsn_envid, attr.dsn_ret, \
	&attr.msg_stats, &attr.rcpt, attr.relay, \
	DSN_FROM_DSN_BUF(attr.why)
#define SENT_ATTR(attr) \
	attr.queue_id, &attr.msg_stats, &attr.rcpt, attr.relay, \
	DSN_FROM_DSN_BUF(attr.why)
#define OPENED_ATTR(attr) \
	attr.queue_id, attr.sender
#define COPY_ATTR(attr) \
	attr.sender, attr.rcpt.orig_addr, attr.delivered, attr.fp

#define MSG_LOG_STATE(m, p) \
	msg_info("%s[%d]: local %s recip %s exten %s deliver %s exp_from %s", \
		m, \
                p.level, \
		p.msg_attr.local ? p.msg_attr.local : "" , \
		p.msg_attr.rcpt.address ? p.msg_attr.rcpt.address : "", \
		p.msg_attr.extension ? p.msg_attr.extension : "", \
		p.msg_attr.delivered ? p.msg_attr.delivered : "", \
		p.msg_attr.exp_from ? p.msg_attr.exp_from : "")

 /*
  * "inner" nodes of the delivery graph.
  */
extern int deliver_recipient(LOCAL_STATE, USER_ATTR);
extern int deliver_alias(LOCAL_STATE, USER_ATTR, char *, int *);
extern int deliver_dotforward(LOCAL_STATE, USER_ATTR, int *);
extern int deliver_include(LOCAL_STATE, USER_ATTR, char *);
extern int deliver_token(LOCAL_STATE, USER_ATTR, TOK822 *);
extern int deliver_token_string(LOCAL_STATE, USER_ATTR, char *, int *);
extern int deliver_token_stream(LOCAL_STATE, USER_ATTR, VSTREAM *, int *);
extern int deliver_resolve_tree(LOCAL_STATE, USER_ATTR, TOK822 *);
extern int deliver_resolve_addr(LOCAL_STATE, USER_ATTR, char *);

 /*
  * "leaf" nodes of the delivery graph.
  */
extern int deliver_mailbox(LOCAL_STATE, USER_ATTR, int *);
extern int deliver_command(LOCAL_STATE, USER_ATTR, const char *);
extern int deliver_file(LOCAL_STATE, USER_ATTR, char *);
extern int deliver_indirect(LOCAL_STATE);
extern int deliver_maildir(LOCAL_STATE, USER_ATTR, char *);
extern int deliver_unknown(LOCAL_STATE, USER_ATTR);

 /*
  * Restrictions on delivery to sensitive destinations.
  */
extern int local_file_deliver_mask;
extern int local_cmd_deliver_mask;

 /*
  * Restrictions on extension propagation.
  */
extern int local_ext_prop_mask;

 /*
  * Mailbox lock protocol.
  */
extern int local_mbox_lock_mask;

 /*
  * When to prepend a Delivered-To: header upon external delivery.
  */
#define DELIVER_HDR_CMD		(1<<0)
#define DELIVER_HDR_FILE	(1<<1)
#define DELIVER_HDR_FWD		(1<<2)

extern int local_deliver_hdr_mask;

 /*
  * forward.c
  */
extern int forward_init(void);
extern int forward_append(DELIVER_ATTR);
extern int forward_finish(DELIVER_REQUEST *, DELIVER_ATTR, int);

 /*
  * feature.c
  */
extern int feature_control(const char *);

 /*
  * local_expand.c
  */
int     local_expand(VSTRING *, const char *, LOCAL_STATE *, USER_ATTR *, const char *);

#define LOCAL_EXP_EXTENSION_MATCHED	(1<<MAC_PARSE_USER)

 /*
  * alias.c
  */
extern MAPS *alias_maps;

 /*
  * Silly little macros.
  */
#define STR(s)	vstring_str(s)

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
