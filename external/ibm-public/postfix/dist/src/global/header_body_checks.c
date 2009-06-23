/*	$NetBSD: header_body_checks.c,v 1.1.1.1 2009/06/23 10:08:45 tron Exp $	*/

/*++
/* NAME
/*	header_body_checks 3
/* SUMMARY
/*	header/body checks
/* SYNOPSIS
/*	#include <header_body_checks.h>
/*
/*	typedef struct {
/*		void	(*logger) (void *context, const char *action,
/*				const char *where, const char *line,
/*				const char *optional_text);
/*		void	(*prepend) (void *context, int rec_type,
/*				const char *buf, ssize_t len, off_t offset);
/*		char	*(*extend) (void *context, const char *command,
/*				int cmd_len, const char *cmd_args,
/*				const char *where, const char *line,
/*				ssize_t line_len, off_t offset);
/*	} HBC_CALL_BACKS;
/*
/*	HBC_CHECKS *hbc_header_checks_create(
/*			header_checks_name, header_checks_value
/*			mime_header_checks_name, mime_header_checks_value,
/*			nested_header_checks_name, nested_header_checks_value,
/*			call_backs)
/*	const char *header_checks_name;
/*	const char *header_checks_value;
/*	const char *mime_header_checks_name;
/*	const char *mime_header_checks_value;
/*	const char *nested_header_checks_name;
/*	const char *nested_header_checks_value;
/*	HBC_CALL_BACKS *call_backs;
/*
/*	HBC_CHECKS *hbc_body_checks_create(
/*			body_checks_name, body_checks_value,
/*			call_backs)
/*	const char *body_checks_name;
/*	const char *body_checks_value;
/*	HBC_CALL_BACKS *call_backs;
/*
/*	char	*hbc_header_checks(context, hbc, header_class, hdr_opts, header)
/*	void	*context;
/*	HBC_CHECKS *hbc;
/*	int	header_class;
/*	const HEADER_OPTS *hdr_opts;
/*	VSTRING *header;
/*
/*	char	*hbc_body_checks(context, hbc, body_line, body_line_len)
/*	void	*context;
/*	HBC_CHECKS *hbc;
/*	const char *body_line;
/*	ssize_t	body_line_len;
/*
/*	void	hbc_header_checks_free(hbc)
/*	HBC_CHECKS *hbc;
/*
/*	void	hbc_body_checks_free(hbc)
/*	HBC_CHECKS *hbc;
/* DESCRIPTION
/*	This module implements header_checks and body_checks.
/*	Actions are executed while mail is being delivered. The
/*	following actions are recognized: WARN, REPLACE, PREPEND,
/*	IGNORE, DUNNO, and OK. These actions are safe for use in
/*	delivery agents.
/*
/*	Other actions may be supplied via the extension mechanism
/*	described below.  For example, actions that change the
/*	message delivery time or destination. Such actions do not
/*	make sense in delivery agents, but they can be appropriate
/*	in, for example, before-queue filters.
/*
/*	hbc_header_checks_create() creates a context for header
/*	inspection. This function is typically called once during
/*	program initialization.  The result is a null pointer when
/*	all _value arguments specify zero-length strings; in this
/*	case, hbc_header_checks() and hbc_header_checks_free() must
/*	not be called.
/*
/*	hbc_header_checks() inspects the specified logical header.
/*	The result is either the original header, HBC_CHECK_STAT_IGNORE
/*	(meaning: discard the header) or a new header (meaning:
/*	replace the header and destroy the new header with myfree()).
/*
/*	hbc_header_checks_free() returns memory to the pool.
/*
/*	hbc_body_checks_create(), dbhc_body_checks(), dbhc_body_free()
/*	perform similar functions for body lines.
/*
/*	Arguments:
/* .IP body_line
/*	One line of body text.
/* .IP body_line_len
/*	Body line length.
/* .IP call_backs
/*	Table with call-back function pointers. This argument is
/*	not copied.  Note: the description below is not necessarily
/*	in data structure order.
/* .RS
/* .IP logger
/*	Call-back function for logging an action with the action's
/*	name in lower case, a location within a message ("header"
/*	or "body"), the content of the header or body line that
/*	triggered the action, and optional text or a zero-length
/*	string. This call-back feature must be specified.
/* .IP prepend
/*	Call-back function for the PREPEND action. The arguments
/*	are the same as those of mime_state(3) body output call-back
/*	functions.  Specify a null pointer to disable this action.
/* .IP extend
/*	Call-back function that logs and executes other actions.
/*	This function receives as arguments the command name and
/*	name length, the command arguments if any, the location
/*	within the message ("header" or "body"), the content and
/*	length of the header or body line that triggered the action,
/*	and the input byte offset within the current header or body
/*	segment.  The result value is either the original line
/*	argument, HBC_CHECKS_STAT_IGNORE (delete the line from the
/*	input stream) or HBC_CHECK_STAT_UNKNOWN (the command was
/*	not recognized).  Specify a null pointer to disable this
/*	feature.
/* .RE
/* .IP context
/*	Application context for call-back functions specified with the
/*	call_backs argument.
/* .IP header
/*	A logical message header. Lines within a multi-line header
/*	are separated by a newline character.
/* .IP "header_checks_name, mime_header_checks_name"
/* .IP "nested_header_checks_name, body_checks_name"
/*	The main.cf configuration parameter names for header and body
/*	map lists.
/* .IP "header_checks_value, mime_header_checks_value"
/* .IP "nested_header_checks_value, body_checks_value"
/*	The values of main.cf configuration parameters for header and body
/*	map lists. Specify a zero-length string to disable a specific list.
/* .IP header_class
/*	A number in the range MIME_HDR_FIRST..MIME_HDR_LAST.
/* .IP hbc
/*	A handle created with hbc_header_checks_create() or
/*	hbc_body_checks_create().
/* .IP hdr_opts
/*	Message header properties.
/* SEE ALSO
/*	msg(3) diagnostics interface
/* DIAGNOSTICS
/*	Fatal errors: memory allocation problem.
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

/* System library. */

