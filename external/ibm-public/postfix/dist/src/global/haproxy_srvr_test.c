/*	$NetBSD: haproxy_srvr_test.c,v 1.1.1.1 2026/05/09 18:39:18 christos Exp $	*/

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
#include <myaddrinfo.h>
#include <sock_addr.h>
#include <stringops.h>
#include <vstring.h>

 /*
  * Global library.
  */
#define _HAPROXY_SRVR_INTERNAL_
#include <haproxy_srvr.h>

 /*
  * Application-specific.
  */
#define STR_OR_NULL(str) ((str) ? (str) : "(null)")

 /*
  * Test cases with inputs and expected outputs. A request may contain
  * trailing garbage, and it may be too short. A v1 request may also contain
  * malformed address or port information. TODO(wietse) add unit test with
  * different inet_protocols setting. See normalize_v4mapped_addr_test.c.
  */
typedef struct TEST_CASE {
    const char *haproxy_request;	/* v1 or v2 request including thrash */
    ssize_t haproxy_req_len;		/* request length including thrash */
    ssize_t exp_req_len;		/* parsed request length */
    int     exp_non_proxy;		/* request is not proxied */
    const char *exp_return;		/* expected error string */
    const char *exp_client_addr;	/* expected client address string */
    const char *exp_server_addr;	/* expected client port string */
    const char *exp_client_port;	/* expected client address string */
    const char *exp_server_port;	/* expected server port string */
} TEST_CASE;

static TEST_CASE v1_test_cases[] = {
    /* IPv6. */
    {"PROXY TCP6 fc:00:00:00:1:2:3:4 fc:00:00:00:4:3:2:1 123 321\n", 0, 0, 0, 0, "fc::1:2:3:4", "fc::4:3:2:1", "123", "321"},
    {"PROXY TCP6 FC:00:00:00:1:2:3:4 FC:00:00:00:4:3:2:1 123 321\n", 0, 0, 0, 0, "fc::1:2:3:4", "fc::4:3:2:1", "123", "321"},
    {"PROXY TCP6 1.2.3.4 4.3.2.1 123 321\n", 0, 0, 0, "bad or missing client address"},
    {"PROXY TCP6 fc:00:00:00:1:2:3:4 4.3.2.1 123 321\n", 0, 0, 0, "bad or missing server address"},
    /* IPv4 in IPv6. */
    {"PROXY TCP6 ::ffff:1.2.3.4 ::ffff:4.3.2.1 123 321\n", 0, 0, 0, 0, "1.2.3.4", "4.3.2.1", "123", "321"},
    {"PROXY TCP6 ::FFFF:1.2.3.4 ::FFFF:4.3.2.1 123 321\n", 0, 0, 0, 0, "1.2.3.4", "4.3.2.1", "123", "321"},
    {"PROXY TCP4 ::ffff:1.2.3.4 ::ffff:4.3.2.1 123 321\n", 0, 0, 0, "bad or missing client address"},
    {"PROXY TCP4 1.2.3.4 ::ffff:4.3.2.1 123 321\n", 0, 0, 0, "bad or missing server address"},
    /* IPv4. */
    {"PROXY TCP4 1.2.3.4 4.3.2.1 123 321\n", 0, 0, 0, 0, "1.2.3.4", "4.3.2.1", "123", "321"},
    {"PROXY TCP4 01.02.03.04 04.03.02.01 123 321\n", 0, 0, 0, 0, "1.2.3.4", "4.3.2.1", "123", "321"},
    {"PROXY TCP4 1.2.3.4 4.3.2.1 123456 321\n", 0, 0, 0, "bad or missing client port"},
    {"PROXY TCP4 1.2.3.4 4.3.2.1 123 654321\n", 0, 0, 0, "bad or missing server port"},
    {"PROXY TCP4 1.2.3.4 4.3.2.1 0123 321\n", 0, 0, 0, "bad or missing client port"},
    {"PROXY TCP4 1.2.3.4 4.3.2.1 123 0321\n", 0, 0, 0, "bad or missing server port"},
    /* Missing fields. */
    {"PROXY TCP6 fc:00:00:00:1:2:3:4 fc:00:00:00:4:3:2:1 123\n", 0, 0, 0, "bad or missing server port"},
    {"PROXY TCP6 fc:00:00:00:1:2:3:4 fc:00:00:00:4:3:2:1\n", 0, 0, 0, "bad or missing client port"},
    {"PROXY TCP6 fc:00:00:00:1:2:3:4\n", 0, 0, 0, "bad or missing server address"},
    {"PROXY TCP6\n", 0, 0, 0, "bad or missing client address"},
    {"PROXY TCP4 1.2.3.4 4.3.2.1 123\n", 0, 0, 0, "bad or missing server port"},
    {"PROXY TCP4 1.2.3.4 4.3.2.1\n", 0, 0, 0, "bad or missing client port"},
    {"PROXY TCP4 1.2.3.4\n", 0, 0, 0, "bad or missing server address"},
    {"PROXY TCP4\n", 0, 0, 0, "bad or missing client address"},
    /* Other. */
    {"PROXY BLAH\n", 0, 0, 0, "bad or missing protocol type"},
    {"PROXY\n", 0, 0, 0, "short protocol header"},
    {"BLAH\n", 0, 0, 0, "short protocol header"},
    {"\n", 0, 0, 0, "short protocol header"},
    {"", 0, 0, 0, "short protocol header"},
    0,
};

