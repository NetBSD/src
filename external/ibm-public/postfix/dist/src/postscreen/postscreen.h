/*	$NetBSD: postscreen.h,v 1.1.1.2 2013/01/02 18:59:04 tron Exp $	*/

/*++
/* NAME
/*	postscreen 3h
/* SUMMARY
/*	postscreen internal interfaces
/* SYNOPSIS
/*	#include <postscreen.h>
/* DESCRIPTION
/* .nf

 /*
  * System library.
  */

 /*
  * Utility library.
  */
#include <dict_cache.h>
#include <vstream.h>
#include <vstring.h>
#include <events.h>
#include <htable.h>

 /*
  * Global library.
  */
#include <addr_match_list.h>
#include <string_list.h>
#include <maps.h>
#include <server_acl.h>

 /*
  * Preliminary stuff, to be fixed.
  */
#define PSC_READ_BUF_SIZE	1024

 /*
  * Per-session state.
  */
typedef struct {
    int     flags;			/* see below */
    /* Socket state. */
    VSTREAM *smtp_client_stream;	/* remote SMTP client */
    int     smtp_server_fd;		/* real SMTP server */
    char   *smtp_client_addr;		/* client address */
    char   *smtp_client_port;		/* client port */
    int     client_concurrency;		/* per-client */
    const char *final_reply;		/* cause for hanging up */
    VSTRING *send_buf;			/* pending output */
    /* Test context. */
    struct timeval start_time;		/* start of current test */
    const char *test_name;		/* name of current test */
    /* Before-handshake tests. */
    time_t  pregr_stamp;		/* pregreet expiration time */
    time_t  dnsbl_stamp;		/* dnsbl expiration time */
    VSTRING *dnsbl_reply;		/* dnsbl reject text */
    int     dnsbl_index;		/* dnsbl request index */
    time_t  penal_stamp;		/* penalty expiration time */
    /* Built-in SMTP protocol engine. */
    time_t  pipel_stamp;		/* pipelining expiration time */
    time_t  nsmtp_stamp;		/* non-smtp command expiration time */
    time_t  barlf_stamp;		/* bare newline expiration time */
    const char *rcpt_reply;		/* how to reject recipients */
    int     command_count;		/* error + junk command count */
    const char *protocol;		/* SMTP or ESMTP */
    char   *helo_name;			/* SMTP helo/ehlo */
    char   *sender;			/* MAIL FROM */
    VSTRING *cmd_buffer;		/* command read buffer */
    int     read_state;			/* command read state machine */
    /* smtpd(8) compatibility */
    int     ehlo_discard_mask;		/* EHLO filter */
    VSTRING *expand_buf;		/* macro expansion */
} PSC_STATE;

#define PSC_TIME_STAMP_NEW		(0)	/* test was never passed */
#define PSC_TIME_STAMP_DISABLED		(1)	/* never passed but disabled */
#define PSC_TIME_STAMP_INVALID		(-1)	/* must not be cached */

#define PSC_STATE_FLAG_NOFORWARD	(1<<0)	/* don't forward this session */
#define PSC_STATE_FLAG_USING_TLS	(1<<1)	/* using the TLS proxy */
#define PSC_STATE_FLAG_UNUSED2		(1<<2)	/* use me! */
#define PSC_STATE_FLAG_NEW		(1<<3)	/* some test was never passed */
#define PSC_STATE_FLAG_BLIST_FAIL	(1<<4)	/* blacklisted */
#define PSC_STATE_FLAG_HANGUP		(1<<5)	/* NOT a test failure */
#define PSC_STATE_FLAG_SMTPD_X21	(1<<6)	/* hang up after command */
#define PSC_STATE_FLAG_WLIST_FAIL	(1<<7)	/* do not whitelist */

 /*
  * Important: every MUMBLE_TODO flag must have a MUMBLE_PASS flag, such that
  * MUMBLE_PASS == PSC_STATE_FLAGS_TODO_TO_PASS(MUMBLE_TODO).
  * 
  * MUMBLE_TODO flags must not be cleared once raised. The _TODO_TO_PASS and
  * _TODO_TO_DONE macros depend on this to decide that a group of tests is
  * passed or completed.
  * 
  * MUMBLE_DONE flags are used for "early" tests that have final results.
  * 
  * MUMBLE_SKIP flags are used for "deep" tests where the client messed up.
  * These flags look like MUMBLE_DONE but they are different. Deep tests can
  * tentatively pass, but can still fail later in a session. The "ignore"
  * action introduces an additional complication. MUMBLE_PASS indicates
  * either that a deep test passed tentatively, or that the test failed but
  * the result was ignored. MUMBLE_FAIL, on the other hand, is always final.
  * We use MUMBLE_SKIP to indicate that a decision was either "fail" or
  * forced "pass".
  */