#include <sys_defs.h>
#include <ctype.h>
#include <string.h>
#ifdef STRCASECMP_IN_STRINGS_H
#include <strings.h>
#endif

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>

/* Global library. */

#include <mime_state.h>
#include <rec_type.h>
#include <is_header.h>
#include <cleanup_user.h>
#include <dsn_util.h>
#include <header_body_checks.h>

/* Application-specific. */

 /*
  * Something that is guaranteed to be different from a real string result
  * from header/body_checks.
  */
const char hbc_checks_unknown;

 /*
  * Header checks are stored as an array of HBC_MAP_INFO structures, one
  * structure for each header class (MIME_HDR_PRIMARY, MIME_HDR_MULTIPART, or
  * MIME_HDR_NESTED).
  * 
  * Body checks are stored as one single HBC_MAP_INFO structure, because we make
  * no distinction between body segments.
  */
#define HBC_HEADER_INDEX(class)	((class) - MIME_HDR_FIRST)
#define HBC_BODY_INDEX	(0)

#define HBC_INIT(hbc, index, name, value) do { \
	HBC_MAP_INFO *_mp = (hbc)->map_info + (index); \
	if (*(value) != 0) { \
	    _mp->map_class = (name); \
	    _mp->maps = maps_create((name), (value), DICT_FLAG_LOCK); \
	} else { \
	    _mp->map_class = 0; \
	    _mp->maps = 0; \
	} \
    } while (0)

/* How does the action routine know where we are? */

#define	HBC_CTXT_HEADER	"header"
#define HBC_CTXT_BODY	"body"

/* Silly little macros. */

#define STR(x)	vstring_str(x)
#define LEN(x)	VSTRING_LEN(x)

/* hbc_action - act upon a header/body match */