static struct proxy_hdr_v2 v2_local_request = {
    PP2_SIGNATURE, PP2_VERSION | PP2_CMD_LOCAL,
};
static TEST_CASE v2_non_proxy_test = {
    (char *) &v2_local_request, PP2_HEADER_LEN, PP2_HEADER_LEN, 1,
};

#define STR(x)	vstring_str(x)
#define LEN(x)	VSTRING_LEN(x)

#define DO_SOCKADDR_OUTPUT	1
#define NO_SOCKADDR_OUTPUT	0

static int evaluate_sockaddr(const char *which, struct sockaddr *sa,
			        SOCKADDR_SIZE sa_len, const char *want_addr,
			             const char *want_port)
{
    MAI_HOSTADDR_STR act_addr;
    MAI_SERVPORT_STR act_port;
    int     err;
    int     failed = 0;

    if ((err = sockaddr_to_hostaddr(sa, sa_len, &act_addr, &act_port, 0)) != 0) {
	msg_warn("sockaddr_to_hostaddr: %s", MAI_STRERROR(err));
	return (1);
    }
    if (strcmp(act_addr.buf, want_addr) != 0) {
	msg_warn("got %s address '%s', want '%s'",
		 which, act_addr.buf, want_addr);
	failed = 1;
    }
    if (strcmp(act_port.buf, want_port) != 0) {
	msg_warn("got %s port '%s', want '%s'",
		 which, act_port.buf, want_port);
	failed = 1;
    }
    return (failed);
}

/* evaluate_test_case - evaluate one base or mutated test case */