#define PSC_STATE_FLAGS_TODO_TO_PASS(todo_flags) ((todo_flags) >> 1)
#define PSC_STATE_FLAGS_TODO_TO_DONE(todo_flags) ((todo_flags) << 1)

#define PSC_STATE_FLAG_PENAL_UPDATE	(1<<6)	/* save new penalty */
#define PSC_STATE_FLAG_PENAL_FAIL	(1<<7)	/* penalty is active */

#define PSC_STATE_FLAG_PREGR_FAIL	(1<<8)	/* failed pregreet test */
#define PSC_STATE_FLAG_PREGR_PASS	(1<<9)	/* passed pregreet test */
#define PSC_STATE_FLAG_PREGR_TODO	(1<<10)	/* pregreet test expired */
#define PSC_STATE_FLAG_PREGR_DONE	(1<<11)	/* decision is final */

#define PSC_STATE_FLAG_DNSBL_FAIL	(1<<12)	/* failed DNSBL test */
#define PSC_STATE_FLAG_DNSBL_PASS	(1<<13)	/* passed DNSBL test */
#define PSC_STATE_FLAG_DNSBL_TODO	(1<<14)	/* DNSBL test expired */
#define PSC_STATE_FLAG_DNSBL_DONE	(1<<15)	/* decision is final */

 /* Room here for one more after-handshake test. */

#define PSC_STATE_FLAG_PIPEL_FAIL	(1<<20)	/* failed pipelining test */
#define PSC_STATE_FLAG_PIPEL_PASS	(1<<21)	/* passed pipelining test */
#define PSC_STATE_FLAG_PIPEL_TODO	(1<<22)	/* pipelining test expired */
#define PSC_STATE_FLAG_PIPEL_SKIP	(1<<23)	/* action is already logged */

#define PSC_STATE_FLAG_NSMTP_FAIL	(1<<24)	/* failed non-SMTP test */
#define PSC_STATE_FLAG_NSMTP_PASS	(1<<25)	/* passed non-SMTP test */
#define PSC_STATE_FLAG_NSMTP_TODO	(1<<26)	/* non-SMTP test expired */
#define PSC_STATE_FLAG_NSMTP_SKIP	(1<<27)	/* action is already logged */

#define PSC_STATE_FLAG_BARLF_FAIL	(1<<28)	/* failed bare newline test */
#define PSC_STATE_FLAG_BARLF_PASS	(1<<29)	/* passed bare newline test */
#define PSC_STATE_FLAG_BARLF_TODO	(1<<30)	/* bare newline test expired */
#define PSC_STATE_FLAG_BARLF_SKIP	(1<<31)	/* action is already logged */

 /*
  * Aggregates for individual tests.
  */
#define PSC_STATE_MASK_PREGR_TODO_FAIL \
	(PSC_STATE_FLAG_PREGR_TODO | PSC_STATE_FLAG_PREGR_FAIL)
#define PSC_STATE_MASK_DNSBL_TODO_FAIL \
	(PSC_STATE_FLAG_DNSBL_TODO | PSC_STATE_FLAG_DNSBL_FAIL)
#define PSC_STATE_MASK_PIPEL_TODO_FAIL \
	(PSC_STATE_FLAG_PIPEL_TODO | PSC_STATE_FLAG_PIPEL_FAIL)