static char *hbc_action(void *context, HBC_CALL_BACKS *cb,
			        const char *map_class, const char *where,
			        const char *cmd, const char *line,
			        ssize_t line_len, off_t offset)
{
    const char *cmd_args = cmd + strcspn(cmd, " \t");
    int     cmd_len = cmd_args - cmd;
    char   *ret;

    /*
     * XXX We don't use a hash table for action lookup. Mail rarely triggers
     * an action, and mail that triggers multiple actions is even rarer.
     * Setting up the hash table costs more than we would gain from using it.
     */
    while (*cmd_args && ISSPACE(*cmd_args))
	cmd_args++;

#define STREQUAL(x,y,l) (strncasecmp((x), (y), (l)) == 0 && (y)[l] == 0)

    if (cb->extend
	&& (ret = cb->extend(context, cmd, cmd_len, cmd_args, where, line,
			     line_len, offset)) != HBC_CHECKS_STAT_UNKNOWN)
	return (ret);

    if (STREQUAL(cmd, "WARN", cmd_len)) {
	cb->logger(context, "warning", where, line, cmd_args);
	return ((char *) line);
    }
    if (STREQUAL(cmd, "REPLACE", cmd_len)) {
	if (*cmd_args == 0) {
	    msg_warn("REPLACE action without text in %s map", map_class);
	    return ((char *) line);
	} else if (strcmp(where, HBC_CTXT_HEADER) == 0
		   && !is_header(cmd_args)) {
	    msg_warn("bad REPLACE header text \"%s\" in %s map -- "
		   "need \"headername: headervalue\"", cmd_args, map_class);
	    return ((char *) line);
	} else {
	    cb->logger(context, "replace", where, line, cmd_args);
	    return (mystrdup(cmd_args));
	}
    }
    if (cb->prepend && STREQUAL(cmd, "PREPEND", cmd_len)) {
	if (*cmd_args == 0) {
	    msg_warn("PREPEND action without text in %s map", map_class);
	} else if (strcmp(where, HBC_CTXT_HEADER) == 0
		   && !is_header(cmd_args)) {
	    msg_warn("bad PREPEND header text \"%s\" in %s map -- "
		   "need \"headername: headervalue\"", cmd_args, map_class);
	} else {
	    cb->logger(context, "prepend", where, line, cmd_args);
	    cb->prepend(context, REC_TYPE_NORM, cmd_args, strlen(cmd_args), offset);
	}
	return ((char *) line);
    }
    /* Allow and ignore optional text after the action. */

    if (STREQUAL(cmd, "IGNORE", cmd_len))
	/* XXX Not logged for compatibility with cleanup(8). */
	return (HBC_CHECKS_STAT_IGNORE);

    if (STREQUAL(cmd, "DUNNO", cmd_len)		/* preferred */
	||STREQUAL(cmd, "OK", cmd_len))		/* compatibility */
	return ((char *) line);

    msg_warn("unsupported command in %s map: %s", map_class, cmd);
    return ((char *) line);
}

/* hbc_header_checks - process one complete header line */

char   *hbc_header_checks(void *context, HBC_CHECKS *hbc, int header_class,
			          const HEADER_OPTS *hdr_opts,
			          VSTRING *header, off_t offset)
{
    const char *myname = "hbc_header_checks";
    const char *action;
    HBC_MAP_INFO *mp;

    if (msg_verbose)
	msg_info("%s: '%.30s'", myname, STR(header));

    /*
     * XXX This is for compatibility with the cleanup(8) server.
     */
    if (hdr_opts && (hdr_opts->flags & HDR_OPT_MIME))
	header_class = MIME_HDR_MULTIPART;

    mp = hbc->map_info + HBC_HEADER_INDEX(header_class);

    if (mp->maps != 0 && (action = maps_find(mp->maps, STR(header), 0)) != 0) {
	return (hbc_action(context, hbc->call_backs,
			   mp->map_class, HBC_CTXT_HEADER, action,
			   STR(header), LEN(header), offset));
    } else {
	return (STR(header));
    }
}

/* hbc_body_checks - inspect one body record */

char   *hbc_body_checks(void *context, HBC_CHECKS *hbc, const char *line,
			        ssize_t len, off_t offset)
{
    const char *myname = "hbc_body_checks";
    const char *action;
    HBC_MAP_INFO *mp;

    if (msg_verbose)
	msg_info("%s: '%.30s'", myname, line);

    mp = hbc->map_info;

    if ((action = maps_find(mp->maps, line, 0)) != 0) {
	return (hbc_action(context, hbc->call_backs,
			   mp->map_class, HBC_CTXT_BODY, action,
			   line, len, offset));
    } else {
	return ((char *) line);
    }
}

/* hbc_header_checks_create - create header checking context */

HBC_CHECKS *hbc_header_checks_create(const char *header_checks_name,
				             const char *header_checks_value,
				        const char *mime_header_checks_name,
			               const char *mime_header_checks_value,
			              const char *nested_header_checks_name,
			             const char *nested_header_checks_value,
				             HBC_CALL_BACKS *call_backs)
{
    HBC_CHECKS *hbc;

    /*
     * Optimize for the common case.
     */
    if (*header_checks_value == 0 && *mime_header_checks_value == 0
	&& *nested_header_checks_value == 0) {
	return (0);
    } else {
	hbc = (HBC_CHECKS *) mymalloc(sizeof(*hbc)
		 + (MIME_HDR_LAST - MIME_HDR_FIRST) * sizeof(HBC_MAP_INFO));
	hbc->call_backs = call_backs;
	HBC_INIT(hbc, HBC_HEADER_INDEX(MIME_HDR_PRIMARY),
		 header_checks_name, header_checks_value);
	HBC_INIT(hbc, HBC_HEADER_INDEX(MIME_HDR_MULTIPART),
		 mime_header_checks_name, mime_header_checks_value);
	HBC_INIT(hbc, HBC_HEADER_INDEX(MIME_HDR_NESTED),
		 nested_header_checks_name, nested_header_checks_value);
	return (hbc);
    }
}