static int evaluate_test_case(const char *test_label,
			              const TEST_CASE *test_case,
			              int want_sockaddr_output)
{
    /* Actual results. */
    const char *act_return;
    ssize_t act_req_len;
    int     act_non_proxy;
    MAI_HOSTADDR_STR act_smtp_client_addr;
    MAI_HOSTADDR_STR act_smtp_server_addr;
    MAI_SERVPORT_STR act_smtp_client_port;
    MAI_SERVPORT_STR act_smtp_server_port;
    int     test_failed;

    struct sockaddr_storage client_ss;
    struct sockaddr *client_sa;
    SOCKADDR_SIZE client_sa_len;

    struct sockaddr_storage server_ss;
    struct sockaddr *server_sa;
    SOCKADDR_SIZE server_sa_len;

    if (msg_verbose)
	msg_info("test case=%s exp_client_addr=%s exp_server_addr=%s "
		 "exp_client_port=%s exp_server_port=%s",
		 test_label, STR_OR_NULL(test_case->exp_client_addr),
		 STR_OR_NULL(test_case->exp_server_addr),
		 STR_OR_NULL(test_case->exp_client_port),
		 STR_OR_NULL(test_case->exp_server_port));

    if (want_sockaddr_output) {
	client_sa = (struct sockaddr *) &client_ss;
	client_sa_len = sizeof(client_ss);
	server_sa = (struct sockaddr *) &server_ss;
	server_sa_len = sizeof(server_ss);
    } else {
	client_sa = 0;
	client_sa_len = 0;
	server_sa = 0;
	server_sa_len = 0;
    }

    /*
     * Start the test.
     */
    test_failed = 0;
    act_req_len = test_case->haproxy_req_len;
    act_return =
	haproxy_srvr_parse_sa(test_case->haproxy_request, &act_req_len,
			      &act_non_proxy,
			      &act_smtp_client_addr, &act_smtp_client_port,
			      &act_smtp_server_addr, &act_smtp_server_port,
			      client_sa, &client_sa_len,
			      server_sa, &server_sa_len);
    if (act_return != test_case->exp_return
    && strcmp(STR_OR_NULL(act_return), STR_OR_NULL(test_case->exp_return))) {
	msg_warn("test case %s return expected=>%s< actual=>%s<",
		 test_label, STR_OR_NULL(test_case->exp_return),
		 STR_OR_NULL(act_return));
	test_failed = 1;
	return (test_failed);
    }
    if (act_req_len != test_case->exp_req_len) {
	msg_warn("test case %s str_len expected=%ld actual=%ld",
		 test_label,
		 (long) test_case->exp_req_len, (long) act_req_len);
	test_failed = 1;
	return (test_failed);
    }
    if (act_non_proxy != test_case->exp_non_proxy) {
	msg_warn("test case %s non_proxy expected=%d actual=%d",
		 test_label,
		 test_case->exp_non_proxy, act_non_proxy);
	test_failed = 1;
	return (test_failed);
    }
    if (test_case->exp_non_proxy || test_case->exp_return != 0)
	/* No expected address/port results. */
	return (test_failed);

    /*
     * Compare address/port results against expected results.
     */
    if (strcmp(test_case->exp_client_addr, act_smtp_client_addr.buf)) {
	msg_warn("test case %s client_addr  expected=%s actual=%s",
		 test_label,
		 test_case->exp_client_addr, act_smtp_client_addr.buf);
	test_failed = 1;
    }
    if (strcmp(test_case->exp_server_addr, act_smtp_server_addr.buf)) {
	msg_warn("test case %s server_addr  expected=%s actual=%s",
		 test_label,
		 test_case->exp_server_addr, act_smtp_server_addr.buf);
	test_failed = 1;
    }
    if (strcmp(test_case->exp_client_port, act_smtp_client_port.buf)) {
	msg_warn("test case %s client_port  expected=%s actual=%s",
		 test_label,
		 test_case->exp_client_port, act_smtp_client_port.buf);
	test_failed = 1;
    }
    if (strcmp(test_case->exp_server_port, act_smtp_server_port.buf)) {
	msg_warn("test case %s server_port  expected=%s actual=%s",
		 test_label,
		 test_case->exp_server_port, act_smtp_server_port.buf);
	test_failed = 1;
    }
    if (want_sockaddr_output) {
	if (evaluate_sockaddr("client", client_sa, client_sa_len,
			      test_case->exp_client_addr,
			      test_case->exp_client_port) != 0
	    || evaluate_sockaddr("server", server_sa, server_sa_len,
				 test_case->exp_server_addr,
				 test_case->exp_server_port) != 0)
	    test_failed = 1;
    }
    return (test_failed);
}

/* convert_v1_proxy_req_to_v2 - convert well-formed v1 proxy request to v2 */