#define PSC_STATE_MASK_NSMTP_TODO_FAIL \
	(PSC_STATE_FLAG_NSMTP_TODO | PSC_STATE_FLAG_NSMTP_FAIL)
#define PSC_STATE_MASK_BARLF_TODO_FAIL \
	(PSC_STATE_FLAG_BARLF_TODO | PSC_STATE_FLAG_BARLF_FAIL)

#define PSC_STATE_MASK_PIPEL_TODO_SKIP \
	(PSC_STATE_FLAG_PIPEL_TODO | PSC_STATE_FLAG_PIPEL_SKIP)
#define PSC_STATE_MASK_NSMTP_TODO_SKIP \
	(PSC_STATE_FLAG_NSMTP_TODO | PSC_STATE_FLAG_NSMTP_SKIP)
#define PSC_STATE_MASK_BARLF_TODO_SKIP \
	(PSC_STATE_FLAG_BARLF_TODO | PSC_STATE_FLAG_BARLF_SKIP)

#define PSC_STATE_MASK_PIPEL_TODO_PASS_FAIL \
	(PSC_STATE_MASK_PIPEL_TODO_FAIL | PSC_STATE_FLAG_PIPEL_PASS)
#define PSC_STATE_MASK_NSMTP_TODO_PASS_FAIL \
	(PSC_STATE_MASK_NSMTP_TODO_FAIL | PSC_STATE_FLAG_NSMTP_PASS)
#define PSC_STATE_MASK_BARLF_TODO_PASS_FAIL \
	(PSC_STATE_MASK_BARLF_TODO_FAIL | PSC_STATE_FLAG_BARLF_PASS)

 /*
  * Separate aggregates for early tests and deep tests.
  */
#define PSC_STATE_MASK_EARLY_DONE \
	(PSC_STATE_FLAG_PREGR_DONE | PSC_STATE_FLAG_DNSBL_DONE)
#define PSC_STATE_MASK_EARLY_TODO \
	(PSC_STATE_FLAG_PREGR_TODO | PSC_STATE_FLAG_DNSBL_TODO)
#define PSC_STATE_MASK_EARLY_PASS \
	(PSC_STATE_FLAG_PREGR_PASS | PSC_STATE_FLAG_DNSBL_PASS)
#define PSC_STATE_MASK_EARLY_FAIL \
	(PSC_STATE_FLAG_PREGR_FAIL | PSC_STATE_FLAG_DNSBL_FAIL)

#define PSC_STATE_MASK_SMTPD_TODO \
	(PSC_STATE_FLAG_PIPEL_TODO | PSC_STATE_FLAG_NSMTP_TODO | \
	PSC_STATE_FLAG_BARLF_TODO)
#define PSC_STATE_MASK_SMTPD_PASS \
	(PSC_STATE_FLAG_PIPEL_PASS | PSC_STATE_FLAG_NSMTP_PASS | \
	PSC_STATE_FLAG_BARLF_PASS)
#define PSC_STATE_MASK_SMTPD_FAIL \
	(PSC_STATE_FLAG_PIPEL_FAIL | PSC_STATE_FLAG_NSMTP_FAIL | \
	PSC_STATE_FLAG_BARLF_FAIL)

 /*
  * Super-aggregates for all tests combined.
  */
#define PSC_STATE_MASK_ANY_FAIL \
	(PSC_STATE_FLAG_BLIST_FAIL | PSC_STATE_FLAG_PENAL_FAIL | \
	PSC_STATE_MASK_EARLY_FAIL | PSC_STATE_MASK_SMTPD_FAIL | \
	PSC_STATE_FLAG_WLIST_FAIL)

#define PSC_STATE_MASK_ANY_PASS \
	(PSC_STATE_MASK_EARLY_PASS | PSC_STATE_MASK_SMTPD_PASS)

#define PSC_STATE_MASK_ANY_TODO \
	(PSC_STATE_MASK_EARLY_TODO | PSC_STATE_MASK_SMTPD_TODO)

