/*	$NetBSD: smtp_reqtls_policy_test.c,v 1.2 2026/05/09 18:49:20 christos Exp $	*/

 /*
  * System library.
  */
#include <sys_defs.h>
#include <stdlib.h>

 /*
  * Utility library.
  */
#include <msg.h>
#include <msg_vstream.h>
#include <stringops.h>

 /*
  * Application-specific.
  */
#include <smtp_reqtls_policy.h>

 /*
  * Tests and test cases.
  */
struct QUERY_REPLY {
    const char *query;
    int     reply;
};

typedef struct TEST_CASE {
    const char *label;
    int     (*action) (const struct TEST_CASE *);
} TEST_CASE;

static int non_regexp_policy_no_error(const TEST_CASE *tp)
{
    const char *ext_policy = "inline:{"
    "{.example = disable},"
    "{.foo.example = enforce},"
    "{bar.foo.example = opportunistic+starttls}},"
    "opportunistic";
    const struct QUERY_REPLY qr[] = {
	{"foo.example", SMTP_REQTLS_POLICY_ACT_DISABLE},
	{"x.foo.example", SMTP_REQTLS_POLICY_ACT_ENFORCE},
	{"bar.foo.example", SMTP_REQTLS_POLICY_ACT_OPP_TLS},
	{"other", SMTP_REQTLS_POLICY_ACT_OPPORTUNISTIC},
	{0},
    };
    SMTP_REQTLS_POLICY *int_policy;
    const struct QUERY_REPLY *qp;
    const char test_origin[] = "test";
    int     got;
    int     errors = 0;

    int_policy = smtp_reqtls_policy_parse(test_origin, ext_policy);
    for (qp = qr; qp->query; qp++) {
	got = smtp_reqtls_policy_eval(int_policy, qp->query);
	if (got != qp->reply) {
	    msg_warn("got result '%d', want: '%d'", got, qp->reply);
	    errors++;
	}
    }
    smtp_reqtls_policy_free(int_policy);
    return (errors == 0);
}

static int non_regexp_policy_error_at_end(const TEST_CASE *tp)
{
    const char *ext_policy = "inline:{"
    "{.foo.example = enforce},"
    "{bar.foo.example = opportunistic+starttls}},"
    "nonsense";
    const struct QUERY_REPLY qr[] = {
	{"x.foo.example", SMTP_REQTLS_POLICY_ACT_ENFORCE},
	{"bar.foo.example", SMTP_REQTLS_POLICY_ACT_OPP_TLS},
	{"other", SMTP_REQTLS_POLICY_ACT_ERROR},
	{0},
    };
    SMTP_REQTLS_POLICY *int_policy;
    const struct QUERY_REPLY *qp;
    const char test_origin[] = "test";
    int     got;
    int     errors = 0;

    int_policy = smtp_reqtls_policy_parse(test_origin, ext_policy);
    for (qp = qr; qp->query; qp++) {
	got = smtp_reqtls_policy_eval(int_policy, qp->query);
	if (got != qp->reply) {
	    msg_warn("got result '%d', want: '%d'", got, qp->reply);
	    errors++;
	}
    }
    smtp_reqtls_policy_free(int_policy);
    return (errors == 0);
}

static int non_regexp_policy_error_at_start(const TEST_CASE *tp)
{
    const char *ext_policy = "nonsense,inline:{"
    "{.foo.example = enforce},"
    "{bar.foo.example = debug}},"
    "nonsense";
    const struct QUERY_REPLY qr[] = {
	{"x.foo.example", SMTP_REQTLS_POLICY_ACT_ERROR},
	{"bar.foo.example", SMTP_REQTLS_POLICY_ACT_ERROR},
	{"other", SMTP_REQTLS_POLICY_ACT_ERROR},
	{0},
    };
    SMTP_REQTLS_POLICY *int_policy;
    const struct QUERY_REPLY *qp;
    const char test_origin[] = "test";
    int     got;
    int     errors = 0;

    int_policy = smtp_reqtls_policy_parse(test_origin, ext_policy);
    for (qp = qr; qp->query; qp++) {
	got = smtp_reqtls_policy_eval(int_policy, qp->query);
	if (got != qp->reply) {
	    msg_warn("got result '%d', want: '%d'", got, qp->reply);
	    errors++;
	}
    }
    smtp_reqtls_policy_free(int_policy);
    return (errors == 0);
}

