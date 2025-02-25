/*	$NetBSD: dns_rr_test.c,v 1.2 2025/02/25 19:15:44 christos Exp $	*/

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
#include <mymalloc.h>
#include <stringops.h>
#include <vstring.h>

 /*
  * DNS library.
  */
#include <dns.h>

#define STR(x)	vstring_str(x)

 /*
  * Test helpers. TODO: move eq_dns_rr() to testing/dns_rr_testers.c; need to
  * verify that the expected difference is reported, or use a GTEST matcher.
  */

/* print_dns_rr - format as { qname, reply, flags } */

static char *print_dns_rr(VSTRING *buf, DNS_RR *rr)
{
    static VSTRING *tmp;

    if (tmp == 0)
	tmp = vstring_alloc(100);
    vstring_sprintf(buf, "{qname=%s, reply='%s', flags=0x%x}",
		    rr->qname, dns_strrecord(tmp, rr), rr->flags);
    return (STR(buf));
}

/* eq_dns_rr - predicate that two lists are equivalent */

static int eq_dns_rr(DNS_RR *got, DNS_RR *want)
{
    VSTRING *got_buf = 0;
    VSTRING *want_buf = 0;

#define EQ_DNS_RR_RETURN(val) do { \
	if (got_buf) \
	    vstring_free(got_buf); \
	if (want_buf) \
	    vstring_free(want_buf); \
	return (val); \
    } while (0)

    /* Same length. */
    if (got == 0 && want == 0)
	EQ_DNS_RR_RETURN(1);
    if (want == 0) {
	msg_warn("got %s, want null",
		 print_dns_rr(got_buf = vstring_alloc(100), got));
    }
    if (got == 0) {
	msg_warn("got null, want %s",
		 print_dns_rr(want_buf = vstring_alloc(100), want));
	EQ_DNS_RR_RETURN(0);
    }
    /* Same query name, resource record, flags. */
    if (strcmp(print_dns_rr(got_buf = vstring_alloc(100), got),
	       print_dns_rr(want_buf = vstring_alloc(100), want)) != 0) {
	msg_warn("got %s, want %s", STR(want_buf), STR(got_buf));
	EQ_DNS_RR_RETURN(0);
    }
    /* Same children. */
    EQ_DNS_RR_RETURN(eq_dns_rr(got->next, want->next));
}

static int eq_dns_rr_free(DNS_RR *got, DNS_RR *want)
{
    int     res = eq_dns_rr(got, want);

    dns_rr_free(got);
    dns_rr_free(want);
    return (res);
}

 /*
  * Tests and test cases.
  */
typedef struct TEST_CASE {
    const char *label;			/* identifies test case */
    int     (*fn) (void);
} TEST_CASE;

#define PASS    (0)
#define FAIL    (1)

 /*
  * Begin helper tests. TODO: move these to testing/dns_rr_testers_test.c.
  */

static int eq_dns_rr_qname_differ(void)
{
    DNS_RR *got = dns_rr_create("qa", "ra", T_SRV, C_IN, 3600, 1, 25, 1, "mxa", 4);
    DNS_RR *want = dns_rr_copy(got);

    myfree(want->qname);
    want->qname = mystrdup("qb");
    return (!eq_dns_rr_free(got, want));
}

static int eq_dns_rr_reply_differ(void)
{
    DNS_RR *got = dns_rr_create("qa", "ra", T_SRV, C_IN, 3600, 1, 25, 1, "mxa", 4);
    DNS_RR *want = dns_rr_copy(got);

    want->port += 1;
    return (!eq_dns_rr_free(got, want));
}

 /*
  * End helper tests.
  */

 /*
  * Begin DNS_RR tests.
  */

static int eq_dns_rr_flags_differ(void)
{
    DNS_RR *got = dns_rr_create_noport("qa", "ra", T_MX, C_IN, 3600, 1, "mxa", 4);
    DNS_RR *want = dns_rr_copy(got);

    want->flags |= DNS_RR_FLAG_TRUNCATED;
    return (!eq_dns_rr_free(got, want));
}

static int append_to_null_from_null(void)
{
    DNS_RR *got = dns_rr_append((DNS_RR *) 0, (DNS_RR *) 0);
    DNS_RR *want = 0;

    return (eq_dns_rr_free(got, want));
}

