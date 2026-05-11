/*	$NetBSD: smtpd_peer_test.c,v 1.2.2.2 2026/05/11 17:13:58 martin Exp $	*/

/*++
/* NAME
/*      smtpd_peer_test 1t
/* SUMMARY
/*      smtpd_peer_init() unit tests
/* SYNOPSIS
/*      ./smtpd_peer_test
/* DESCRIPTION
/*      Verifies that smtpd_peer_init() will update the SMTPD_STATE
/*      structure with the expected error or endpoint information for
/*      different input sources.
/* LICENSE
/* .ad
/* .fi
/*      The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*      Wietse Venema
/*	porcupine.org
/*--*/

 /*
  * System library.
  */
#include <sys_defs.h>

 /*
  * Utility library.
  */
#include <htable.h>
#include <msg.h>
#include <msg_vstream.h>
#include <stringops.h>
#include <vstream.h>

 /*
  * Global library.
  */
#include <haproxy_srvr.h>
#include <inet_proto.h>
#include <mail_params.h>
#include <mail_proto.h>

 /*
  * TODO(wietse) make this a proper VSTREAM interface or test helper API.
  */

/* vstream_swap - capture output for testing */

static void vstream_swap(VSTREAM *one, VSTREAM *two)
{
    VSTREAM save;

    save = *one;
    *one = *two;
    *two = save;
}

 /*
  * Application-specific.
  */
#include <smtpd.h>

int     tests_failed = 0;
int     tests_passed = 0;

 /*
  * Fakes to satisfy some dependencies.
  */
#include <smtpd_chat.h>
#include <smtpd_sasl_glue.h>

#define TEST_TIMEOUT	10
char   *var_smtpd_uproxy_proto = "";
int     var_smtpd_uproxy_tmout = TEST_TIMEOUT;
bool    var_smtpd_peername_lookup;
bool    var_smtpd_client_port_log;
bool    var_smtpd_sasl_enable;
int     var_smtpd_cipv4_prefix = DEF_SMTPD_CIPV4_PREFIX;
int     var_smtpd_cipv6_prefix = DEF_SMTPD_CIPV6_PREFIX;
char   *var_notify_classes = DEF_NOTIFY_CLASSES;
void    smtpd_chat_reset(SMTPD_STATE *state)
{
}
void    smtpd_sasl_state_init(SMTPD_STATE *state)
{
}
void    smtpd_xforward_init(SMTPD_STATE *state)
{
}

/* Reset globals that may be tweaked by individual tests */

static void reset_global_variables(void)
{
    var_smtpd_uproxy_proto = "";
    inet_proto_init("reset_global_variables", "all");
}

 /*
  * Basic tests that smtpd_peer_init() will update the SMTPD_STATE structure
  * with the expected error info or endpoint info. This needs to be
  * subclassed to support different input sources (local client, no open
  * connection, HaProxy, postscreen, etc.).
  */
typedef struct TEST_BASE {
    const char *label;
    int     want_hangup;
    const char *want_warning;
    const char *want_client_name;
    int     want_client_name_status;
    const char *want_client_reverse_name;
    int     want_client_reverse_name_status;
    const char *want_client_addr;
    const char *want_client_rfc_addr;
    const char *want_client_port;
    int     want_client_addr_family;
    const char *want_server_addr;
    const char *want_server_port;
    int     want_sockaddr_len;
    int     want_dest_sockaddr_len;
} TEST_BASE;

/* test_smtpd_peer_init - enforce smtpd_peer_init() expectations */