/* hbc_body_checks_create - create body checking context */

HBC_CHECKS *hbc_body_checks_create(const char *body_checks_name,
				           const char *body_checks_value,
				           HBC_CALL_BACKS *call_backs)
{
    HBC_CHECKS *hbc;

    /*
     * Optimize for the common case.
     */
    if (*body_checks_value == 0) {
	return (0);
    } else {
	hbc = (HBC_CHECKS *) mymalloc(sizeof(*hbc));
	hbc->call_backs = call_backs;
	HBC_INIT(hbc, HBC_BODY_INDEX, body_checks_name, body_checks_value);
	return (hbc);
    }
}

/* _hbc_checks_free - destroy header/body checking context */

void    _hbc_checks_free(HBC_CHECKS *hbc, ssize_t len)
{
    HBC_MAP_INFO *mp;

    for (mp = hbc->map_info; mp < hbc->map_info + len; mp++)
	if (mp->maps)
	    maps_free(mp->maps);
    myfree((char *) hbc);
}

 /*
  * Test program. Specify the four maps on the command line, and feed a
  * MIME-formatted message on stdin.
  */

#ifdef TEST

#include <stdlib.h>
#include <stringops.h>
#include <vstream.h>
#include <msg_vstream.h>
#include <rec_streamlf.h>

typedef struct {
    HBC_CHECKS *header_checks;
    HBC_CHECKS *body_checks;
    HBC_CALL_BACKS *call_backs;
    VSTREAM *fp;
    VSTRING *buf;
    const char *queueid;
    int     recno;
} HBC_TEST_CONTEXT;

/*#define REC_LEN	40*/
#define REC_LEN	1024

/* log_cb - log action with context */

static void log_cb(void *context, const char *action, const char *where,
		           const char *content, const char *text)
{
    const HBC_TEST_CONTEXT *dp = (HBC_TEST_CONTEXT *) context;

    if (*text) {
	msg_info("%s: %s: %s %.200s: %s",
		 dp->queueid, action, where, content, text);
    } else {
	msg_info("%s: %s: %s %.200s",
		 dp->queueid, action, where, content);
    }
}

/* out_cb - output call-back */

static void out_cb(void *context, int rec_type, const char *buf,
		           ssize_t len, off_t offset)
{
    const HBC_TEST_CONTEXT *dp = (HBC_TEST_CONTEXT *) context;

    vstream_fwrite(dp->fp, buf, len);
    VSTREAM_PUTC('\n', dp->fp);
    vstream_fflush(dp->fp);
}

/* head_out - MIME_STATE header call-back */

static void head_out(void *context, int header_class,
		             const HEADER_OPTS *header_info,
		             VSTRING *buf, off_t offset)
{
    HBC_TEST_CONTEXT *dp = (HBC_TEST_CONTEXT *) context;
    char   *out;

    if (dp->header_checks == 0
	|| (out = hbc_header_checks(context, dp->header_checks, header_class,
				    header_info, buf, offset)) == STR(buf)) {
	vstring_sprintf(dp->buf, "%d %s %ld\t|%s",
			dp->recno,
			header_class == MIME_HDR_PRIMARY ? "MAIN" :
			header_class == MIME_HDR_MULTIPART ? "MULT" :
			header_class == MIME_HDR_NESTED ? "NEST" :
			"ERROR", (long) offset, STR(buf));
	out_cb(dp, REC_TYPE_NORM, STR(dp->buf), LEN(dp->buf), offset);
    } else if (out != 0) {
	vstring_sprintf(dp->buf, "%d %s %ld\t|%s",
			dp->recno,
			header_class == MIME_HDR_PRIMARY ? "MAIN" :
			header_class == MIME_HDR_MULTIPART ? "MULT" :
			header_class == MIME_HDR_NESTED ? "NEST" :
			"ERROR", (long) offset, out);
	out_cb(dp, REC_TYPE_NORM, STR(dp->buf), LEN(dp->buf), offset);
	myfree(out);
    }
    dp->recno += 1;
}

/* header_end - MIME_STATE end-of-header call-back */

static void head_end(void *context)
{
    HBC_TEST_CONTEXT *dp = (HBC_TEST_CONTEXT *) context;

    out_cb(dp, 0, "HEADER END", sizeof("HEADER END") - 1, 0);
}

/* body_out - MIME_STATE body line call-back */