static int append_to_elem_from_null(void)
{
    DNS_RR *a = dns_rr_create_noport("qa", "ra", T_MX, C_IN, 3600, 1, "mxa", 4);
    DNS_RR *got, *want;

    got = dns_rr_append(dns_rr_copy(a), (DNS_RR *) 0);

    want = a;

    return (eq_dns_rr_free(got, want));
}

static int appent_to_null_from_elem(void)
{
    DNS_RR *a = dns_rr_create_noport("qa", "ra", T_MX, C_IN, 3600, 1, "mxa", 4);
    DNS_RR *got, *want;

    got = dns_rr_append((DNS_RR *) 0, dns_rr_copy(a));

    want = a;

    return (eq_dns_rr_free(got, want));
}

static int append_to_elem_from_elem(void)
{
    DNS_RR *a = dns_rr_create_noport("qa", "ra", T_MX, C_IN, 3600, 1, "mxa", 4);
    DNS_RR *b = dns_rr_create_noport("qb", "rb", T_MX, C_IN, 3600, 1, "mxb", 4);
    DNS_RR *got, *want;

    got = dns_rr_append(dns_rr_copy(a), dns_rr_copy(b));

    (want = a)->next = b;

    return (eq_dns_rr_free(got, want));
}

static int append_to_elem_from_list(void)
{
    DNS_RR *a = dns_rr_create_noport("qa", "ra", T_MX, C_IN, 3600, 1, "mxa", 4);
    DNS_RR *b = dns_rr_create_noport("qb", "rb", T_MX, C_IN, 3600, 1, "mxb", 4);
    DNS_RR *c = dns_rr_create_noport("qc", "rc", T_MX, C_IN, 3600, 1, "mxc", 4);
    DNS_RR *got, *want;

    got = dns_rr_append(dns_rr_copy(a),
			dns_rr_append(dns_rr_copy(b),
				      dns_rr_copy(c)));

    ((want = a)->next = b)->next = c;

    return (eq_dns_rr_free(got, want));
}

static int append_to_list_from_elem(void)
{
    DNS_RR *a = dns_rr_create_noport("qa", "ra", T_MX, C_IN, 3600, 1, "mxa", 4);
    DNS_RR *b = dns_rr_create_noport("qb", "rb", T_MX, C_IN, 3600, 1, "mxb", 4);
    DNS_RR *c = dns_rr_create_noport("qc", "rc", T_MX, C_IN, 3600, 1, "mxc", 4);
    DNS_RR *got, *want;

    got = dns_rr_append(dns_rr_append(dns_rr_copy(a),
				      dns_rr_copy(b)),
			dns_rr_copy(c));

    ((want = a)->next = b)->next = c;

    return (eq_dns_rr_free(got, want));
}

static int append_to_list_from_list(void)
{
    DNS_RR *a = dns_rr_create_noport("qa", "ra", T_MX, C_IN, 3600, 1, "mxa", 4);
    DNS_RR *b = dns_rr_create_noport("qb", "rb", T_MX, C_IN, 3600, 1, "mxb", 4);
    DNS_RR *c = dns_rr_create_noport("qc", "rc", T_MX, C_IN, 3600, 1, "mxc", 4);
    DNS_RR *d = dns_rr_create_noport("qd", "rd", T_MX, C_IN, 3600, 1, "mxd", 4);
    DNS_RR *got, *want;

    got = dns_rr_append(dns_rr_append(dns_rr_copy(a),
				      dns_rr_copy(b)),
			dns_rr_append(dns_rr_copy(c),
				      dns_rr_copy(d)));

    (((want = a)->next = b)->next = c)->next = d;

    return (eq_dns_rr_free(got, want));
}

static int append_propagates_flags(void)
{
    DNS_RR *a = dns_rr_create_noport("qa", "ra", T_MX, C_IN, 3600, 1, "mxa", 4);
    DNS_RR *b = dns_rr_create_noport("qb", "rb", T_MX, C_IN, 3600, 1, "mxb", 4);
    DNS_RR *c = dns_rr_create_noport("qc", "rc", T_MX, C_IN, 3600, 1, "mxc", 4);
    DNS_RR *d = dns_rr_create_noport("qd", "rd", T_MX, C_IN, 3600, 1, "mxd", 4);
    DNS_RR *left = dns_rr_append(dns_rr_copy(a), dns_rr_copy(b));
    DNS_RR *rite = dns_rr_append(dns_rr_copy(c), dns_rr_copy(d));
    DNS_RR *got, *want, *rr;

    for (rr = rite; rr; rr = rr->next)
	rr->flags |= DNS_RR_FLAG_TRUNCATED;

    got = dns_rr_append(left, rite);

    (((want = a)->next = b)->next = c)->next = d;
    for (rr = want; rr; rr = rr->next)
	rr->flags |= DNS_RR_FLAG_TRUNCATED;

    return (eq_dns_rr_free(got, want));
}