static int test_smtpd_peer_init(const TEST_BASE *tp, VSTREAM *fp,
				        SMTPD_STATE *state)
{
    VSTRING *msg_buf = vstring_alloc(100);
    VSTREAM *memory_stream;
    int     test_passed = 1;
    int     aierr;
    MAI_HOSTADDR_STR got_addr;
    MAI_SERVPORT_STR got_port;

    /*
     * Detonate smtpd_state_init() in a little sandbox.
     */
    VSTRING_RESET(msg_buf);
    VSTRING_TERMINATE(msg_buf);
    if ((memory_stream = vstream_memopen(msg_buf, O_WRONLY)) == 0)
	msg_fatal("open memory stream: %m");
    vstream_swap(VSTREAM_ERR, memory_stream);
    smtpd_state_init(state, fp, "something");
    vstream_swap(memory_stream, VSTREAM_ERR);
    (void) vstream_fclose(memory_stream);

    /* Verify the results. */
    if (tp->want_hangup != (state->flags & SMTPD_FLAG_HANGUP)) {
	msg_warn("got hangup flag '0x%x', want '0x%x'",
		 state->flags & SMTPD_FLAG_HANGUP, tp->want_hangup);
	test_passed = 0;
    } else if (tp->want_warning == 0 && VSTRING_LEN(msg_buf) > 0) {
	msg_warn("got warning ``%s'', want ``null''", vstring_str(msg_buf));
	test_passed = 0;
    } else if (tp->want_warning != 0) {
	if (strstr(vstring_str(msg_buf), tp->want_warning) == 0) {
	    msg_warn("got warning ``%s'', want ``%s''",
		     vstring_str(msg_buf), tp->want_warning);
	    test_passed = 0;
	}
    }
    if (test_passed == 0) {
	 /* void */ ;
    } else if (tp->want_client_name
	       && strcmp(state->name, tp->want_client_name) != 0) {
	msg_warn("got client name '%s', want '%s'",
		 state->name, tp->want_client_name);
	test_passed = 0;
    } else if (tp->want_client_name_status != 0
	       && state->name_status != tp->want_client_name_status) {
	msg_warn("got client name status '%d', want '%d'",
		 state->name_status, tp->want_client_name_status);
	test_passed = 0;
    } else if (tp->want_client_reverse_name
	&& strcmp(state->reverse_name, tp->want_client_reverse_name) != 0) {
	msg_warn("got client reverse name '%s', want '%s'",
		 state->reverse_name, tp->want_client_reverse_name);
	test_passed = 0;
    } else if (tp->want_client_reverse_name_status != 0
     && state->reverse_name_status != tp->want_client_reverse_name_status) {
	msg_warn("got client reverse name status '%d', want '%d'",
	   state->reverse_name_status, tp->want_client_reverse_name_status);
	test_passed = 0;
    } else if (tp->want_client_addr
	       && strcmp(state->addr, tp->want_client_addr) != 0) {
	msg_warn("got text client address '%s', want '%s'",
		 state->addr, tp->want_client_addr);
	test_passed = 0;
    } if (tp->want_client_rfc_addr
	  && strcmp(tp->want_client_rfc_addr, state->rfc_addr) != 0) {
	msg_warn("got client rfc_addr '%s', want '%s'",
		 state->rfc_addr, tp->want_client_rfc_addr);
	test_passed = 0;
    } else if (tp->want_client_port
	       && strcmp(state->port, tp->want_client_port) != 0) {
	msg_warn("got text client port '%s', want '%s'",
		 state->port, tp->want_client_port);
	test_passed = 0;
    } else if (!state->sockaddr_len != !tp->want_sockaddr_len) {
	msg_warn("got sockaddr_len '%d', want '%d'",
		 (int) state->sockaddr_len, (int) tp->want_sockaddr_len);
	test_passed = 0;
    } else if (tp->want_server_addr
	       && strcmp(state->dest_addr, tp->want_server_addr) != 0) {
	msg_warn("got text server address '%s', want '%s'",
		 state->dest_addr, tp->want_server_addr);
	test_passed = 0;
    } else if (tp->want_server_port
	       && strcmp(state->dest_port, tp->want_server_port) != 0) {
	msg_warn("got text server port '%s', want '%s'",
		 state->dest_port, tp->want_server_port);
	test_passed = 0;
    } else if (!state->dest_sockaddr_len != !tp->want_dest_sockaddr_len) {
	msg_warn("got dest_sockaddr_len '%d', want '%d'",
	  (int) state->dest_sockaddr_len, (int) tp->want_dest_sockaddr_len);
	test_passed = 0;
    } else {
	if (state->sockaddr_len > 0) {
	    if ((aierr =
		 sockaddr_to_hostaddr((struct sockaddr *) &state->sockaddr,
		      state->sockaddr_len, &got_addr, &got_port, 0)) != 0) {
		msg_warn("sockaddr_to_hostaddr: %s", MAI_STRERROR(aierr));
		test_passed = 0;
	    } else if (tp->want_client_addr
		       && strcmp(got_addr.buf, tp->want_client_addr) != 0) {
		msg_warn("got binary client address '%s', want '%s'",
			 got_addr.buf, tp->want_client_addr);
		test_passed = 0;
	    } else if (tp->want_client_port
		       && strcmp(got_port.buf, tp->want_client_port) != 0) {
		msg_warn("got binary client port '%s', want '%s'",
			 got_port.buf, tp->want_client_port);
		test_passed = 0;
	    }
	}
	if (state->dest_sockaddr_len > 0) {
	    if ((aierr =
	     sockaddr_to_hostaddr((struct sockaddr *) &state->dest_sockaddr,
		 state->dest_sockaddr_len, &got_addr, &got_port, 0)) != 0) {
		msg_warn("sockaddr_to_hostaddr: %s", MAI_STRERROR(aierr));
		test_passed = 0;
	    } else if (tp->want_server_addr
		       && strcmp(got_addr.buf, tp->want_server_addr) != 0) {
		msg_warn("got binary server address '%s', want '%s'",
			 got_addr.buf, tp->want_server_addr);
		test_passed = 0;
	    } else if (tp->want_server_port
		       && strcmp(got_port.buf, tp->want_server_port) != 0) {
		msg_warn("got binary server port '%s', want '%s'",
			 got_port.buf, tp->want_server_port);
		test_passed = 0;
	    }
	}
    }
    (void) vstring_free(msg_buf);
    smtpd_state_reset(state);
    return (test_passed);
}

 /*
  * Tests that a non-socket client results in fake localhost info.
  */
