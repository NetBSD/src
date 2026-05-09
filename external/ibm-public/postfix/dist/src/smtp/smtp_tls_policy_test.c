/*	$NetBSD: smtp_tls_policy_test.c,v 1.1.1.1 2026/05/09 18:39:21 christos Exp $	*/

/*++
/* NAME
/*      smtp_tls_policy_test 1t
/* SUMMARY
/*      smtp_tls_policy unit tests
/* SYNOPSIS
/*      ./smtp_tls_policy_test
/* DESCRIPTION
/*      smtp_tls_policy_test runs and logs each configured test, reports
/*      if a test is a PASS or FAIL, and returns an exit status of zero
/*      if all tests are a PASS.
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
#include <stdlib.h>

 /*
  * Utility library.
  */
#include <argv.h>
#include <msg.h>
#include <msg_vstream.h>
#include <stringops.h>
#include <vstream.h>
#include <vstring.h>

#ifdef USE_TLS

#define PASS    (0)
#define FAIL    (1)

 /*
  * Global library.
  */
#include <mail_params.h>

 /*
  * TLS library.
  */

 /*
  * SMTP client.
  */
#include <smtp.h>

 /*
  * Surrogate dependencies. Parameters are initialized without $name
  * expansion, and can be changed by tests.
  */
char   *var_smtp_tls_policy;
char   *var_smtp_tls_per_site;
char   *var_smtp_tls_ciph;
bool    var_smtp_tls_conn_reuse;
bool    var_smtp_tls_enable_rpk;
char   *var_smtp_tls_level;
int     smtp_host_lookup_mask;
int     var_smtp_cache_conn;
char   *var_smtp_tls_mand_ciph;
char   *var_smtp_tls_mand_excl;
char   *var_smtp_dns_res_opt;
char   *var_smtp_dns_support;
char   *var_smtp_tls_insecure_mx_policy;
bool    var_ign_mx_lookup_err;
bool    var_smtp_enforce_tls;
bool    var_smtp_tls_enforce_peername;
bool    var_smtp_tls_force_tlsa;
char   *var_smtp_tls_fpt_cmatch;
char   *var_smtp_tls_mand_proto;
char   *var_smtp_tls_proto;
char   *var_smtp_tls_sec_cmatch;
char   *var_smtp_tls_sni;
char   *var_smtp_tls_tafile;
char   *var_smtp_tls_vfy_cmatch;
bool    var_smtp_use_tls;
char   *var_smtp_tls_excl_ciph;
bool    var_smtp_tls_enf_sts_mx_pat;
bool    var_smtp_tls_wrappermode;
bool    var_log_tls_feature_status;

 /*
  * Other globals.
  */
int     smtp_mode;
int     smtp_tls_insecure_mx_policy;
unsigned smtp_dns_res_opt;
int     smtp_dns_support;
int     smtp_host_lookup_mask;

 /*
  * Pre-test initializer to make tests independent.
  */