static int append_to_list_from_list_truncate(void)
{
    DNS_RR *a = dns_rr_create_noport("qa", "ra", T_MX, C_IN, 3600, 1, "mxa", 4);
    DNS_RR *b = dns_rr_create_noport("qb", "rb", T_MX, C_IN, 3600, 1, "mxb", 4);
    DNS_RR *c = dns_rr_create_noport("qc", "rc", T_MX, C_IN, 3600, 1, "mxc", 4);
    DNS_RR *d = dns_rr_create_noport("qd", "rd", T_MX, C_IN, 3600, 1, "mxd", 4);
    DNS_RR *got, *want, *rr;

    var_dns_rr_list_limit = 3;

    ((want = dns_rr_copy(a))->next = dns_rr_copy(b))->next = dns_rr_copy(c);
    for (rr = want; rr; rr = rr->next)
	rr->flags |= DNS_RR_FLAG_TRUNCATED;

    got = dns_rr_append(dns_rr_append(a, b),
			dns_rr_append(c, d));

    return (eq_dns_rr_free(got, want));
}

static int append_to_list_from_elem_elem_truncate(void)
{
    DNS_RR *a = dns_rr_create_noport("qa", "ra", T_MX, C_IN, 3600, 1, "mxa", 4);
    DNS_RR *b = dns_rr_create_noport("qb", "rb", T_MX, C_IN, 3600, 1, "mxb", 4);
    DNS_RR *c = dns_rr_create_noport("qc", "rc", T_MX, C_IN, 3600, 1, "mxc", 4);
    DNS_RR *d = dns_rr_create_noport("qd", "rd", T_MX, C_IN, 3600, 1, "mxd", 4);
    DNS_RR *got, *want, *rr;

    var_dns_rr_list_limit = 2;

    (want = dns_rr_copy(a))->next = dns_rr_copy(b);
    for (rr = want; rr; rr = rr->next)
	rr->flags |= DNS_RR_FLAG_TRUNCATED;

    got = dns_rr_append(a, b);
    got = dns_rr_append(got, c);		/* should be logged  */
    got = dns_rr_append(got, d);		/* should be silent */

    return (eq_dns_rr_free(got, want));
}

static int append_to_list_from_elem_truncate(void)
{
    DNS_RR *a = dns_rr_create_noport("qa", "ra", T_MX, C_IN, 3600, 1, "mxa", 4);
    DNS_RR *b = dns_rr_create_noport("qb", "rb", T_MX, C_IN, 3600, 1, "mxb", 4);
    DNS_RR *c = dns_rr_create_noport("qc", "rc", T_MX, C_IN, 3600, 1, "mxc", 4);
    DNS_RR *got, *want, *rr;

    var_dns_rr_list_limit = 2;

    (want = dns_rr_copy(a))->next = dns_rr_copy(b);
    for (rr = want; rr; rr = rr->next)
	rr->flags |= DNS_RR_FLAG_TRUNCATED;

    got = dns_rr_append(dns_rr_append(a, b), c);

    return (eq_dns_rr_free(got, want));
}

static int append_to_elem_from_list_truncate(void)
{
    DNS_RR *a = dns_rr_create_noport("qa", "ra", T_MX, C_IN, 3600, 1, "mxa", 4);
    DNS_RR *b = dns_rr_create_noport("qb", "rb", T_MX, C_IN, 3600, 1, "mxb", 4);
    DNS_RR *c = dns_rr_create_noport("qc", "rc", T_MX, C_IN, 3600, 1, "mxc", 4);
    DNS_RR *got, *want, *rr;

    var_dns_rr_list_limit = 2;

    (want = dns_rr_copy(a))->next = dns_rr_copy(b);
    for (rr = want; rr; rr = rr->next)
	rr->flags |= DNS_RR_FLAG_TRUNCATED;

    got = dns_rr_append(a, dns_rr_append(b, c));

    return (eq_dns_rr_free(got, want));
}