static void convert_v1_proxy_req_to_v2(VSTRING *buf, const char *req,
				               ssize_t req_len)
{
    const char myname[] = "convert_v1_proxy_req_to_v2";
    const char *err;
    int     non_proxy;
    MAI_HOSTADDR_STR smtp_client_addr;
    MAI_SERVPORT_STR smtp_client_port;
    MAI_HOSTADDR_STR smtp_server_addr;
    MAI_SERVPORT_STR smtp_server_port;
    struct proxy_hdr_v2 *hdr_v2;
    struct addrinfo *src_res;
    struct addrinfo *dst_res;

    /*
     * Allocate buffer space for the largest possible protocol header, so we
     * don't have to worry about hidden realloc() calls.
     */
    VSTRING_RESET(buf);
    VSTRING_SPACE(buf, sizeof(struct proxy_hdr_v2));
    hdr_v2 = (struct proxy_hdr_v2 *) STR(buf);

    /*
     * Fill in the header,
     */
    memcpy(hdr_v2->sig, PP2_SIGNATURE, PP2_SIGNATURE_LEN);
    hdr_v2->ver_cmd = PP2_VERSION | PP2_CMD_PROXY;
    if ((err = haproxy_srvr_parse(req, &req_len, &non_proxy, &smtp_client_addr,
				  &smtp_client_port, &smtp_server_addr,
				  &smtp_server_port)) != 0 || non_proxy)
	msg_fatal("%s: malformed or non-proxy request: %s",
		  myname, req);

    if (hostaddr_to_sockaddr(smtp_client_addr.buf, smtp_client_port.buf, 0,
			     &src_res) != 0)
	msg_fatal("%s: unable to convert source address %s port %s",
		  myname, smtp_client_addr.buf, smtp_client_port.buf);
    if (hostaddr_to_sockaddr(smtp_server_addr.buf, smtp_server_port.buf, 0,
			     &dst_res) != 0)
	msg_fatal("%s: unable to convert destination address %s port %s",
		  myname, smtp_server_addr.buf, smtp_server_port.buf);
    if (src_res->ai_family != dst_res->ai_family)
	msg_fatal("%s: mixed source/destination address families", myname);
#ifdef AF_INET6
    if (src_res->ai_family == PF_INET6) {
	hdr_v2->fam = PP2_FAM_INET6 | PP2_TRANS_STREAM;
	hdr_v2->len = htons(PP2_ADDR_LEN_INET6);
	memcpy(hdr_v2->addr.ip6.src_addr,
	       &SOCK_ADDR_IN6_ADDR(src_res->ai_addr),
	       sizeof(hdr_v2->addr.ip6.src_addr));
	hdr_v2->addr.ip6.src_port = SOCK_ADDR_IN6_PORT(src_res->ai_addr);
	memcpy(hdr_v2->addr.ip6.dst_addr,
	       &SOCK_ADDR_IN6_ADDR(dst_res->ai_addr),
	       sizeof(hdr_v2->addr.ip6.dst_addr));
	hdr_v2->addr.ip6.dst_port = SOCK_ADDR_IN6_PORT(dst_res->ai_addr);
    } else
#endif
    if (src_res->ai_family == PF_INET) {
	hdr_v2->fam = PP2_FAM_INET | PP2_TRANS_STREAM;
	hdr_v2->len = htons(PP2_ADDR_LEN_INET);
	hdr_v2->addr.ip4.src_addr = SOCK_ADDR_IN_ADDR(src_res->ai_addr).s_addr;
	hdr_v2->addr.ip4.src_port = SOCK_ADDR_IN_PORT(src_res->ai_addr);
	hdr_v2->addr.ip4.dst_addr = SOCK_ADDR_IN_ADDR(dst_res->ai_addr).s_addr;
	hdr_v2->addr.ip4.dst_port = SOCK_ADDR_IN_PORT(dst_res->ai_addr);
    } else {
	msg_panic("unknown address family 0x%x", src_res->ai_family);
    }
    vstring_set_payload_size(buf, PP2_SIGNATURE_LEN + ntohs(hdr_v2->len));
    freeaddrinfo(src_res);
    freeaddrinfo(dst_res);
}