static void test_setup(void)
{
    var_smtp_tls_policy = DEF_SMTP_TLS_POLICY;
    var_smtp_tls_per_site = DEF_SMTP_TLS_PER_SITE;
    var_smtp_tls_ciph = DEF_SMTP_TLS_CIPH;
    var_smtp_tls_conn_reuse = DEF_SMTP_TLS_CONN_REUSE;
    var_smtp_tls_enable_rpk = DEF_SMTP_TLS_ENABLE_RPK;
    var_smtp_tls_level = "may";
    var_smtp_cache_conn = 2;
    var_smtp_tls_mand_ciph = DEF_SMTP_TLS_MAND_CIPH;
    var_smtp_tls_mand_excl = DEF_SMTP_TLS_MAND_EXCL;
    var_smtp_dns_res_opt = DEF_SMTP_DNS_RES_OPT;
    var_smtp_dns_support = DEF_SMTP_DNS_SUPPORT;
    var_smtp_tls_insecure_mx_policy = DEF_SMTP_TLS_INSECURE_MX_POLICY;
    var_ign_mx_lookup_err = DEF_IGN_MX_LOOKUP_ERR;
    var_smtp_enforce_tls = DEF_SMTP_ENFORCE_TLS;
    var_smtp_tls_enforce_peername = DEF_SMTP_TLS_ENFORCE_PN;
    var_smtp_tls_force_tlsa = DEF_SMTP_TLS_FORCE_TLSA;
    var_smtp_tls_fpt_cmatch = DEF_SMTP_TLS_FPT_CMATCH;
    var_smtp_tls_mand_proto = DEF_SMTP_TLS_MAND_PROTO;
    var_smtp_tls_proto = DEF_SMTP_TLS_PROTO;
    var_smtp_tls_sec_cmatch = DEF_SMTP_TLS_SEC_CMATCH;
    var_smtp_tls_sni = DEF_SMTP_TLS_SNI;
    var_smtp_tls_tafile = DEF_SMTP_TLS_TAFILE;
    var_smtp_tls_vfy_cmatch = DEF_SMTP_TLS_VFY_CMATCH;
    var_smtp_use_tls = DEF_SMTP_USE_TLS;
    var_smtp_tls_excl_ciph = DEF_SMTP_TLS_EXCL_CIPH;
    var_smtp_tls_enf_sts_mx_pat = 1;
    var_smtp_tls_wrappermode = 0;
    var_tls_required_enable = 0;
    var_log_tls_feature_status = 1;

    smtp_mode = 1;

    smtp_tls_policy_cache_flush();
}

 /*
  * Post-test finalizer to help memory leak tests.
  */
static void test_teardown(void)
{
    smtp_tls_policy_cache_flush();
}

 /*
  * Test helpers. TODO(wietse) adopt PTEST which does a nicer job.
  */
static int match_int(const char *what, int want, int got)
{
    if (want != got) {
	msg_warn("%s: got %d, want %d", what, got, want);
	return (0);
    }
    return (1);
}

#define STR_OR_NULL(s) ((s)? (s) : "NULL")

static int match_cstr(const char *what, const char *want, const char *got)
{
    if (!want != !got) {
	msg_warn("%s: got '%s', want '%s'", what, STR_OR_NULL(got),
		 STR_OR_NULL(want));
	return (0);
    }
    if (want != 0 && strcmp(got, want) != 0) {
	msg_warn("%s: got '%s', want '%s'", what, got, want);
	return (0);
    }
    return (1);
}

#define WANT_ARGV_MAX	5

struct WANT_ARGV {
    ssize_t argc;
    char   *argv[WANT_ARGV_MAX];
};

#define ARGV_OR_NULL(a) ((a)? ("argv") : "NULL")

static int match_argv(const char *what, const struct WANT_ARGV * want,
		              const ARGV *got)
{
    if (!want != !got) {
	msg_warn("%s: got '%s', want '%s'", what, ARGV_OR_NULL(got),
		 ARGV_OR_NULL(want));
	return (0);
    }
    if (match_int("argc", want->argc, got->argc) == 0) {
	return (0);
    } else {
	VSTRING *buf;
	int     idx;
	int     equal;

	buf = vstring_alloc(100);
	for (equal = 1, idx = 0; idx < want->argc; idx++) {
	    vstring_sprintf(buf, "%s->argv[%d]", what, idx);
	    if (match_cstr(STR(buf), want->argv[idx], got->argv[idx]) == 0)
		equal = 0;
	}
	vstring_free(buf);
	return (equal);
    }
}

 /*
  * Limited policy for STS tests.
  */