static int append_to_list_from_elem_exact_fit(void)
{
    DNS_RR *a = dns_rr_create_noport("qa", "ra", T_MX, C_IN, 3600, 1, "mxa", 4);
    DNS_RR *b = dns_rr_create_noport("qb", "rb", T_MX, C_IN, 3600, 1, "mxb", 4);
    DNS_RR *c = dns_rr_create_noport("qc", "rc", T_MX, C_IN, 3600, 1, "mxc", 4);
    DNS_RR *got, *want;

    var_dns_rr_list_limit = 3;

    ((want = dns_rr_copy(a))->next = dns_rr_copy(b))->next = dns_rr_copy(c);

    got = dns_rr_append(dns_rr_append(a, b), c);

    return (eq_dns_rr_free(got, want));
}

static int append_to_elem_from_list_exact_fit(void)
{
    DNS_RR *a = dns_rr_create_noport("qa", "ra", T_MX, C_IN, 3600, 1, "mxa", 4);
    DNS_RR *b = dns_rr_create_noport("qb", "rb", T_MX, C_IN, 3600, 1, "mxb", 4);
    DNS_RR *c = dns_rr_create_noport("qc", "rc", T_MX, C_IN, 3600, 1, "mxc", 4);
    DNS_RR *got, *want;

    var_dns_rr_list_limit = 3;

    ((want = dns_rr_copy(a))->next = dns_rr_copy(b))->next = dns_rr_copy(c);

    got = dns_rr_append(a, dns_rr_append(b, c));

    return (eq_dns_rr_free(got, want));
}

static int delete_middle_element(void)
{
    DNS_RR *a = dns_rr_create_noport("qa", "ra", T_MX, C_IN, 3600, 1, "mxa", 4);
    DNS_RR *b = dns_rr_create_noport("qb", "rb", T_MX, C_IN, 3600, 1, "mxb", 4);
    DNS_RR *c = dns_rr_create_noport("qc", "rc", T_MX, C_IN, 3600, 1, "mxc", 4);
    DNS_RR *got, *want, *list;

    ((list = a)->next = b)->next = c;
    (want = dns_rr_copy(a))->next = dns_rr_copy(c);
    got = dns_rr_remove(list, b);

    return (eq_dns_rr_free(got, want));
}

static int delete_first_element(void)
{
    DNS_RR *a = dns_rr_create_noport("qa", "ra", T_MX, C_IN, 3600, 1, "mxa", 4);
    DNS_RR *b = dns_rr_create_noport("qb", "rb", T_MX, C_IN, 3600, 1, "mxb", 4);
    DNS_RR *c = dns_rr_create_noport("qc", "rc", T_MX, C_IN, 3600, 1, "mxc", 4);
    DNS_RR *got, *want, *list;

    ((list = a)->next = b)->next = c;
    (want = dns_rr_copy(b))->next = dns_rr_copy(c);
    got = dns_rr_remove(list, a);

    return (eq_dns_rr_free(got, want));
}

static int delete_last_element(void)
{
    DNS_RR *a = dns_rr_create_noport("qa", "ra", T_MX, C_IN, 3600, 1, "mxa", 4);
    DNS_RR *b = dns_rr_create_noport("qb", "rb", T_MX, C_IN, 3600, 1, "mxb", 4);
    DNS_RR *c = dns_rr_create_noport("qc", "rc", T_MX, C_IN, 3600, 1, "mxc", 4);
    DNS_RR *got, *want, *list;

    ((list = a)->next = b)->next = c;
    (want = dns_rr_copy(a))->next = dns_rr_copy(b);
    got = dns_rr_remove(list, c);

    return (eq_dns_rr_free(got, want));
}

static int detach_middle_element(void)
{
    DNS_RR *a = dns_rr_create_noport("qa", "ra", T_MX, C_IN, 3600, 1, "mxa", 4);
    DNS_RR *b = dns_rr_create_noport("qb", "rb", T_MX, C_IN, 3600, 1, "mxb", 4);
    DNS_RR *c = dns_rr_create_noport("qc", "rc", T_MX, C_IN, 3600, 1, "mxc", 4);
    DNS_RR *got, *want, *list;

    ((list = a)->next = b)->next = c;
    (want = dns_rr_copy(a))->next = dns_rr_copy(c);
    got = dns_rr_detach(list, b);
    dns_rr_free(b);

    return (eq_dns_rr_free(got, want));
}