typedef struct PEER_FROM_NON_SOCKET_CASE {
    TEST_BASE base;
    const char *inet_protocols;
} PEER_FROM_NON_SOCKET_CASE;

static const PEER_FROM_NON_SOCKET_CASE peer_from_non_socket_cases[] = {
    {
	.base = {
	    .label = "prefer_ipv4",
	    .want_client_name = "localhost",
	    .want_client_name_status = SMTPD_PEER_CODE_OK,
	    .want_client_addr = "127.0.0.1",
	    .want_client_addr_family = AF_UNSPEC,
	    .want_client_rfc_addr = "127.0.0.1",
	    .want_client_reverse_name_status = SMTPD_PEER_CODE_OK,
	    .want_client_port = "0",
	    .want_server_addr = "127.0.0.1",
	    .want_server_port = "0",
	},
	.inet_protocols = "ipv4",
    },
    {
	.base = {
	    .label = "prefer_ipv6",
	    .want_client_name = "localhost",
	    .want_client_name_status = SMTPD_PEER_CODE_OK,
	    .want_client_reverse_name = "localhost",
	    .want_client_reverse_name_status = SMTPD_PEER_CODE_OK,
	    .want_client_addr = "::1",
	    .want_client_addr_family = AF_UNSPEC,
	    .want_client_rfc_addr = "IPv6:::1",
	    .want_client_port = "0",
	    .want_server_addr = "::1",
	    .want_server_port = "0",
	},
	.inet_protocols = "ipv6",
    },
    {0},
};