#define PSC_STATE_MASK_ANY_TODO_FAIL \
	(PSC_STATE_MASK_ANY_TODO | PSC_STATE_MASK_ANY_FAIL)

#define PSC_STATE_MASK_ANY_UPDATE \
	(PSC_STATE_MASK_ANY_PASS | PSC_STATE_FLAG_PENAL_UPDATE)

 /*
  * See log_adhoc.c for discussion.
  */
typedef struct {
    int     dt_sec;			/* make sure it's signed */
    int     dt_usec;			/* make sure it's signed */
} DELTA_TIME;

#define PSC_CALC_DELTA(x, y, z) \
    do { \
	(x).dt_sec = (y).tv_sec - (z).tv_sec; \
	(x).dt_usec = (y).tv_usec - (z).tv_usec; \
	while ((x).dt_usec < 0) { \
	    (x).dt_usec += 1000000; \
	    (x).dt_sec -= 1; \
	} \
	while ((x).dt_usec >= 1000000) { \
	    (x).dt_usec -= 1000000; \
	    (x).dt_sec += 1; \
	} \
	if ((x).dt_sec < 0) \
	    (x).dt_sec = (x).dt_usec = 0; \
    } while (0)

#define SIG_DIGS        2

 /*
  * Event management.
  */

/* PSC_READ_EVENT_REQUEST - prepare for transition to next state */

#define PSC_READ_EVENT_REQUEST(fd, action, context, timeout) do { \
	if (msg_verbose > 1) \
	    msg_info("%s: read-request fd=%d", myname, (fd)); \
	event_enable_read((fd), (action), (context)); \
	event_request_timer((action), (context), (timeout)); \
    } while (0)

#define PSC_READ_EVENT_REQUEST2(fd, read_act, time_act, context, timeout) do { \
	if (msg_verbose > 1) \
	    msg_info("%s: read-request fd=%d", myname, (fd)); \
	event_enable_read((fd), (read_act), (context)); \
	event_request_timer((time_act), (context), (timeout)); \
    } while (0)

/* PSC_CLEAR_EVENT_REQUEST - complete state transition */

#define PSC_CLEAR_EVENT_REQUEST(fd, time_act, context) do { \
	if (msg_verbose > 1) \
	    msg_info("%s: clear-request fd=%d", myname, (fd)); \
	event_disable_readwrite(fd); \
	event_cancel_timer((time_act), (context)); \
    } while (0)

 /*
  * Failure enforcement policies.
  */
#define PSC_NAME_ACT_DROP	"drop"
#define PSC_NAME_ACT_ENFORCE	"enforce"
#define PSC_NAME_ACT_IGNORE	"ignore"
#define PSC_NAME_ACT_CONT	"continue"

#define PSC_ACT_DROP		1
#define PSC_ACT_ENFORCE		2
#define PSC_ACT_IGNORE		3

 /*
  * Global variables.
  */
extern int psc_check_queue_length;	/* connections being checked */
extern int psc_post_queue_length;	/* being sent to real SMTPD */
extern DICT_CACHE *psc_cache_map;	/* cache table handle */
extern VSTRING *psc_temp;		/* scratchpad */
extern char *psc_smtpd_service_name;	/* path to real SMTPD */
extern int psc_pregr_action;		/* PSC_ACT_DROP etc. */
extern int psc_dnsbl_action;		/* PSC_ACT_DROP etc. */
extern int psc_pipel_action;		/* PSC_ACT_DROP etc. */
extern int psc_nsmtp_action;		/* PSC_ACT_DROP etc. */
extern int psc_barlf_action;		/* PSC_ACT_DROP etc. */
extern int psc_min_ttl;			/* Update with new tests! */
extern int psc_max_ttl;			/* Update with new tests! */
extern STRING_LIST *psc_forbid_cmds;	/* CONNECT GET POST */
extern int psc_stress_greet_wait;	/* stressed greet wait */
extern int psc_normal_greet_wait;	/* stressed greet wait */
extern int psc_stress_cmd_time_limit;	/* stressed command limit */
extern int psc_normal_cmd_time_limit;	/* normal command time limit */
extern int psc_stress;			/* stress level */
extern int psc_lowat_check_queue_length;/* stress low-water mark */
extern int psc_hiwat_check_queue_length;/* stress high-water mark */
extern DICT *psc_dnsbl_reply;		/* DNSBL name mapper */
extern HTABLE *psc_client_concurrency;	/* per-client concurrency */

