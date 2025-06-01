/*
* NPF socket User/group id tests.
*
* Public Domain.
*/

#ifdef _KERNEL
#include <sys/types.h>
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: npf_rid_test.c,v 1.1 2025/06/01 01:07:26 joe Exp $");
#endif

#include "npf_impl.h"
#include "npf_test.h"

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/kauth.h>
#include <sys/socketvar.h>
#include <sys/lwp.h>
#include <sys/cpu.h>

#define	RESULT_PASS	0
#define	RESULT_BLOCK	ENETUNREACH

/* this port number suitable for testing */
#define REMOTE_PORT	65500
#define LOCAL_PORT	65000
#define LOCAL_IP	"127.0.0.1"
#define REMOTE_IP	LOCAL_IP

static const struct test_case {
	int		af;
	const char *	src;
	uint16_t	sport;
	const char *	dst;
	uint16_t	dport;
	uint32_t	uid;
	uint32_t	gid;
	const char *	ifname;
	int		di;
	int 	ret;
	int 	stateful_ret;
} test_cases[] = {
	{
		/* pass in final from $local_ip4 user $Kojo = 1001 group $wheel = 20 */
		.af = AF_INET,
		.src = "10.1.1.4",			.sport = 9000,
		.dst = LOCAL_IP,			.dport = LOCAL_PORT,
		.ifname = IFNAME_EXT,			.di = PFIL_IN,
		.uid = 1001,				.gid = 20, /* matches so pass it */
		.ret = RESULT_PASS,			.stateful_ret = RESULT_PASS
	},
	{
		/* connect on different UID and block */
		.af = AF_INET,
		.src = "10.1.1.4",			.sport = 9000,
		.dst = LOCAL_IP,			.dport = LOCAL_PORT,
		.ifname = IFNAME_EXT,			.di = PFIL_IN,
		.uid = 1001,				.gid = 10, /* mismatch gid so block it */
		.ret = RESULT_BLOCK,			.stateful_ret = RESULT_BLOCK
	},
	{
		.af = AF_INET,
		.src = "10.1.1.4",			.sport = 9000,
		.dst = LOCAL_IP,			.dport = LOCAL_PORT,
		.ifname = IFNAME_EXT,			.di = PFIL_IN,
		.uid = 100,				.gid = 20, /* mismatch uid so block it */
		.ret = RESULT_BLOCK,			.stateful_ret = RESULT_BLOCK
	},


	/* block out final to 127.0.0.1 user > $Kojo( > 1001) group 1 >< $wheel( IRG 1 >< 20) */
	{
		.af = AF_INET,
		.src = LOCAL_IP,			.sport = LOCAL_PORT,
		.dst = REMOTE_IP,			.dport = REMOTE_PORT,
		.ifname = IFNAME_EXT,			.di = PFIL_OUT,
		.uid = 1005,				.gid = 14, /* matches so blocks it */
		.ret = RESULT_BLOCK,			.stateful_ret = RESULT_BLOCK
	},
	{
		.af = AF_INET,
		.src = LOCAL_IP,			.sport = LOCAL_PORT,
		.dst = REMOTE_IP,			.dport = REMOTE_PORT,
		.ifname = IFNAME_EXT,			.di = PFIL_OUT,
		.uid = 1005,				.gid = 30, /* mismatch gid so pass it */
		.ret = RESULT_PASS,			.stateful_ret = RESULT_PASS
	},
	{
		.af = AF_INET,
		.src = LOCAL_IP,			.sport = LOCAL_PORT,
		.dst = REMOTE_IP,			.dport = REMOTE_PORT,
		.ifname = IFNAME_EXT,			.di = PFIL_OUT,
		.uid = 100,				.gid = 15, /* mismatch uid so pass it */
		.ret = RESULT_PASS,			.stateful_ret = RESULT_PASS
	},
	{
		.af = AF_INET,
		.src = LOCAL_IP,			.sport = LOCAL_PORT,
		.dst = REMOTE_IP,			.dport = REMOTE_PORT,
		.ifname = IFNAME_EXT,			.di = PFIL_OUT,
		.uid = 1010,				.gid = 11, /* matches so blocks it */
		.ret = RESULT_BLOCK,			.stateful_ret = RESULT_BLOCK
	},
};