static void test_peer_from_non_socket(void)
{
    int     test_passed;
    const PEER_FROM_NON_SOCKET_CASE *tp;

    reset_global_variables();

    for (tp = peer_from_non_socket_cases; tp->base.label != 0; tp++) {
	msg_info("RUN  test_peer_from_non_socket/%s", tp->base.label);
	{
	    SMTPD_STATE state;
	    VSTREAM *fp;
	    int     pair[2];

	    if (pipe(pair) < 0)
		msg_fatal("pipe: %m");
	    if ((fp = vstream_fdopen(pair[0], O_RDONLY)) == 0)
		msg_fatal("vstream_fdopen: %m");
	    (void) inet_proto_init("test_peer_from_non_socket",
				   tp->inet_protocols);

	    test_passed = test_smtpd_peer_init((TEST_BASE *) tp, fp, &state);

	    (void) vstream_fdclose(fp);
	    (void) close(pair[0]);
	    (void) close(pair[1]);
	}
	if (test_passed) {
	    msg_info("PASS test_peer_from_non_socket/%s", tp->base.label);
	    tests_passed += 1;
	} else {
	    msg_info("FAIL test_peer_from_non_socket/%s", tp->base.label);
	    tests_failed += 1;
	}
    }
}

 /*
  * Tests that a non-connected socket results in 'unknown' endpoint info.
  */
typedef struct PEER_FROM_UNCONN_SOCKET_CASE {
    TEST_BASE base;
    int     proto_family;
} PEER_FROM_UNCONN_SOCKET_CASE;

static const PEER_FROM_UNCONN_SOCKET_CASE peer_from_unconn_socket_cases[] = {
    {
	.base = {
	    .label = "tcp4",
	    .want_client_name = CLIENT_NAME_UNKNOWN,
	    .want_client_name_status = SMTPD_PEER_CODE_PERM,
	    .want_client_addr = CLIENT_ADDR_UNKNOWN,
	    .want_client_addr_family = AF_UNSPEC,
	    .want_client_rfc_addr = CLIENT_ADDR_UNKNOWN,
	    .want_client_reverse_name = CLIENT_NAME_UNKNOWN,
	    .want_client_reverse_name_status = SMTPD_PEER_CODE_PERM,
	    .want_client_port = CLIENT_PORT_UNKNOWN,
	    .want_server_addr = SERVER_ADDR_UNKNOWN,
	    .want_server_port = SERVER_PORT_UNKNOWN,
	},
	.proto_family = PF_INET,
    },
    {
	.base = {
	    .label = "tcp6",
	    .want_client_name = CLIENT_NAME_UNKNOWN,
	    .want_client_name_status = SMTPD_PEER_CODE_PERM,
	    .want_client_addr = CLIENT_ADDR_UNKNOWN,
	    .want_client_addr_family = AF_UNSPEC,
	    .want_client_rfc_addr = CLIENT_ADDR_UNKNOWN,
	    .want_client_reverse_name = CLIENT_NAME_UNKNOWN,
	    .want_client_reverse_name_status = SMTPD_PEER_CODE_PERM,
	    .want_client_port = CLIENT_PORT_UNKNOWN,
	    .want_server_addr = SERVER_ADDR_UNKNOWN,
	    .want_server_port = SERVER_PORT_UNKNOWN,
	},
	.proto_family = PF_INET6,
    },
    {
	.base = {
	    .label = "unix",
	    .want_client_name = CLIENT_NAME_UNKNOWN,
	    .want_client_name_status = SMTPD_PEER_CODE_PERM,
	    .want_client_addr = CLIENT_ADDR_UNKNOWN,
	    .want_client_addr_family = AF_UNSPEC,
	    .want_client_rfc_addr = CLIENT_ADDR_UNKNOWN,
	    .want_client_reverse_name = CLIENT_NAME_UNKNOWN,
	    .want_client_reverse_name_status = SMTPD_PEER_CODE_PERM,
	    .want_client_port = CLIENT_PORT_UNKNOWN,
	    .want_server_addr = SERVER_ADDR_UNKNOWN,
	    .want_server_port = SERVER_PORT_UNKNOWN,
	},
	.proto_family = PF_UNIX,
    },
    {0},
};