static int detach_first_element(void)
{
    DNS_RR *a = dns_rr_create_noport("qa", "ra", T_MX, C_IN, 3600, 1, "mxa", 4);
    DNS_RR *b = dns_rr_create_noport("qb", "rb", T_MX, C_IN, 3600, 1, "mxb", 4);
    DNS_RR *c = dns_rr_create_noport("qc", "rc", T_MX, C_IN, 3600, 1, "mxc", 4);
    DNS_RR *got, *want, *list;

    ((list = a)->next = b)->next = c;
    (want = dns_rr_copy(b))->next = dns_rr_copy(c);
    got = dns_rr_detach(list, a);
    dns_rr_free(a);

    return (eq_dns_rr_free(got, want));
}

static int detach_last_element(void)
{
    DNS_RR *a = dns_rr_create_noport("qa", "ra", T_MX, C_IN, 3600, 1, "mxa", 4);
    DNS_RR *b = dns_rr_create_noport("qb", "rb", T_MX, C_IN, 3600, 1, "mxb", 4);
    DNS_RR *c = dns_rr_create_noport("qc", "rc", T_MX, C_IN, 3600, 1, "mxc", 4);
    DNS_RR *got, *want, *list;

    ((list = a)->next = b)->next = c;
    (want = dns_rr_copy(a))->next = dns_rr_copy(b);
    got = dns_rr_detach(list, c);
    dns_rr_free(c);

    return (eq_dns_rr_free(got, want));
}

 /*
  * The test cases.
  */
static const TEST_CASE test_cases[] = {

    /*
     * Test eq_dns_rr; TODO: move to testing/dns_rr_testers_test.c
     */
    "eq_dns_rr qname differ", eq_dns_rr_qname_differ,
    "eq_dns_rr reply differ", eq_dns_rr_reply_differ,
    "eq_dns_rr flags differ", eq_dns_rr_flags_differ,

    /*
     * Test dns_rr_append() without truncation.
     */
    "append to null from null", append_to_null_from_null,
    "append to null from element", appent_to_null_from_elem,
    "append to element from null", append_to_elem_from_null,
    "append to element from element", append_to_elem_from_elem,
    "append to element from list", append_to_elem_from_list,
    "append to list from element", append_to_list_from_elem,
    "append to list from list", append_to_list_from_list,

    /*
     * Test dns_rr_append() flag propagation.
     */
    "append propagates flags", append_propagates_flags,

    /*
     * Test dns_rr_append() with truncation.
     */
    "append to list from list truncate", append_to_list_from_list_truncate,
    "append to list from element element truncate", append_to_list_from_elem_elem_truncate,
    "append to list from element truncate", append_to_list_from_elem_truncate,
    "append to element from list truncate", append_to_elem_from_list_truncate,
    "append to list from element exact fit", append_to_list_from_elem_exact_fit,
    "append to element from list exact fit", append_to_elem_from_list_exact_fit,

    /*
     * TODO: tests for dns_rr_sort(), dns_rr_srv_sort(), dns_rr_shuffle(),
     * etc.
     */
    "delete element from list (middle)", delete_middle_element,
    "delete element from list (first)", delete_first_element,
    "delete element from list (last)", delete_last_element,
    "detach element from list (middle)", detach_middle_element,
    "detach element from list (first)", detach_first_element,
    "detach element from list (last)", detach_last_element,
    0,
};

int     main(int argc, char **argv)
{
    const TEST_CASE *tp;
    int     pass = 0;
    int     fail = 0;
    VSTRING *res_buf = vstring_alloc(100);
    int     saved_limit = var_dns_rr_list_limit;

    msg_vstream_init(sane_basename((VSTRING *) 0, argv[0]), VSTREAM_ERR);

    for (tp = test_cases; tp->label != 0; tp++) {
	msg_info("RUN  %s", tp->label);
	if (tp->fn() == 0) {
	    fail++;
	    msg_info("FAIL %s", tp->label);
	} else {
	    msg_info("PASS %s", tp->label);
	    pass++;
	}
	var_dns_rr_list_limit = saved_limit;
    }
    msg_info("PASS=%d FAIL=%d", pass, fail);
    vstring_free(res_buf);
    exit(fail != 0);
}