static void body_out(void *context, int rec_type, const char *buf,
		             ssize_t len, off_t offset)
{
    HBC_TEST_CONTEXT *dp = (HBC_TEST_CONTEXT *) context;
    char   *out;

    if (dp->body_checks == 0
	|| (out = hbc_body_checks(context, dp->body_checks,
				  buf, len, offset)) == buf) {
	vstring_sprintf(dp->buf, "%d BODY %c %ld\t|%s",
			dp->recno, rec_type, (long) offset, buf);
	out_cb(dp, rec_type, STR(dp->buf), LEN(dp->buf), offset);
    } else if (out != 0) {
	vstring_sprintf(dp->buf, "%d BODY %c %ld\t|%s",
			dp->recno, rec_type, (long) offset, out);
	out_cb(dp, rec_type, STR(dp->buf), LEN(dp->buf), offset);
	myfree(out);
    }
    dp->recno += 1;
}

/* body_end - MIME_STATE end-of-message call-back */

static void body_end(void *context)
{
    HBC_TEST_CONTEXT *dp = (HBC_TEST_CONTEXT *) context;

    out_cb(dp, 0, "BODY END", sizeof("BODY END") - 1, 0);
}

/* err_print - print MIME_STATE errors */

static void err_print(void *unused_context, int err_flag,
		              const char *text, ssize_t len)
{
    msg_warn("%s: %.*s", mime_state_error(err_flag),
	     len < 100 ? (int) len : 100, text);
}

int     var_header_limit = 2000;
int     var_mime_maxdepth = 20;
int     var_mime_bound_len = 2000;

int     main(int argc, char **argv)
{
    int     rec_type;
    VSTRING *buf;
    int     err;
    MIME_STATE *mime_state;
    HBC_TEST_CONTEXT context;
    static HBC_CALL_BACKS call_backs[1] = {
	log_cb,				/* logger */
	out_cb,				/* prepend */
    };

    /*
     * Sanity check.
     */
    if (argc != 5)
	msg_fatal("usage: %s header_checks mime_header_checks nested_header_checks body_checks", argv[0]);

    /*
     * Initialize.
     */
#define MIME_OPTIONS \
            (MIME_OPT_REPORT_8BIT_IN_7BIT_BODY \
            | MIME_OPT_REPORT_8BIT_IN_HEADER \
            | MIME_OPT_REPORT_ENCODING_DOMAIN \
            | MIME_OPT_REPORT_TRUNC_HEADER \
            | MIME_OPT_REPORT_NESTING \
            | MIME_OPT_DOWNGRADE)
    msg_vstream_init(basename(argv[0]), VSTREAM_OUT);
    buf = vstring_alloc(10);
    mime_state = mime_state_alloc(MIME_OPTIONS,
				  head_out, head_end,
				  body_out, body_end,
				  err_print,
				  (void *) &context);
    context.header_checks =
	hbc_header_checks_create("header_checks", argv[1],
				 "mime_header_checks", argv[2],
				 "nested_header_checks", argv[3],
				 call_backs);
    context.body_checks =
	hbc_body_checks_create("body_checks", argv[4], call_backs);
    context.buf = vstring_alloc(100);
    context.fp = VSTREAM_OUT;
    context.queueid = "test-queueID";
    context.recno = 0;

    /*
     * Main loop.
     */
    do {
	rec_type = rec_streamlf_get(VSTREAM_IN, buf, REC_LEN);
	VSTRING_TERMINATE(buf);
	err = mime_state_update(mime_state, rec_type, STR(buf), LEN(buf));
	vstream_fflush(VSTREAM_OUT);
    } while (rec_type > 0);

    /*
     * Error reporting.
     */
    if (err & MIME_ERR_TRUNC_HEADER)
	msg_warn("message header length exceeds safety limit");
    if (err & MIME_ERR_NESTING)
	msg_warn("MIME nesting exceeds safety limit");
    if (err & MIME_ERR_8BIT_IN_HEADER)
	msg_warn("improper use of 8-bit data in message header");
    if (err & MIME_ERR_8BIT_IN_7BIT_BODY)
	msg_warn("improper use of 8-bit data in message body");
    if (err & MIME_ERR_ENCODING_DOMAIN)
	msg_warn("improper message/* or multipart/* encoding domain");

    /*
     * Cleanup.
     */
    if (context.header_checks)
	hbc_header_checks_free(context.header_checks);
    if (context.body_checks)
	hbc_body_checks_free(context.body_checks);
    vstring_free(context.buf);
    mime_state_free(mime_state);
    vstring_free(buf);
    exit(0);
}

#endif