static void test_peer_from_unconn_socket(void)
{
    int     test_passed;
    const PEER_FROM_UNCONN_SOCKET_CASE *tp;

    reset_global_variables();

    for (tp = peer_from_unconn_socket_cases; tp->base.label != 0; tp++) {
	msg_info("RUN  test_peer_from_unconn_socket/%s", tp->base.label);
	{
	    SMTPD_STATE state;
	    VSTREAM *fp;
	    int     sock;

	    if ((sock = socket(tp->proto_family, SOCK_STREAM, 0)) < 0)
		msg_fatal("socketpair: %m");
	    if ((fp = vstream_fdopen(sock, O_RDONLY)) == 0)
		msg_fatal("vstream_fdopen: %m");

	    test_passed = test_smtpd_peer_init((TEST_BASE *) tp, fp, &state);

	    (void) vstream_fclose(fp);
	}
	if (test_passed) {
	    msg_info("PASS test_peer_from_unconn_socket/%s", tp->base.label);
	    tests_passed += 1;
	} else {
	    msg_info("FAIL test_peer_from_unconn_socket/%s", tp->base.label);
	    tests_failed += 1;
	}
    }
}

 /*
  * Tests that smtpd_peer_from_pass_attr() updates the SMTPD_STATE structure
  * with the expected error or endpoint information.
  */
#define PASS_ATTR_COUNT	5
struct PASS_ATTR {
    const char *key;
    const char *value;
};
typedef struct PEER_FROM_PASS_ATTR_CASE {
    const TEST_BASE base;		/* parent class */
    struct PASS_ATTR attrs[PASS_ATTR_COUNT];
} PEER_FROM_PASS_ATTR_CASE;

 /*
  * We need one test for every constraint in smtpd_peer_from_pass(), to
  * demonstrate that smtpd_pass_attr.c propagates errors and endpoint info.
  */
