/*	$NetBSD: cleanup_message_test.c,v 1.2 2026/05/09 18:49:15 christos Exp $	*/

/*++
/* NAME
/*      cleanup_message_test 1t
/* SUMMARY
/*      cleanup_message unit tests
/* SYNOPSIS
/*      ./cleanup_message_test
/* DESCRIPTION
/*      cleanup_message_test runs and logs each configured test, reports if
/*      a test is a PASS or FAIL, and returns an exit status of zero if
/*      all tests are a PASS.
/* LICENSE
/* .ad
/* .fi
/*      The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*      Wietse Venema
/*      porcupine.org
/*--*/

 /*
  * System library.
  */
#include <sys_defs.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

 /*
  * Utility library.
  */
#include <msg.h>
#include <msg_vstream.h>
#include <stringops.h>
#include <vstream.h>
#include <vstring.h>

 /*
  * Global library.
  */
#include <cleanup_user.h>
#include <hfrom_format.h>
#include <mail_params.h>
#include <record.h>
#include <rec_type.h>

 /*
  * Application-specific.
  */
#include <cleanup.h>

 /*
  * Stubs for configuration parameter dependencies.
  */
bool    var_always_add_hdrs;
bool    var_auto_8bit_enc_hdr;
int     var_body_check_len;
bool    var_cleanup_mask_stray_cr_lf;
int     var_dup_filter_limit;
bool    var_force_mime_iconv;
char   *var_full_name_encoding_charset;
char   *var_header_checks;
int     var_hopcount_limit;
char   *var_mimehdr_checks;
char   *var_nesthdr_checks;
char   *var_rcpt_witheld;
bool    var_reqtls_esmtp_hdr;

MAPS   *cleanup_comm_canon_maps;
MAPS   *cleanup_send_canon_maps;
MAPS   *cleanup_rcpt_canon_maps;
MAPS   *cleanup_virt_alias_maps;
int     cleanup_comm_canon_flags;
int     cleanup_send_canon_flags;
int     cleanup_rcpt_canon_flags;
ARGV   *cleanup_masq_domains;
MAPS   *cleanup_header_checks;
MAPS   *cleanup_mimehdr_checks;
MAPS   *cleanup_nesthdr_checks;
MAPS   *cleanup_body_checks;
STRING_LIST *cleanup_masq_exceptions;
char   *var_masq_classes;
MAPS   *cleanup_send_bcc_maps;
MAPS   *cleanup_rcpt_bcc_maps;
MILTERS *cleanup_milters;
int     cleanup_ext_prop_mask;
int     cleanup_hfrom_format;
int     cleanup_masq_flags;
int     cleanup_non_empty_eoh_action;
char   *cleanup_path;
VSTRING *cleanup_reject_chars;
VSTRING *cleanup_strip_chars;

static void test_setup(void)
{
    var_always_add_hdrs = DEF_ALWAYS_ADD_HDRS;
    var_auto_8bit_enc_hdr = DEF_AUTO_8BIT_ENC_HDR;
    var_body_check_len = DEF_BODY_CHECK_LEN;
    var_cleanup_mask_stray_cr_lf = DEF_CLEANUP_MASK_STRAY_CR_LF;
    var_dup_filter_limit = DEF_DUP_FILTER_LIMIT;
    var_force_mime_iconv = DEF_FORCE_MIME_ICONV;
    var_full_name_encoding_charset = DEF_FULL_NAME_ENCODING_CHARSET;
    var_header_checks = DEF_HEADER_CHECKS;
    var_hopcount_limit = DEF_HOPCOUNT_LIMIT;
    var_mimehdr_checks = DEF_MIMEHDR_CHECKS;
    var_nesthdr_checks = DEF_NESTHDR_CHECKS;
    var_rcpt_witheld = DEF_RCPT_WITHELD;
    cleanup_hfrom_format = HFROM_FORMAT_CODE_STD;
    cleanup_masq_flags = (CLEANUP_MASQ_FLAG_ENV_FROM | \
		   CLEANUP_MASQ_FLAG_HDR_FROM | CLEANUP_MASQ_FLAG_HDR_RCPT);
    cleanup_non_empty_eoh_action = NON_EMPTY_EOH_CODE_FIX_QUIETLY;
    var_masq_classes = DEF_MASQ_CLASSES;
    var_drop_hdrs = DEF_DROP_HDRS;
    var_header_limit = 2000;
    var_line_limit = DEF_LINE_LIMIT;
    var_info_log_addr_form = DEF_INFO_LOG_ADDR_FORM;
    var_reqtls_esmtp_hdr = 1;
}

 /*
  * Stubs for cleanup_message.c dependencies.
  */
void    cleanup_extracted(CLEANUP_STATE *state, int type, const char *data,
			          ssize_t len)
{
    msg_panic("cleanup_extracted");
}

int     cleanup_map11_tree(CLEANUP_STATE *state, TOK822 *tree,
			           MAPS *maps, int propagate)
{
    return (0);
}

int     cleanup_masquerade_tree(CLEANUP_STATE *state, TOK822 *tree,
				        ARGV *masq_domains)
{
    return (0);
}

int     cleanup_rewrite_tree(const char *context_name, TOK822 *tree)
{
    return (0);
}

 /*
  * Stubs for cleanup_state.c dependencies.
  */
void    cleanup_envelope(CLEANUP_STATE *state, int type, const char *data,
			         ssize_t len)
{
    msg_panic("cleanup_envelope");
}