static int
run_raw_testcase(unsigned i, bool verbose)
{
	const struct test_case *t = &test_cases[i];
	npf_t *npf = npf_getkernctx();
	npf_cache_t *npc;
	struct mbuf *m;
	npf_rule_t *rl;
	int slock, error;

	m = mbuf_get_pkt(t->af, IPPROTO_UDP, t->src, t->dst, t->sport, t->dport);
	npc = get_cached_pkt(m, t->ifname);

	slock = npf_config_read_enter(npf);
	rl = npf_ruleset_inspect(npc, npf_config_ruleset(npf), t->di, NPF_LAYER_3);
	if (rl) {
		npf_match_info_t mi;
		int id_match;

		id_match = npf_rule_match_rid(rl, npc, t->di);
		error = npf_rule_conclude(rl, &mi);
		if (verbose)
			printf("id match is ...%d\n", id_match);
		if (id_match != -1 && !id_match) {
			error = npf_rule_reverse(npc, &mi, error);
		}

	} else {
		error = ENOENT;
	}
	npf_config_read_exit(npf, slock);

	put_cached_pkt(npc);
	return error;
}

static int
run_handler_testcase(unsigned i)
{
	const struct test_case *t = &test_cases[i];
	ifnet_t *ifp = npf_test_getif(t->ifname);
	npf_t *npf = npf_getkernctx();
	struct mbuf *m;
	int error;

	m = mbuf_get_pkt(t->af, IPPROTO_UDP, t->src, t->dst, t->sport, t->dport);
	error = npfk_packet_handler(npf, &m, ifp, t->di);
	if (m) {
		m_freem(m);
	}
	return error;
}

/*
 * we create our specific server socket here which listens on
 * loopback address and port 65000. easier to test pcb lookup here since
 * it will be loaded into the protocol table.
 */
static struct socket *
test_socket(int dir, uid_t uid, gid_t gid)
{
	struct sockaddr_in server;
	struct lwp *cur = curlwp;
	void *p, *rp;

	memset(&Server, 0, sizeof(server));

	server.sin_len = sizeof(server);
	server.sin_family = AF_INET;
	p = &server.sin_addr.s_addr;
	npf_inet_pton(AF_INET, LOCAL_IP, p); /* we bind to 127.0.0.1 */
	server.sin_port = htons(LOCAL_PORT);

	struct socket *so;
	int error = socreate(AF_INET, &so, SOCK_DGRAM, 0, cur, NULL);
	if (error) {
		printf("socket creation failed: error is %d\n", error);
		return NULL;
	}

	solock(so);

	kauth_cred_t cred = kauth_cred_alloc();
	kauth_cred_seteuid(cred, uid);
	kauth_cred_setegid(cred, gid);

	kauth_cred_t old = so->so_cred;
	so->so_cred = kauth_cred_dup(cred);
	kauth_cred_free(old);

	sounlock(so);

	if ((error = sobind(so, (struct sockaddr *)&server, cur)) != 0) {
		printf("bind failed %d\n", error);
		return NULL;
	}

	if (dir == PFIL_OUT) {
		/* connect to an additional remote address to set the 4 tuple addr-port state */
		struct sockaddr_in remote;
		memset(&Remote, 0, sizeof(remote));

		remote.sin_len = sizeof(remote);
		remote.sin_family = AF_INET;
		rp = &remote.sin_addr.s_addr;
		npf_inet_pton(AF_INET, REMOTE_IP, rp); /* we connect to 127.0.0.1 */
		remote.sin_port = htons(REMOTE_PORT);

		solock(so);
		if ((error = soconnect(so, (struct sockaddr *)&remote, cur)) != 0) {
			printf("connect failed :%d\n", error);
			return NULL;
		}
		sounlock(so);
	}

	return so;
}

static bool
test_static(bool verbose)
{
	for (size_t i = 0; i < __arraycount(test_cases); i++) {
		const struct test_case *t = &test_cases[i];
		int error, serror;
		struct socket *so;

		so = test_socket(t->di, t->uid, t->gid);
		if (so == NULL) {
			printf("socket:\n");
			return false;
		}

		if (npf_test_getif(t->ifname) == NULL) {
			printf("Interface %s is not configured.\n", t->ifname);
			return false;
		}

		error = run_raw_testcase(i, verbose);
		serror = run_handler_testcase(i);

		if (verbose) {
			printf("rule test %d:\texpected %d (stateful) and %d\n"
			    "\t\t-> returned %d and %d\n",
			    i + 1, t->stateful_ret, t->ret, serror, error);
		}
		CHECK_TRUE(error == t->ret);
		CHECK_TRUE(serror == t->stateful_ret)

		soclose(so);
	}
	return true;
}

bool
npf_guid_test(bool verbose)
{
	soinit1();

	bool ok;

	ok = test_static(verbose);
	CHECK_TRUE(ok);

	return true;
}