static const PEER_FROM_PASS_ATTR_CASE peer_from_pass_attr_cases[] = {
    {
	.base = {
	    .label = "propagates_endpoint_info_from_good_pass_attr",
	    .want_client_addr = "1.2.3.4",
	    .want_client_port = "123",
	    .want_client_addr_family = AF_INET,
	    .want_server_addr = "4.3.2.1",
	    .want_server_port = "321",
	    .want_sockaddr_len = 1,
	    .want_dest_sockaddr_len = 1,
	},
	.attrs = {
	    MAIL_ATTR_ACT_CLIENT_ADDR, "1.2.3.4",
	    MAIL_ATTR_ACT_CLIENT_PORT, "123",
	    MAIL_ATTR_ACT_SERVER_ADDR, "4.3.2.1",
	    MAIL_ATTR_ACT_SERVER_PORT, "321",
	},
    },
    {
	.base = {
	    .label = "propagates_error_from_bad_IPv4_client_addr",
	    .want_hangup = 1,
	    .want_warning = "bad IPv4 client address",
	    /* TODO(wietse) Should we verify the surrogate endpoint info? */
	},
	.attrs = {
	    MAIL_ATTR_ACT_CLIENT_ADDR, "1.1.2.3.4",
	    MAIL_ATTR_ACT_CLIENT_PORT, "123",
	    MAIL_ATTR_ACT_SERVER_ADDR, "4.3.2.1",
	    MAIL_ATTR_ACT_SERVER_PORT, "321",
	},
    },
    {
	.base = {
	    .label = "propagates_error_from_missing_client_addr",
	    .want_hangup = 1,
	    .want_warning = "missing client address",
	},
	.attrs = {
	    MAIL_ATTR_ACT_CLIENT_PORT, "123",
	    MAIL_ATTR_ACT_SERVER_ADDR, "4.3.2.1",
	    MAIL_ATTR_ACT_SERVER_PORT, "321",
	},
    },
    {
	.base = {
	    .label = "propagates_error_from_bad_TCP_client_port",
	    .want_hangup = 1,
	    .want_warning = "bad TCP client port",
	},
	.attrs = {
	    MAIL_ATTR_ACT_CLIENT_ADDR, "1.2.3.4",
	    MAIL_ATTR_ACT_CLIENT_PORT, "A23",
	    MAIL_ATTR_ACT_SERVER_ADDR, "4.3.2.1",
	    MAIL_ATTR_ACT_SERVER_PORT, "321",
	},
    },
    {
	.base = {
	    .label = "propagates_error_from_missing_client_port",
	    .want_hangup = 1,
	    .want_warning = "missing client port",
	},
	.attrs = {
	    MAIL_ATTR_ACT_CLIENT_ADDR, "1.2.3.4",
	    MAIL_ATTR_ACT_SERVER_ADDR, "4.3.2.1",
	    MAIL_ATTR_ACT_SERVER_PORT, "321",
	},
    },
    {
	.base = {
	    .label = "propagates_error_from_bad_IPv6_server_addr",
	    .want_hangup = 1,
	    .want_warning = "bad IPv6 server address",
	},
	.attrs = {
	    MAIL_ATTR_ACT_CLIENT_ADDR, "1.2.3.4",
	    MAIL_ATTR_ACT_CLIENT_PORT, "123",
	    MAIL_ATTR_ACT_SERVER_ADDR, ":::4.3.2.1",
	    MAIL_ATTR_ACT_SERVER_PORT, "321",
	},
    },
    {
	.base = {
	    .label = "propagates_error_from_missing_server_addr",
	    .want_hangup = 1,
	    .want_warning = "missing server address",
	},
	.attrs = {
	    MAIL_ATTR_ACT_CLIENT_ADDR, "1.2.3.4",
	    MAIL_ATTR_ACT_CLIENT_PORT, "123",
	    MAIL_ATTR_ACT_SERVER_PORT, "321",
	},
    },
    {
	.base = {
	    .label = "propagates_error_from_bad_TCP_server_port",
	    .want_hangup = 1,
	    .want_warning = "bad TCP server port",
	},
	.attrs = {
	    MAIL_ATTR_ACT_CLIENT_ADDR, "1.2.3.4",
	    MAIL_ATTR_ACT_CLIENT_PORT, "123",
	    MAIL_ATTR_ACT_SERVER_ADDR, "4.3.2.1",
	    MAIL_ATTR_ACT_SERVER_PORT, "A21",
	},
    },
    {
	.base = {
	    .label = "propagates_error_from_missing_server_port",
	    .want_hangup = 1,
	    .want_warning = "missing server port",
	},
	.attrs = {
	    MAIL_ATTR_ACT_CLIENT_ADDR, "1.2.3.4",
	    MAIL_ATTR_ACT_CLIENT_PORT, "123",
	    MAIL_ATTR_ACT_SERVER_ADDR, "4.3.2.1",
	},
    },
    0,
};