#define PSC_EFF_GREET_WAIT \
	(psc_stress ? psc_stress_greet_wait : psc_normal_greet_wait)
#define PSC_EFF_CMD_TIME_LIMIT \
	(psc_stress ? psc_stress_cmd_time_limit : psc_normal_cmd_time_limit)

 /*
  * String plumbing macros.
  */
#define PSC_STRING_UPDATE(str, text) do { \
	if (str) myfree(str); \
	(str) = ((text) ? mystrdup(text) : 0); \
    } while (0)

#define PSC_STRING_RESET(str) do { \
	if (str) { \
	    myfree(str); \
	    (str) = 0; \
	} \
    } while (0)

 /*
  * SLMs.
  */
#define STR(x)  vstring_str(x)
#define LEN(x)  VSTRING_LEN(x)

 /*
  * postscreen_state.c
  */
#define PSC_CLIENT_ADDR_PORT(state) \
	(state)->smtp_client_addr, (state)->smtp_client_port

#define PSC_PASS_SESSION_STATE(state, what, bits) do { \
	if (msg_verbose) \
	    msg_info("PASS %s [%s]:%s", (what), PSC_CLIENT_ADDR_PORT(state)); \
	(state)->flags |= (bits); \
    } while (0)
#define PSC_FAIL_SESSION_STATE(state, bits) do { \
	if (msg_verbose) \
	    msg_info("FAIL [%s]:%s", PSC_CLIENT_ADDR_PORT(state)); \
	(state)->flags |= (bits); \
    } while (0)
#define PSC_SKIP_SESSION_STATE(state, what, bits) do { \
	if (msg_verbose) \
	    msg_info("SKIP %s [%s]:%s", (what), PSC_CLIENT_ADDR_PORT(state)); \
	(state)->flags |= (bits); \
    } while (0)
#define PSC_DROP_SESSION_STATE(state, reply) do { \
	if (msg_verbose) \
	    msg_info("DROP [%s]:%s", PSC_CLIENT_ADDR_PORT(state)); \
	(state)->flags |= PSC_STATE_FLAG_NOFORWARD; \
	(state)->final_reply = (reply); \
	psc_conclude(state); \
    } while (0)
#define PSC_ENFORCE_SESSION_STATE(state, reply) do { \
	if (msg_verbose) \
	    msg_info("ENFORCE [%s]:%s", PSC_CLIENT_ADDR_PORT(state)); \
	(state)->rcpt_reply = (reply); \
	(state)->flags |= PSC_STATE_FLAG_NOFORWARD; \
    } while (0)
#define PSC_UNPASS_SESSION_STATE(state, bits) do { \
	if (msg_verbose) \
	    msg_info("UNPASS [%s]:%s", PSC_CLIENT_ADDR_PORT(state)); \
	(state)->flags &= ~(bits); \
    } while (0)
#define PSC_UNFAIL_SESSION_STATE(state, bits) do { \
	if (msg_verbose) \
	    msg_info("UNFAIL [%s]:%s", PSC_CLIENT_ADDR_PORT(state)); \
	(state)->flags &= ~(bits); \
    } while (0)
#define PSC_ADD_SERVER_STATE(state, fd) do { \
	(state)->smtp_server_fd = (fd); \
	psc_post_queue_length++; \
    } while (0)
#define PSC_DEL_CLIENT_STATE(state) do { \
	event_server_disconnect((state)->smtp_client_stream); \
	(state)->smtp_client_stream = 0; \
	psc_check_queue_length--; \
    } while (0)
extern PSC_STATE *psc_new_session_state(VSTREAM *, const char *, const char *);
extern void psc_free_session_state(PSC_STATE *);
extern const char *psc_print_state_flags(int, const char *);

 /*
  * postscreen_dict.c
  */
