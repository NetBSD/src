/*	$NetBSD: normalize_v4mapped_addr_test.c,v 1.2 2026/05/09 18:49:23 christos Exp $	*/

 /*
  * System library.
  */
#include <sys_defs.h>
#include <stdlib.h>

 /*
  * Utility library.
  */
#include <normalize_v4mapped_addr.h>
#include <msg.h>
#include <msg_vstream.h>
#include <inet_proto.h>
#include <stringops.h>

 /*
  * Test cases are used twice, first to test normalize_v4mapped_hostaddr(),
  * and then  normalize_v4mapped_sockaddr().
  */
typedef struct TEST_CASE {
    const char *label;
    const char *inet_protocols;
    const char *in_hostaddr;
    int     want_return;
    const char *want_hostaddr;
} TEST_CASE;

#define	PASS	1
#define FAIL	2

static int test_normalize_v4mapped_hostaddr(const TEST_CASE *tp)
{
    int     got_return;
    MAI_HOSTADDR_STR hostaddr;

    /*
     * Prepare inputs.
     */
    if (strlen(tp->in_hostaddr) >= sizeof(hostaddr.buf)) {
	msg_warn("test_normalize_v4mapped_hostaddr: input too large");
	return (FAIL);
    }
    memcpy(hostaddr.buf, tp->in_hostaddr, strlen(tp->in_hostaddr) + 1);
    inet_proto_init("test_normalize_v4mapped_hostaddr", tp->inet_protocols);

    /*
     * Exercise the function under test: normalize_v4mapped_hostaddr().
     */
    got_return = normalize_v4mapped_hostaddr(&hostaddr);

    /*
     * Verify the results.
     */
    if (got_return != tp->want_return) {
	msg_warn("got return value %d, want %d", got_return, tp->want_return);
	return (FAIL);
    }
    if (strcmp(hostaddr.buf, tp->want_hostaddr) != 0) {
	msg_warn("got hostaddr '%s', want '%s'",
		 hostaddr.buf, tp->want_hostaddr);
	return (FAIL);
    }
    return (PASS);
}

static int test_normalize_v4mapped_sockaddr(const TEST_CASE *tp)
{
    int     got_return;
    MAI_HOSTADDR_STR hostaddr;
    int     err;
    struct addrinfo *res = 0;
    struct sockaddr_storage ss;
    SOCKADDR_SIZE ss_len;

    /*
     * Prepare inputs.
     */
    if (strlen(tp->in_hostaddr) >= sizeof(hostaddr.buf)) {
	msg_warn("test_normalize_v4mapped_hostaddr: input too large");
	return (FAIL);
    }
    memcpy(hostaddr.buf, tp->in_hostaddr, strlen(tp->in_hostaddr) + 1);
    inet_proto_init("test_normalize_v4mapped_sockaddr", tp->inet_protocols);

    /*
     * Convert the input hostaddr to sockaddr.
     */
    if ((err = hostaddr_to_sockaddr(tp->in_hostaddr, (char *) 0, 0, &res)) != 0) {
	msg_warn("hostaddr_to_sockaddr(\"%s\"...): %s",
		 tp->in_hostaddr, MAI_STRERROR(err));
	return (FAIL);
    }
    memcpy((void *) &ss, res->ai_addr, res->ai_addrlen);
    ss_len = res->ai_addrlen;
    freeaddrinfo(res);

    /*
     * Exercise the function under test: normalize_v4mapped_sockaddr().
     */
    got_return = normalize_v4mapped_sockaddr((struct sockaddr *) &ss, &ss_len);

    /*
     * Convert the output sockaddr to hostaddr.
     */
    if ((err = sockaddr_to_hostaddr((struct sockaddr *) &ss, ss_len,
			      &hostaddr, (MAI_SERVPORT_STR *) 0, 0)) != 0) {
	msg_warn("cannot convert address to string: %s", MAI_STRERROR(err));
	return (FAIL);
    }

    /*
     * Verify the results.
     */
    if (got_return != tp->want_return) {
	msg_warn("got return value %d, want %d", got_return, tp->want_return);
	return (FAIL);
    }
    if (strcmp(hostaddr.buf, tp->want_hostaddr) != 0) {
	msg_warn("got hostaddr '%s', want '%s'",
		 hostaddr.buf, tp->want_hostaddr);
	return (FAIL);
    }
    return (PASS);
}

static const TEST_CASE test_cases[] = {
    {.label = "does not convert v4 address, ipv4 enabled",
	.inet_protocols = "ipv6, ipv4",
	.in_hostaddr = "192.168.1.1",
	.want_return = 0,
	.want_hostaddr = "192.168.1.1",
    },
#if 0
    /* Can't test IPv4 forms with ipv4 disabled.  */
    {.label = "does not convert v4 address, ipv4 disabled",
	.inet_protocols = "ipv6",
	.in_hostaddr = "192.168.1.1",
	.want_return = 0,
	.want_hostaddr = "192.168.1.1",
    },
#endif
    {.label = "does not convert v4inv6 address, ipv4 disabled",
	.inet_protocols = "ipv6",
	.in_hostaddr = "::ffff:192.168.1.1",
	.want_return = 0,
	.want_hostaddr = "::ffff:192.168.1.1",
    },
    {.label = "converts v4inv6 address, ipv4 enabled",
	.inet_protocols = "ipv6, ipv4",
	.in_hostaddr = "::ffff:192.168.1.1",
	.want_return = 1,
	.want_hostaddr = "192.168.1.1",
    },
    0,
};

int     main(int argc, char **argv)
{
    const TEST_CASE *tp;
    int     pass = 0;
    int     fail = 0;
    struct test_action {
	int     (*act_fn) (const TEST_CASE *);
	const char *act_name;
    };
    static struct test_action actions[] = {
	test_normalize_v4mapped_hostaddr, "test_normalize_v4mapped_hostaddr",
	test_normalize_v4mapped_sockaddr, "test_normalize_v4mapped_sockaddr",
	0
    };
    struct test_action *ap;

    msg_vstream_init(sane_basename((VSTRING *) 0, argv[0]), VSTREAM_ERR);

    for (ap = actions; ap->act_fn; ap++) {
	for (tp = test_cases; tp->label != 0; tp++) {
	    msg_info("RUN  %s/%s", ap->act_name, tp->label);
	    if (ap->act_fn(tp) != PASS) {
		fail++;
		msg_info("FAIL %s/%s", ap->act_name, tp->label);
	    } else {
		msg_info("PASS %s/%s", ap->act_name, tp->label);
		pass++;
	    }
	}
    }
    msg_info("PASS=%d FAIL=%d", pass, fail);
    exit(fail != 0);
}