static void test_peer_from_pass_attr(void)
{
    int     test_passed;
    const PEER_FROM_PASS_ATTR_CASE *tp;

    reset_global_variables();

    for (tp = peer_from_pass_attr_cases; tp->base.label != 0; tp++) {
	msg_info("RUN  test_peer_from_pass_attr/%s", tp->base.label);
	{
	    SMTPD_STATE state;
	    VSTREAM *fp;
	    int     sock;
	    HTABLE *attr_table = htable_create(PASS_ATTR_COUNT);
	    const struct PASS_ATTR *p;

	    if ((sock = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
		msg_fatal("socket: %m");
	    if ((fp = vstream_fdopen(sock, O_RDWR)) == 0)
		msg_fatal("vstream_fdopen: %m");
	    for (p = tp->attrs; p < tp->attrs + PASS_ATTR_COUNT && p->key != 0; p++)
		htable_enter(attr_table, p->key, (void *) p->value);
	    vstream_control(fp,
			    VSTREAM_CTL_CONTEXT, (void *) attr_table,
			    VSTREAM_CTL_END);

	    test_passed = test_smtpd_peer_init((TEST_BASE *) tp, fp, &state);

	    htable_free(attr_table, (void (*) (void *)) 0);
	    (void) vstream_fclose(fp);
	}
	if (test_passed) {
	    msg_info("PASS test_peer_from_pass_attr/%s", tp->base.label);
	    tests_passed += 1;
	} else {
	    msg_info("FAIL test_peer_from_pass_attr/%s", tp->base.label);
	    tests_failed += 1;
	}
    }
}

 /*
  * Tests that smtpd_peer_from_haproxy() updates the SMTPD_STATE structure
  * with the expected error or endpoint information.
  */
typedef struct PEER_FROM_HAPROXY_CASE {
    const TEST_BASE base;
    const char *proxy_header;
} PEER_FROM_HAPROXY_CASE;

 /*
  * We need only two tests to show that smtpd_haproxy.c propagates errors and
  * non-error endpoint info. We don't need to duplicate each individual test
  * in haproxy_srvr_test.c for different IP protocols, HaProxy protocol
  * versions, and error modes.
  */
static const PEER_FROM_HAPROXY_CASE peer_from_haproxy_caes[] = {
    {
	.base = {
	    .label = "propagates_endpoint_info_from_good_proxy_header",
	    .want_client_addr = "1.2.3.4",
	    .want_client_port = "123",
	    .want_client_addr_family = AF_INET,
	    .want_server_addr = "4.3.2.1",
	    .want_server_port = "321",
	    .want_sockaddr_len = 1,
	    .want_dest_sockaddr_len = 1,
	},
	.proxy_header = "PROXY TCP4 1.2.3.4 4.3.2.1 123 321\n",
    },
    {
	.base = {
	    .label = "propagates_error_from_bad_proxy_header",
	    .want_hangup = 1,
	    .want_warning = "short protocol header",
	    /* TODO(wietse) Should we verify the surrogate endpoint info? */
	},
	.proxy_header = "bad",
    },
    0,
};

static void test_peer_from_haproxy(void)
{
    int     test_passed;
    const PEER_FROM_HAPROXY_CASE *tp;

    reset_global_variables();
    var_smtpd_uproxy_proto = HAPROXY_PROTO_NAME;

    for (tp = peer_from_haproxy_caes; tp->proxy_header != 0; tp++) {
	msg_info("RUN  test_peer_from_haproxy/%s", tp->base.label);
	{
	    SMTPD_STATE state;
	    VSTREAM *fp;
	    int     pair[2];

	    if (socketpair(AF_UNIX, SOCK_STREAM, 0, pair) < 0)
		msg_fatal("socketpair: %m");
	    if (write_buf(pair[1], tp->proxy_header, strlen(tp->proxy_header),
			  TEST_TIMEOUT) < 0)
		msg_fatal("write_buf: %m");
	    if ((fp = vstream_fdopen(pair[0], O_RDONLY)) == 0)
		msg_fatal("vstream_fdopen: %m");

	    test_passed = test_smtpd_peer_init((TEST_BASE *) tp, fp, &state);

	    (void) vstream_fdclose(fp);
	    (void) close(pair[0]);
	    (void) close(pair[1]);
	}
	if (test_passed) {
	    msg_info("PASS test_peer_from_haproxy/%s", tp->base.label);
	    tests_passed += 1;
	} else {
	    msg_info("FAIL test_peer_from_haproxy/%s", tp->base.label);
	    tests_failed += 1;
	}
    }
}

int     main(int argc, char **argv)
{
    msg_vstream_init(sane_basename((VSTRING *) 0, argv[0]), VSTREAM_ERR);

    test_peer_from_non_socket();

    test_peer_from_unconn_socket();

    test_peer_from_pass_attr();

    test_peer_from_haproxy();

    /*
     * TODO(wietse) tests for a connected socket. This will require mock
     * get_peername/get/sockname() and getnameinfo/getaddrinfo()
     * infrastructure.
     */

    msg_info("PASS=%d FAIL=%d", tests_passed, tests_failed);
    exit(tests_failed != 0);
}