void    cleanup_region_done(CLEANUP_STATE *state)
{
}

 /*
  * Intercept cleanup_out_header(), cleanup_out_string(),
  * cleanup_out_format()
  */
int     rec_put(VSTREAM *stream, int type, const char *data, ssize_t len)
{
    if (msg_verbose)
	msg_info("fake_%s: %c '%.*s' %d",
		 __func__, type, (int) len, data, (int) len);

    /*
     * TOO(wietse) envelope records.
     */ if (type == REC_TYPE_NORM)
	vstream_fprintf(stream, "%.*s\n", (int) len, data);
    else if (type == REC_TYPE_CONT)
	vstream_fprintf(stream, "%.*s", (int) len, data);
    return (type);
}

 /*
  * Tests and test cases.
  */
typedef struct TEST_CASE {
    const char *label;			/* identifies test case */
    int     (*action) (const struct TEST_CASE *);
} TEST_CASE;

#define PASS	1
#define FAIL	0

static int test_action(const TEST_CASE *tp, const char *inputs[],
		               int want_errs, const char *want_text)
{
    VSTRING *got_text = vstring_alloc(100);
    VSTREAM *dst = vstream_memopen(got_text, O_WRONLY);
    CLEANUP_STATE *state = 0;
    int     ret = PASS;
    const char *const * cpp;

    if (dst == 0) {
	msg_warn("vstream_memopen: %m");
	ret = FAIL;
    } else {
	state = cleanup_state_alloc((VSTREAM *) 0);
	state->queue_id = mystrdup("queue_id");
	state->flags |= CLEANUP_FLAG_FILTER;
	state->sender = mystrdup("sender");
	state->recip = mystrdup("recip");
	/* Don't add 'missing' headers. */
	state->headers_seen = ~0;

	state->dst = dst;
	state->action = cleanup_message;
	for (cpp = inputs; *cpp; cpp++)
	    state->action(state, REC_TYPE_NORM, *cpp, strlen(*cpp));
	state->action(state, REC_TYPE_XTRA, "", 0);
	(void) vstream_fclose(state->dst);
	state->dst = 0;
	if (state->errs != want_errs) {
	    msg_warn("cleanup_envelope: got: '%s', want: '%s'",
		     cleanup_strerror(state->errs),
		     cleanup_strerror(want_errs));
	    ret = FAIL;
	} else if (want_errs == CLEANUP_STAT_OK
		   && strcmp(vstring_str(got_text), want_text) != 0) {
	    msg_warn("got '%s', want: '%s'", vstring_str(got_text),
		     want_text);
	    ret = FAIL;
	}
    }

    /*
     * Cleanup.
     */
    vstring_free(got_text);
    if (state)
	cleanup_state_free(state);
    return (ret);
}

static int silently_adds_empty_line(const TEST_CASE *tp)
{
    static const char *inputs[] = {
	"Received: text",
	"bad header: text",
	0,
    };
    int     want_errs = CLEANUP_STAT_OK;
    const char *want_text = {
	"Received: text\n"
	"\n"
	"bad header: text\n"
    };

    cleanup_non_empty_eoh_action = NON_EMPTY_EOH_CODE_FIX_QUIETLY;
    return (test_action(tp, inputs, want_errs, want_text));
}

static int adds_informative_header(const TEST_CASE *tp)
{
    static const char *inputs[] = {
	"Received: text",
	"bad header: text",
	0,
    };
    int     want_errs = CLEANUP_STAT_OK;
    const char *want_text = {
	"Received: text\n"
	"MIME-Error: message header was not terminated by empty line\n"
	"\n"
	"bad header: text\n"
    };

    cleanup_non_empty_eoh_action = NON_EMPTY_EOH_CODE_ADD_HDR;
    return (test_action(tp, inputs, want_errs, want_text));
}

static int rejects_non_empty_header_end(const TEST_CASE *tp)
{
    static const char *inputs[] = {
	"Received: text",
	"bad header text",
	0,
    };
    int     want_errs = CLEANUP_STAT_CONT;
    const char *want_text = 0;

    cleanup_non_empty_eoh_action = NON_EMPTY_EOH_CODE_REJECT;
    return (test_action(tp, inputs, want_errs, want_text));
}

static const TEST_CASE test_cases[] = {
    {"silently_adds_empty_line",
	silently_adds_empty_line,
    },
    {"adds_informative_header",
	adds_informative_header,
    },
    {"rejects_non_empty_header_end",
	rejects_non_empty_header_end,
    },
    {0},
};

static NORETURN usage(const char *myname)
{
    msg_fatal("usage: %s [-v]", myname);
}

int     main(int argc, char **argv)
{
    const char *myname = sane_basename((VSTRING *) 0, argv[0]);
    const TEST_CASE *tp;
    int     pass = 0;
    int     fail = 0;
    int     ch;

    /* XXX How to avoid linking in mail_params.o? */

    msg_vstream_init(myname, VSTREAM_ERR);
    while ((ch = GETOPT(argc, argv, "v")) > 0) {
	switch (ch) {
	default:
	    usage(myname);
	case 'v':
	    msg_verbose++;
	    break;
	}
    }

    for (tp = test_cases; tp->label != 0; tp++) {
	test_setup();
	msg_info("RUN  %s", tp->label);
	if (tp->action(tp) != PASS) {
	    fail++;
	    msg_info("FAIL %s", tp->label);
	} else {
	    msg_info("PASS %s", tp->label);
	    pass++;
	}
    }
    msg_info("PASS=%d FAIL=%d", pass, fail);
    exit(fail != 0);
}