struct WANT_SMTP_TLS_POLICY {
    int     level;			/* TLS enforcement level */
#if 0
    char   *protocols;			/* Acceptable SSL protocols */
    char   *grade;			/* Cipher grade: "export", ... */
    VSTRING *exclusions;		/* Excluded SSL ciphers */
    char   *protocols;			/* Acceptable SSL protocols */
    char   *grade;			/* Cipher grade: "export", ... */
    VSTRING *exclusions;		/* Excluded SSL ciphers */
#endif
    struct WANT_ARGV matchargv;		/* Cert match patterns */
#if 0
    DSN_BUF *why;			/* Lookup error status */
    TLS_DANE *dane;			/* DANE TLSA digests */
#endif
    char   *sni;			/* Optional SNI name when not DANE */
#if 0
    int     conn_reuse;			/* enable connection reuse */
    int     enable_rpk;			/* Enable server->client RPK */
#endif
    int     ext_policy_ttl;		/* TTL from DNS etc. */
    char   *ext_policy_type;		/* (sts) */
    struct WANT_ARGV ext_policy_strings;/* policy strings from DNS etc. */
    char   *ext_policy_domain;		/* policy scope */
    struct WANT_ARGV ext_mx_host_patterns;	/* (sts) MX host patterns */
    char   *ext_policy_failure;		/* (sts) policy failure */
};

#define POLICY_OR_NULL(p) ((p)? ("policy") : "NULL")

static int match_smtp_tls_policy(const char *what,
			           const struct WANT_SMTP_TLS_POLICY * want,
				         const SMTP_TLS_POLICY *got)
{
    int     equal = 1;

    if (!want != !got) {
	msg_warn("%s: got '%s', want '%s'", what, POLICY_OR_NULL(got),
		 POLICY_OR_NULL(want));
	return (0);
    }
    if (match_int("level", want->level, got->level) == 0) {
	msg_warn("%s->level mismatch", what);
	equal = 0;
    }
    if (match_argv("matchargv", &want->matchargv, got->matchargv) == 0) {
	msg_warn("%s->matchargv mismatch", what);
	equal = 0;
    }
    if (match_cstr("sni", want->sni, got->sni) == 0) {
	msg_warn("%s->sni mismatch", what);
	equal = 0;
    }
    if (match_int("ext_policy_ttl", want->ext_policy_ttl, got->ext_policy_ttl) == 0) {
	msg_warn("%s->ext_policy_ttl mismatch", what);
	equal = 0;
    }
    if (match_cstr("ext_policy_type", want->ext_policy_type,
		   got->ext_policy_type) == 0) {
	msg_warn("%s->ext_policy_type mismatch", what);
	equal = 0;
    }
    if (match_argv("ext_policy_strings", &want->ext_policy_strings,
		   got->ext_policy_strings) == 0) {
	msg_warn("%s->ext_policy_strings mismatch", what);
	equal = 0;
    }
    if (match_cstr("ext_policy_domain", want->ext_policy_domain,
		   got->ext_policy_domain) == 0) {
	msg_warn("%s->ext_policy_domain mismatch", what);
	equal = 0;
    }
    if (match_argv("ext_mx_host_patterns", &want->ext_mx_host_patterns,
		   got->ext_mx_host_patterns) == 0) {
	msg_warn("%s->ext_mx_host_patterns mismatch", what);
	equal = 0;
    }
    if (match_cstr("ext_policy_failure", want->ext_policy_failure,
		   got->ext_policy_failure) == 0) {
	msg_warn("%s->ext_policy_failure mismatch", what);
	equal = 0;
    }
    return (equal);
}

 /*
  * Test structure. Some tests may bring their own.
  */
typedef struct TEST_CASE {
    const char *label;
    int     (*action) (const struct TEST_CASE *);
} TEST_CASE;

 /*
  * Verify that policies from an STS plugin are converted into Postfix TLS
  * policies that match a certificate against the server hostname, and that
  * other policy attributes are stored appropriately. A later test will
  * verify that MX hostnames are matched against the STS policy MX hostname
  * patterns.
  */