static int policy_table_lookup_error(const TEST_CASE *tp)
{
    const char *ext_policy = "fail:whatever enforce";
    const struct QUERY_REPLY qr[] = {
	{"x.foo.example", SMTP_REQTLS_POLICY_ACT_ERROR},
	{"other", SMTP_REQTLS_POLICY_ACT_ERROR},
	{0},
    };
    SMTP_REQTLS_POLICY *int_policy;
    const struct QUERY_REPLY *qp;
    const char test_origin[] = "test";
    int     got;
    int     errors = 0;

    int_policy = smtp_reqtls_policy_parse(test_origin, ext_policy);
    for (qp = qr; qp->query; qp++) {
	got = smtp_reqtls_policy_eval(int_policy, qp->query);
	if (got != qp->reply) {
	    msg_warn("got result '%d', want: '%d'", got, qp->reply);
	    errors++;
	}
    }
    smtp_reqtls_policy_free(int_policy);
    return (errors == 0);
}

static int regexp_table_no_error(const TEST_CASE *tp)
{
    const char *ext_policy = "regexp:{{/^foo\\.example$/ enforce}}, opportunistic";
    const struct QUERY_REPLY qr[] = {
	{"foo.example", SMTP_REQTLS_POLICY_ACT_ENFORCE},
	{"x.foo.example", SMTP_REQTLS_POLICY_ACT_OPPORTUNISTIC},
	{"bar.example", SMTP_REQTLS_POLICY_ACT_OPPORTUNISTIC},
	{"other", SMTP_REQTLS_POLICY_ACT_OPPORTUNISTIC},
	{0},
    };
    SMTP_REQTLS_POLICY *int_policy;
    const struct QUERY_REPLY *qp;
    const char test_origin[] = "test";
    int     got;
    int     errors = 0;

    int_policy = smtp_reqtls_policy_parse(test_origin, ext_policy);
    for (qp = qr; qp->query; qp++) {
	got = smtp_reqtls_policy_eval(int_policy, qp->query);
	if (got != qp->reply) {
	    msg_warn("got result '%d', want: '%d'", got, qp->reply);
	    errors++;
	}
    }
    smtp_reqtls_policy_free(int_policy);
    return (errors == 0);
}

static int default_is_enforce(const TEST_CASE *tp)
{
    const char *ext_policy = "regexp:{{/^foo\\.example$/ opportunistic+starttls}}";
    const struct QUERY_REPLY qr[] = {
	{"foo.example", SMTP_REQTLS_POLICY_ACT_OPP_TLS},
	{"x.foo.example", SMTP_REQTLS_POLICY_ACT_ENFORCE},
	{"bar.example", SMTP_REQTLS_POLICY_ACT_ENFORCE},
	{"other", SMTP_REQTLS_POLICY_ACT_ENFORCE},
	{0},
    };
    SMTP_REQTLS_POLICY *int_policy;
    const struct QUERY_REPLY *qp;
    const char test_origin[] = "test";
    int     got;
    int     errors = 0;

    int_policy = smtp_reqtls_policy_parse(test_origin, ext_policy);
    for (qp = qr; qp->query; qp++) {
	got = smtp_reqtls_policy_eval(int_policy, qp->query);
	if (got != qp->reply) {
	    msg_warn("got result '%d', want: '%d'", got, qp->reply);
	    errors++;
	}
    }
    smtp_reqtls_policy_free(int_policy);
    return (errors == 0);
}

static int disallows_nested_table(const TEST_CASE *tp)
{
    const char *ext_policy = "inline:{{foo.example = hash:/no/where}}";
    const struct QUERY_REPLY qr[] = {
	{"foo.example", SMTP_REQTLS_POLICY_ACT_ERROR},
	{"x.foo.example", SMTP_REQTLS_POLICY_ACT_ENFORCE},
	{0},
    };
    SMTP_REQTLS_POLICY *int_policy;
    const struct QUERY_REPLY *qp;
    const char test_origin[] = "test";
    int     got;
    int     errors = 0;

    int_policy = smtp_reqtls_policy_parse(test_origin, ext_policy);
    for (qp = qr; qp->query; qp++) {
	got = smtp_reqtls_policy_eval(int_policy, qp->query);
	if (got != qp->reply) {
	    msg_warn("got result '%d', want: '%d'", got, qp->reply);
	    errors++;
	}
    }
    smtp_reqtls_policy_free(int_policy);
    return (errors == 0);
}