int     main(int argc, char **argv)
{
    VSTRING *test_label;
    TEST_CASE *v1_test_case;
    TEST_CASE v2_test_case;
    TEST_CASE mutated_test_case;
    VSTRING *v2_request_buf;
    VSTRING *mutated_request_buf;

    msg_vstream_init(sane_basename((VSTRING *) 0, argv[0]), VSTREAM_ERR);

    /* Findings. */
    int     tests_failed = 0;
    int     tests_passed = 0;

    test_label = vstring_alloc(100);
    v2_request_buf = vstring_alloc(100);
    mutated_request_buf = vstring_alloc(100);

    /*
     * Evaluate each case in the test case list. If the test input is
     * well-formed, also test a number of mutations based on that test,
     * before proceeding with the next test case in the list.
     */
    for (tests_failed = 0, tests_passed = 0, v1_test_case = v1_test_cases;
	 v1_test_case->haproxy_request != 0; v1_test_case++) {

	/*
	 * Fill in missing string length info in v1 test data.
	 */
	if (v1_test_case->haproxy_req_len == 0)
	    v1_test_case->haproxy_req_len =
		strlen(v1_test_case->haproxy_request);
	if (v1_test_case->exp_req_len == 0)
	    v1_test_case->exp_req_len = v1_test_case->haproxy_req_len;

	/*
	 * Evaluate each v1 test case.
	 */
	vstring_sprintf(test_label, "%d (%s%.5s input)",
			(int) (v1_test_case - v1_test_cases),
		     v1_test_case->exp_return ? "malformed" : "well-formed",
	 v1_test_case->exp_return ? "" : v1_test_case->haproxy_request + 5);
	msg_info("RUN  %s", STR(test_label));
	if (evaluate_test_case(STR(test_label), v1_test_case,
			       NO_SOCKADDR_OUTPUT)) {
	    msg_info("FAIL %s", STR(test_label));
	    tests_failed += 1;
	} else {
	    msg_info("PASS %s", STR(test_label));
	    tests_passed += 1;
	}

	/*
	 * If the v1 test input is malformed, skip the mutation tests.
	 */
	if (v1_test_case->exp_return != 0)
	    continue;

	/*
	 * Mutation test: a well-formed v1 test case should also pass with
	 * output to sockaddr arguments.
	 */
	vstring_sprintf(test_label, "%d (with sockaddr output)",
			(int) (v1_test_case - v1_test_cases));
	msg_info("RUN  %s", STR(test_label));
	if (evaluate_test_case(STR(test_label), v1_test_case,
			       DO_SOCKADDR_OUTPUT)) {
	    msg_info("FAIL %s", STR(test_label));
	    tests_failed += 1;
	} else {
	    msg_info("PASS %s", STR(test_label));
	    tests_passed += 1;
	}

	/*
	 * Mutation test: a well-formed v1 test case should still pass after
	 * appending a byte, and should return the actual parsed header
	 * length. The test uses the implicit VSTRING null safety byte.
	 */
	vstring_sprintf(test_label, "%d (one byte appended)",
			(int) (v1_test_case - v1_test_cases));
	mutated_test_case = *v1_test_case;
	mutated_test_case.haproxy_req_len += 1;
	msg_info("RUN  %s", STR(test_label));
	/* reuse v1_test_case->exp_req_len */
	if (evaluate_test_case(STR(test_label), &mutated_test_case,
			       NO_SOCKADDR_OUTPUT)) {
	    msg_info("FAIL %s", STR(test_label));
	    tests_failed += 1;
	} else {
	    msg_info("PASS %s", STR(test_label));
	    tests_passed += 1;
	}

	/*
	 * Mutation test: a well-formed v1 test case should fail after
	 * stripping the terminator.
	 */
	vstring_sprintf(test_label, "%d (last byte stripped)",
			(int) (v1_test_case - v1_test_cases));
	mutated_test_case = *v1_test_case;
	mutated_test_case.exp_return = "missing protocol header terminator";
	mutated_test_case.haproxy_req_len -= 1;
	mutated_test_case.exp_req_len = mutated_test_case.haproxy_req_len;
	msg_info("RUN  %s", STR(test_label));
	if (evaluate_test_case(STR(test_label), &mutated_test_case,
			       NO_SOCKADDR_OUTPUT)) {
	    msg_info("FAIL %s", STR(test_label));
	    tests_failed += 1;
	} else {
	    msg_info("PASS %s", STR(test_label));
	    tests_passed += 1;
	}

	/*
	 * A 'well-formed' v1 test case should pass after conversion to v2.
	 */
	vstring_sprintf(test_label, "%d (converted to v2)",
			(int) (v1_test_case - v1_test_cases));
	v2_test_case = *v1_test_case;
	convert_v1_proxy_req_to_v2(v2_request_buf,
				   v1_test_case->haproxy_request,
				   v1_test_case->haproxy_req_len);
	v2_test_case.haproxy_request = STR(v2_request_buf);
	v2_test_case.haproxy_req_len = PP2_HEADER_LEN
	    + ntohs(((struct proxy_hdr_v2 *) STR(v2_request_buf))->len);
	v2_test_case.exp_req_len = v2_test_case.haproxy_req_len;
	msg_info("RUN  %s", STR(test_label));
	if (evaluate_test_case(STR(test_label), &v2_test_case,
			       NO_SOCKADDR_OUTPUT)) {
	    msg_info("FAIL %s", STR(test_label));
	    tests_failed += 1;
	} else {
	    msg_info("PASS %s", STR(test_label));
	    tests_passed += 1;
	}

	/*
	 * Mutation test: a well-formed v2 test case should also pass with
	 * output to sockaddr arguments.
	 */
	vstring_sprintf(test_label,
			"%d (converted to v2, with sockaddr output)",
			(int) (v1_test_case - v1_test_cases));
	msg_info("RUN  %s", STR(test_label));
	if (evaluate_test_case(STR(test_label), &v2_test_case,
			       DO_SOCKADDR_OUTPUT)) {
	    msg_info("FAIL %s", STR(test_label));
	    tests_failed += 1;
	} else {
	    msg_info("PASS %s", STR(test_label));
	    tests_passed += 1;
	}

	/*
	 * Mutation test: a well-formed v2 test case should still pass after
	 * appending a byte, and should return the actual parsed header
	 * length. The test uses the implicit VSTRING null safety byte.
	 */
	vstring_sprintf(test_label, "%d (converted to v2, one byte appended)",
			(int) (v1_test_case - v1_test_cases));
	mutated_test_case = v2_test_case;
	mutated_test_case.haproxy_req_len += 1;
	/* reuse v2_test_case->exp_req_len */
	msg_info("RUN  %s", STR(test_label));
	if (evaluate_test_case(STR(test_label), &mutated_test_case,
			       NO_SOCKADDR_OUTPUT)) {
	    msg_info("FAIL %s", STR(test_label));
	    tests_failed += 1;
	} else {
	    msg_info("PASS %s", STR(test_label));
	    tests_passed += 1;
	}

	/*
	 * Mutation test: a well-formed v2 test case should fail after
	 * stripping one byte
	 */
	vstring_sprintf(test_label, "%d (converted to v2, last byte stripped)",
			(int) (v1_test_case - v1_test_cases));
	mutated_test_case = v2_test_case;
	mutated_test_case.haproxy_req_len -= 1;
	mutated_test_case.exp_req_len = mutated_test_case.haproxy_req_len;
	mutated_test_case.exp_return = "short version 2 protocol header";
	msg_info("RUN  %s", STR(test_label));
	if (evaluate_test_case(STR(test_label), &mutated_test_case,
			       NO_SOCKADDR_OUTPUT)) {
	    msg_info("FAIL %s", STR(test_label));
	    tests_failed += 1;
	} else {
	    msg_info("PASS %s", STR(test_label));
	    tests_passed += 1;
	}
    }

    /*
     * Additional V2-only tests.
     */
    vstring_strcpy(test_label, "v2 non-proxy request");
    msg_info("RUN  %s", STR(test_label));
    if (evaluate_test_case("v2 non-proxy request", &v2_non_proxy_test,
			   NO_SOCKADDR_OUTPUT)) {
	msg_info("FAIL %s", STR(test_label));
	tests_failed += 1;
    } else {
	msg_info("PASS %s", STR(test_label));
	tests_passed += 1;
    }

    /*
     * Clean up.
     */
    vstring_free(v2_request_buf);
    vstring_free(mutated_request_buf);
    vstring_free(test_label);
    msg_info("PASS=%d FAIL=%d", tests_passed, tests_failed);
    exit(tests_failed != 0);
}