static int sts_policy_smoke_test(const struct TEST_CASE *tp)
{
    var_smtp_tls_policy =
    "static:{secure match=example:.example "
    "servername=example policy_ttl=123 "
    "policy_type=sts policy_string=one "
    "policy_string=two policy_domain=example "
    "mx_host_pattern=example mx_host_pattern=*.example}";
    static const struct WANT_SMTP_TLS_POLICY want_policy = {
	.level = TLS_LEV_SECURE,
	.matchargv = {.argc = 1,.argv = {"hostname"}},
	.sni = "example",
	.ext_policy_ttl = 123,
	.ext_policy_type = "sts",
	.ext_policy_strings = {.argc = 2,.argv = {"one", "two"}},
	.ext_policy_domain = "example",
	.ext_mx_host_patterns = {.argc = 2,.argv = {"example", "*.example"}}
    };
    SMTP_STATE *state = smtp_state_alloc();
    const char *domain = "example";
    const char *host = "mail.example";
    const char *addr = "10.0.1.1";
    int     port = 25;
    int     match;

    var_smtp_tls_enf_sts_mx_pat = 1;
    var_smtp_tls_level = "secure";
    smtp_tls_list_init();
    SMTP_ITER_INIT(state->iterator, domain, host, addr, port, state);
    if ((match = smtp_tls_policy_cache_query(state->why, state->tls,
					     state->iterator)) == 0) {
	msg_warn("smtp_tls_policy_cache_query failed: %s",
		 STR(state->why->reason));
    } else {
	match = match_smtp_tls_policy("policy", &want_policy, state->tls);
    }
    smtp_state_free(state);
    smtp_tls_policy_cache_flush();
    return (match ? PASS : FAIL);
}

 /*
  * Verify that the historical support for policies from STS plugins is still
  * available. STS plugins generate a policy that will match a certificate
  * against all MX patterns, after converting a pattern "*.domain" to
  * ".domain", and do not constrain the allowed MX hostnames.
  */
static int obs_sts_policy_smoke_test(const struct TEST_CASE *tp)
{
    var_smtp_tls_policy =
    "static:{secure match=example:.example "
    "servername=example policy_ttl=123 "
    "policy_type=sts policy_string=one "
    "policy_string=two policy_domain=example "
    "mx_host_pattern=example mx_host_pattern=*.example}";
    static const struct WANT_SMTP_TLS_POLICY want_policy = {
	.level = TLS_LEV_SECURE,
	.matchargv = {.argc = 2,.argv = {"example", ".example"}},
	.sni = "example",
	.ext_policy_ttl = 123,
	.ext_policy_type = "sts",
	.ext_policy_strings = {.argc = 2,.argv = {"one", "two"}},
	.ext_policy_domain = "example",
	.ext_mx_host_patterns = {.argc = 2,.argv = {"example", "*.example"}}
    };
    SMTP_STATE *state = smtp_state_alloc();
    const char *domain = "example";
    const char *host = "mail.example";
    const char *addr = "10.0.1.1";
    int     port = 25;
    int     match;

    var_smtp_tls_enf_sts_mx_pat = 0;
    var_smtp_tls_level = "secure";
    smtp_tls_list_init();
    SMTP_ITER_INIT(state->iterator, domain, host, addr, port, state);
    if ((match = smtp_tls_policy_cache_query(state->why, state->tls,
					     state->iterator)) == 0) {
	msg_warn("smtp_tls_policy_cache_query failed: %s",
		 STR(state->why->reason));
    } else {
	match = match_smtp_tls_policy("policy", &want_policy, state->tls);
    }
    smtp_state_free(state);
    smtp_tls_policy_cache_flush();
    return (match ? PASS : FAIL);
}

 /*
  * Test the MX host authorization constraints.
  */