extern int psc_addr_match_list_match(ADDR_MATCH_LIST *, const char *);
extern const char *psc_cache_lookup(DICT_CACHE *, const char *);
extern void psc_cache_update(DICT_CACHE *, const char *, const char *);
const char *psc_dict_get(DICT *, const char *);
const char *psc_maps_find(MAPS *, const char *, int);

 /*
  * postscreen_dnsbl.c
  */
extern void psc_dnsbl_init(void);
extern int psc_dnsbl_retrieve(const char *, const char **, int);
extern int psc_dnsbl_request(const char *, void (*) (int, char *), char *);

 /*
  * postscreen_tests.c
  */
#define PSC_INIT_TESTS(dst) do { \
	(dst)->flags = 0; \
	(dst)->pregr_stamp = PSC_TIME_STAMP_INVALID; \
	(dst)->dnsbl_stamp = PSC_TIME_STAMP_INVALID; \
	(dst)->pipel_stamp = PSC_TIME_STAMP_INVALID; \
	(dst)->barlf_stamp = PSC_TIME_STAMP_INVALID; \
	(dst)->penal_stamp = PSC_TIME_STAMP_INVALID; \
    } while (0)
#define PSC_BEGIN_TESTS(state, name) do { \
	(state)->test_name = (name); \
	GETTIMEOFDAY(&(state)->start_time); \
    } while (0)
extern void psc_new_tests(PSC_STATE *);
extern void psc_parse_tests(PSC_STATE *, const char *, time_t);
extern char *psc_print_tests(VSTRING *, PSC_STATE *);
extern char *psc_print_grey_key(VSTRING *, const char *, const char *,
				        const char *, const char *);

#define PSC_MIN(x, y) ((x) < (y) ? (x) : (y))
#define PSC_MAX(x, y) ((x) > (y) ? (x) : (y))

 /*
  * postscreen_early.c
  */
extern void psc_early_tests(PSC_STATE *);
extern void psc_early_init(void);

 /*
  * postscreen_smtpd.c
  */
extern void psc_smtpd_tests(PSC_STATE *);
extern void psc_smtpd_init(void);
extern void psc_smtpd_pre_jail_init(void);

#define PSC_SMTPD_X21(state, reply) do { \
	(state)->flags |= PSC_STATE_FLAG_SMTPD_X21; \
	(state)->final_reply = (reply); \
	psc_smtpd_tests(state); \
    } while (0)

 /*
  * postscreen_misc.c
  */
extern char *psc_format_delta_time(VSTRING *, struct timeval, DELTA_TIME *);
extern void psc_conclude(PSC_STATE *);
extern void psc_hangup_event(PSC_STATE *);

 /*
  * postscreen_send.c
  */
#define PSC_SEND_REPLY psc_send_reply	/* legacy macro */
extern int psc_send_reply(PSC_STATE *, const char *);
extern void psc_send_socket(PSC_STATE *);

 /*
  * postscreen_starttls.c
  */
extern void psc_starttls_open(PSC_STATE *, EVENT_NOTIFY_FN);

 /*
  * postscreen_expand.c
  */
extern VSTRING *psc_expand_filter;
extern void psc_expand_init(void);
extern const char *psc_expand_lookup(const char *, int, char *);

 /*
  * postscreen_access emulation.
  */
#define PSC_ACL_ACT_WHITELIST	SERVER_ACL_ACT_PERMIT
#define PSC_ACL_ACT_DUNNO	SERVER_ACL_ACT_DUNNO
#define PSC_ACL_ACT_BLACKLIST	SERVER_ACL_ACT_REJECT
#define PSC_ACL_ACT_ERROR	SERVER_ACL_ACT_ERROR

#define psc_acl_pre_jail_init	server_acl_pre_jail_init
#define psc_acl_parse		server_acl_parse
#define psc_acl_eval(s,a,p)	server_acl_eval((s)->smtp_client_addr, (a), (p))

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