static int disallows_composite_lookup(const TEST_CASE *tp)
{
    const char *ext_policy = "inline:{{foo.example = foo bar}}";
    const struct QUERY_REPLY qr[] = {
	{"foo.example", SMTP_REQTLS_POLICY_ACT_ERROR},
	{"x.foo.example", SMTP_REQTLS_POLICY_ACT_ENFORCE},
	{"example", SMTP_REQTLS_POLICY_ACT_ENFORCE},
	{0},
    };
    SMTP_REQTLS_POLICY *int_policy;
    const struct QUERY_REPLY *qp;
    const char test_origin[] = "test";
    int     got;
    int     errors = 0;

    int_policy = smtp_reqtls_policy_parse(test_origin, ext_policy);
    for (qp = qr; qp->query; qp++) {
	got = smtp_reqtls_policy_eval(int_policy, qp->query);
	if (got != qp->reply) {
	    msg_warn("got result '%d', want: '%d'", got, qp->reply);
	    errors++;
	}
    }
    smtp_reqtls_policy_free(int_policy);
    return (errors == 0);
}

static int converts_good_u_label_query(const TEST_CASE *tp)
{
    const char *ext_policy = "inline:{{foo.xn--1xa.example = opportunistic}} enforce";
    const struct QUERY_REPLY qr[] = {
        {"foo.xn--1xa.example", SMTP_REQTLS_POLICY_ACT_OPPORTUNISTIC},
        {"foo.\xcf\x80.example", SMTP_REQTLS_POLICY_ACT_OPPORTUNISTIC},
        {"x.foo.xn--1xa.example", SMTP_REQTLS_POLICY_ACT_ENFORCE},
        {"x.foo.\xcf\x80.example", SMTP_REQTLS_POLICY_ACT_ENFORCE},
        {"example", SMTP_REQTLS_POLICY_ACT_ENFORCE},
        {0},
    };
    SMTP_REQTLS_POLICY *int_policy;
    const struct QUERY_REPLY *qp;
    const char test_origin[] = "test";
    int     got;
    int     errors = 0;

    int_policy = smtp_reqtls_policy_parse(test_origin, ext_policy);
    for (qp = qr; qp->query; qp++) {
        got = smtp_reqtls_policy_eval(int_policy, qp->query);
        if (got != qp->reply) {
            msg_warn("got result '%d', want: '%d'", got, qp->reply);
            errors++;
        }
    }
    smtp_reqtls_policy_free(int_policy);
    return (errors == 0);
}

static int fails_bad_u_label_query(const TEST_CASE *tp)
{
    const char *ext_policy = "inline:{{foo.xn--1xa.example = opportunistic}} disable";
    const struct QUERY_REPLY qr[] = {
        {"foo.\xcf\x80.example", SMTP_REQTLS_POLICY_ACT_OPPORTUNISTIC},
        {"foo.-\xcf\x80.example", SMTP_REQTLS_POLICY_ACT_ENFORCE},
        {"example", SMTP_REQTLS_POLICY_ACT_DISABLE},
        {0},
    };
    SMTP_REQTLS_POLICY *int_policy;
    const struct QUERY_REPLY *qp;
    const char test_origin[] = "test";
    int     got;
    int     errors = 0;

    int_policy = smtp_reqtls_policy_parse(test_origin, ext_policy);
    for (qp = qr; qp->query; qp++) {
        got = smtp_reqtls_policy_eval(int_policy, qp->query);
        if (got != qp->reply) {
            msg_warn("got result '%d', want: '%d'", got, qp->reply);
            errors++;
        }
    }
    smtp_reqtls_policy_free(int_policy);
    return (errors == 0);
}


static TEST_CASE test_cases[] = {
    "non_regexp_policy_no_error", non_regexp_policy_no_error,
    "non_regexp_policy_error_at_end", non_regexp_policy_error_at_end,
    "non_regexp_policy_error_at_start", non_regexp_policy_error_at_start,
    "policy_table_lookup_error", policy_table_lookup_error,
    "regexp_table_no_error", regexp_table_no_error,
    "default_is_enforce", default_is_enforce,
    "disallows_nested_table", disallows_nested_table,
    "disallows_composite_lookup", disallows_composite_lookup,
    "converts_good_u_label_query", converts_good_u_label_query,
    "fails_bad_u_label_query", fails_bad_u_label_query,
    0,
};

int     main(int argc, char **argv)
{
    const TEST_CASE *tp;
    int     pass = 0;
    int     fail = 0;

    msg_vstream_init(sane_basename((VSTRING *) 0, argv[0]), VSTREAM_ERR);
    util_utf8_enable = 1;

    for (tp = test_cases; tp->label != 0; tp++) {
	msg_info("RUN  %s", tp->label);
	if (tp->action(tp) == 0) {
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