static int test_hostname_authorization(const struct TEST_CASE *tp)
{
    var_smtp_tls_policy =
    "static:{secure match=example.com:.example.com "
    "servername=example.com policy_ttl=123 "
    "policy_type=sts policy_string=one "
    "policy_string=two policy_domain=example.com "
    "mx_host_pattern=example.com mx_host_pattern=*.example.com}";
    static const struct WANT_SMTP_TLS_POLICY want_policy = {
	.level = TLS_LEV_SECURE,
	.matchargv = {.argc = 1,.argv = {"hostname"}},
	.sni = "example.com",
	.ext_policy_ttl = 123,
	.ext_policy_type = "sts",
	.ext_policy_strings = {.argc = 2,.argv = {"one", "two"}},
	.ext_policy_domain = "example.com",
	.ext_mx_host_patterns = {.argc = 2,.argv = {"example.com", "*.example.com"}}
    };
    SMTP_STATE *state = smtp_state_alloc();
    const char *domain = "example.com";
    const char *host = "mail.example.com";
    const char *addr = "10.0.1.1";
    int     port = 25;
    int     match;
    static const char *permit_domains[] = {
	"example.com", "mail.example.com", 0,
    };
    static const char *reject_domains[] = {
	".example.com", "foo.bar.example.com", 0,
    };
    const char *const * cpp;

    var_smtp_tls_enf_sts_mx_pat = 1;
    var_smtp_tls_level = "secure";
    smtp_tls_list_init();
    SMTP_ITER_INIT(state->iterator, domain, host, addr, port, state);
    if ((match = smtp_tls_policy_cache_query(state->why, state->tls,
					     state->iterator)) == 0) {
	msg_warn("smtp_tls_policy_cache_query failed: %s",
		 STR(state->why->reason));
    } else {
	match = match_smtp_tls_policy("policy", &want_policy, state->tls);
    }
    if (match == 0)
	return (FAIL);

    /* Verify that 'good' MX host names are authorized. */
    for (cpp = permit_domains; *cpp; cpp++) {
	if (!smtp_tls_authorize_mx_hostname(state->tls, *cpp)) {
	    msg_warn("hostname '%s' is not authorized", *cpp);
	    match = 0;
	}
    }
    /* Verify that 'wrong' MX host names are not authorized. */
    for (cpp = reject_domains; *cpp; cpp++) {
	if (smtp_tls_authorize_mx_hostname(state->tls, *cpp)) {
	    msg_warn("hostname '%s' is authorized", *cpp);
	    match = 0;
	}
    }

    smtp_state_free(state);
    smtp_tls_policy_cache_flush();
    return (match ? PASS : FAIL);
}

static int test_tls_reqd_no_sans_header(const struct TEST_CASE *tp)
{
    SMTP_STATE *state = smtp_state_alloc();
    const char *domain = "example.com";
    const char *host = "mail.example.com";
    const char *addr = "10.0.1.1";
    int     port = 25;
    int     want_level;
    int     ret = FAIL;

    var_smtp_tls_level = "secure";
    var_smtp_tls_policy = "static:none";

    /* Test-dependent. */
    state->request = &(DELIVER_REQUEST) {
	.sendopts = 0
    };
    var_smtp_tls_wrappermode = 1;
    var_tls_required_enable = 1;
    want_level = TLS_LEV_NONE;

    smtp_tls_list_init();
    SMTP_ITER_INIT(state->iterator, domain, host, addr, port, state);
    if (smtp_tls_policy_cache_query(state->why, state->tls,
				    state->iterator) == 0) {
	msg_warn("smtp_tls_policy_cache_query failed: %s",
		 STR(state->why->reason));
    } else if (state->tls->level != want_level) {
	msg_warn("got TLS level '%s', want '%s'",
	       str_tls_level(state->tls->level), str_tls_level(want_level));
    } else {
	ret = PASS;
    }
    smtp_tls_policy_cache_flush();
    smtp_state_free(state);
    return (ret);
}

static int test_tls_reqd_no_with_wrappermode(const struct TEST_CASE *tp)
{
    SMTP_STATE *state = smtp_state_alloc();
    const char *domain = "example.com";
    const char *host = "mail.example.com";
    const char *addr = "10.0.1.1";
    int     port = 25;
    int     want_level;
    int     ret = FAIL;

    var_smtp_tls_level = "secure";
    var_smtp_tls_policy = "static:none";

    /* Test-dependent. */
    state->request = &(DELIVER_REQUEST) {
	.sendopts = SOPT_REQUIRETLS_HEADER
    };
    var_smtp_tls_wrappermode = 1;
    var_tls_required_enable = 1;
    want_level = TLS_LEV_ENCRYPT;

    smtp_tls_list_init();
    SMTP_ITER_INIT(state->iterator, domain, host, addr, port, state);
    if (smtp_tls_policy_cache_query(state->why, state->tls,
				    state->iterator) == 0) {
	msg_warn("smtp_tls_policy_cache_query failed: %s",
		 STR(state->why->reason));
    } else if (state->tls->level != want_level) {
	msg_warn("got TLS level '%s', want '%s'",
	       str_tls_level(state->tls->level), str_tls_level(want_level));
    } else {
	ret = PASS;
    }
    smtp_tls_policy_cache_flush();
    smtp_state_free(state);
    return (ret);
}

static int test_tls_reqd_no_sans_wrappermode(const struct TEST_CASE *tp)
{
    SMTP_STATE *state = smtp_state_alloc();
    const char *domain = "example.com";
    const char *host = "mail.example.com";
    const char *addr = "10.0.1.1";
    int     port = 25;
    int     want_level;
    int     ret = FAIL;

    var_smtp_tls_level = "secure";
    var_smtp_tls_policy = "static:none";

    /* Test-dependent. */
    state->request = &(DELIVER_REQUEST) {
	.sendopts = SOPT_REQUIRETLS_HEADER
    };
    var_smtp_tls_wrappermode = 0;
    var_tls_required_enable = 1;
    want_level = TLS_LEV_MAY;

    smtp_tls_list_init();
    SMTP_ITER_INIT(state->iterator, domain, host, addr, port, state);
    if (smtp_tls_policy_cache_query(state->why, state->tls,
				    state->iterator) == 0) {
	msg_warn("smtp_tls_policy_cache_query failed: %s",
		 STR(state->why->reason));
    } else if (state->tls->level != want_level) {
	msg_warn("got TLS level '%s', want '%s'",
	       str_tls_level(state->tls->level), str_tls_level(want_level));
    } else {
	ret = PASS;
    }
    smtp_tls_policy_cache_flush();
    smtp_state_free(state);
    return (ret);
}

static const struct TEST_CASE test_cases[] = {
    {"sts_policy_smoke_test", sts_policy_smoke_test,},
    {"obs_sts_policy_smoke_test", obs_sts_policy_smoke_test,},
    {"test_hostname_authorization", test_hostname_authorization},
    {"test_tls_reqd_no_sans_header", test_tls_reqd_no_sans_header},
    {"test_tls_reqd_no_with_wrappermode", test_tls_reqd_no_with_wrappermode},
    {"test_tls_reqd_no_sans_wrappermode", test_tls_reqd_no_sans_wrappermode},
    {0},
};

int     main(int argc, char **argv)
{
    static int tests_passed = 0;
    static int tests_failed = 0;
    const struct TEST_CASE *tp;

    msg_vstream_init(sane_basename((VSTRING *) 0, argv[0]), VSTREAM_ERR);

    for (tp = test_cases; tp->label; tp++) {
	msg_info("RUN  %s", tp->label);
	test_setup();
	if (tp->action(tp) == PASS) {
	    msg_info("PASS %s", tp->label);
	    tests_passed += 1;
	} else {
	    msg_info("FAIL %s", tp->label);
	    tests_failed += 1;
	}
	test_teardown();
    }
    msg_info("PASS=%d FAIL=%d", tests_passed, tests_failed);
    exit(tests_failed != 0);
}

#else

int     main(int argc, char **argv)
{
    msg_fatal("this program requires `#define USE_TLS'");
}

#endif
