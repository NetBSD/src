/*	$NetBSD: lagg.c,v 1.3 2022/03/31 01:53:22 yamaguchi Exp $	*/

/*
 * Copyright (c) 2021 Internet Initiative Japan Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#if !defined(lint)
__RCSID("$NetBSD: lagg.c,v 1.3 2022/03/31 01:53:22 yamaguchi Exp $");
#endif /* !defined(lint) */

#include <sys/param.h>

#include <sys/ioctl.h>

#include <net/if.h>
#include <net/if_ether.h>
#include <net/if_lagg.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <util.h>

#include "env.h"
#include "extern.h"
#include "util.h"

static status_func_t	 status;
static usage_func_t	 usage;
static cmdloop_branch_t	 branch;

static void	lagg_constructor(void) __attribute__((constructor));
static void	lagg_status(prop_dictionary_t, prop_dictionary_t);
static void	lagg_usage(prop_dictionary_t);

static int	setlaggproto(prop_dictionary_t, prop_dictionary_t);
static int	setlaggport(prop_dictionary_t, prop_dictionary_t);
static int	setlagglacp(prop_dictionary_t, prop_dictionary_t);
static int	setlagglacpmaxports(prop_dictionary_t, prop_dictionary_t);
static int	setlaggfail(prop_dictionary_t, prop_dictionary_t);
static void	lagg_status_proto(lagg_proto, struct laggreqproto *);
static void	lagg_status_port(lagg_proto, struct laggreqport *);

#ifdef LAGG_DEBUG
static const bool	 lagg_debug = true;
#else
static const bool	 lagg_debug = false;
#endif

#define LAGG_RETRY_MAX 10

static const char	*laggprotostr[LAGG_PROTO_MAX] = {
	[LAGG_PROTO_NONE] = "none",
	[LAGG_PROTO_LACP] = "lacp",
	[LAGG_PROTO_FAILOVER] = "failover",
	[LAGG_PROTO_LOADBALANCE] = "loadbalance",
};

enum laggportcmd {
	LAGGPORT_NOCMD = 1,
	LAGGPORT_ADD,
	LAGGPORT_DEL,
};

enum lagglacpcmd {
	LAGGLACP_ADD = 1,
	LAGGLACP_DEL,
};

enum lagglacpopt {
	LAGGLACPOPT_DUMPDU = 1,
	LAGGLACPOPT_STOPDU,
	LAGGLACPOPT_OPTIMISTIC,
	LAGGLACPOPT_MULTILS,
};

enum laggfailopt {
	LAGGFAILOPT_RXALL = 1,
};

struct pbranch	 laggport_root;
struct pbranch	 lagglacp_root;
struct pbranch	 laggfail_root;

struct pstr	 laggproto = PSTR_INITIALIZER(&laggproto, "lagg-protocol",
		    setlaggproto, "laggproto", &command_root.pb_parser);
struct piface	 laggaddport = PIFACE_INITIALIZER(&laggaddport,
		    "lagg-port-interface", setlaggport, "laggport",
		    &laggport_root.pb_parser);
struct piface	 laggdelport = PIFACE_INITIALIZER(&laggdelport,
		    "lagg-port-interface", setlaggport, "laggport",
		    &command_root.pb_parser);
struct pinteger	 laggportoptpri =  PINTEGER_INITIALIZER1(&laggportoptpri,
		    "lagg-port-priority", 0, UINT16_MAX, 10,
		    setlaggport, "laggportpri", &laggport_root.pb_parser);
struct pinteger	 lagglacpmaxports = PINTEGER_INITIALIZER1(&lagglacpmaxports,
		    "lagg-lacp-maxports", 1, UINT32_MAX, 10,
		    setlagglacpmaxports, "lacpmaxports",
		    &lagglacp_root.pb_parser);
struct pinteger	 laggportpri_num = PINTEGER_INITIALIZER1(&laggportpri_num,
		    "lagg-port-priority", 0, UINT16_MAX, 10,
		    setlaggport, "laggportpri", &command_root.pb_parser);
struct piface	 laggportpri_if = PIFACE_INITIALIZER(&laggportpri_if,
		    "lagg-port-interface", NULL, "laggport",
		    &laggportpri_num.pi_parser);

static const struct kwinst	 lagglacpkw[] = {
	  {.k_word = "dumpdu", .k_key = "lacpdumpdu",
	   .k_type = KW_T_INT, .k_int = LAGGLACPOPT_DUMPDU}
	, {.k_word = "-dumpdu", .k_key = "lacpdumpdu",
	   .k_type = KW_T_INT, .k_int = -LAGGLACPOPT_DUMPDU}
	, {.k_word = "stopdu", .k_key = "lacpstopdu",
	   .k_type = KW_T_INT, .k_int = LAGGLACPOPT_STOPDU}
	, {.k_word = "-stopdu", .k_key = "lacpstopdu",
	   .k_type = KW_T_INT, .k_int = -LAGGLACPOPT_STOPDU}
	, {.k_word = "optimistic", .k_key = "lacpoptimistic",
	   .k_type = KW_T_INT, .k_int = LAGGLACPOPT_OPTIMISTIC}
	, {.k_word = "-optimistic", .k_key = "lacpoptimistic",
	   .k_type = KW_T_INT, .k_int = -LAGGLACPOPT_OPTIMISTIC}
	, {.k_word = "maxports", .k_nextparser = &lagglacpmaxports.pi_parser}
	, {.k_word = "-maxports", .k_key = "lacpmaxports",
	   .k_type = KW_T_INT, .k_int = 0}
	, {.k_word = "multi-linkspeed", .k_key = "lacpmultils",
	   .k_type = KW_T_INT, .k_int = LAGGLACPOPT_MULTILS}
	, {.k_word = "-multi-linkspeed", .k_key = "lacpmultils",
	   .k_type = KW_T_INT, .k_int = -LAGGLACPOPT_MULTILS}
};
struct pkw	 lagglacp = PKW_INITIALIZER(&lagglacp, "lagg-lacp-option",
		    setlagglacp, NULL, lagglacpkw, __arraycount(lagglacpkw),
		    &lagglacp_root.pb_parser);

static const struct kwinst	 laggfailkw[] = {
	  {.k_word = "rx-all", .k_key = "failrxall",
	   .k_type = KW_T_INT, .k_int = LAGGFAILOPT_RXALL}
	, {.k_word = "-rx-all", .k_key = "failrxall",
	   .k_type = KW_T_INT, .k_int = -LAGGFAILOPT_RXALL}
};
struct pkw	 laggfail = PKW_INITIALIZER(&laggfail, "lagg-failover-option",
		    setlaggfail, NULL, laggfailkw, __arraycount(laggfailkw),
		    &laggfail_root.pb_parser);

static const struct kwinst	 laggkw[] = {
	  {.k_word = "laggproto", .k_nextparser = &laggproto.ps_parser}
	, {.k_word = "-laggproto", .k_key = "laggproto", .k_type = KW_T_STR,
	   .k_str = "none", .k_exec = setlaggproto}
	, {.k_word = "laggport", .k_key = "laggportcmd", .k_type = KW_T_INT,
	   .k_int = LAGGPORT_ADD, .k_nextparser = &laggaddport.pif_parser}
	, {.k_word = "-laggport", .k_key = "laggportcmd", .k_type = KW_T_INT,
	   .k_int = LAGGPORT_DEL, .k_nextparser = &laggdelport.pif_parser}
	, {.k_word = "lagglacp", .k_nextparser = &lagglacp.pk_parser}
	, {.k_word = "laggportpri", .k_nextparser = &laggportpri_if.pif_parser}
	, {.k_word = "laggfailover", .k_nextparser = &laggfail.pk_parser}
};
struct pkw	 lagg = PKW_INITIALIZER(&lagg, "lagg", NULL, NULL,
		    laggkw, __arraycount(laggkw), NULL);

static const struct kwinst	 laggportkw[] = {
	  {.k_word = "pri", .k_nextparser = &laggportoptpri.pi_parser}
};
struct pkw	laggportopt = PKW_INITIALIZER(&laggportopt, "lagg-port-option",
		    NULL, NULL, laggportkw, __arraycount(laggportkw), NULL);

struct branch	 laggport_brs[] = {
	  {.b_nextparser = &laggportopt.pk_parser}
	, {.b_nextparser = &command_root.pb_parser}
};
struct branch	 lagglacp_brs[] = {
	  {.b_nextparser = &lagglacp.pk_parser}
	, {.b_nextparser = &command_root.pb_parser}
};
struct branch	 laggfail_brs[] = {
	  {.b_nextparser = &laggfail.pk_parser}
	, {.b_nextparser = &command_root.pb_parser}
};

static void
lagg_constructor(void)
{
	struct pbranch _laggport_root = PBRANCH_INITIALIZER(&laggport_root,
	    "laggport-root", laggport_brs, __arraycount(laggport_brs), true);
	struct pbranch _lagglacp_root = PBRANCH_INITIALIZER(&lagglacp_root,
	    "lagglacp-root", lagglacp_brs, __arraycount(lagglacp_brs), true);
	struct pbranch _laggfail_root = PBRANCH_INITIALIZER(&laggfail_root,
	    "laggfail-root", laggfail_brs, __arraycount(laggfail_brs), true);

	laggport_root = _laggport_root;
	lagglacp_root = _lagglacp_root;
	laggfail_root = _laggfail_root;

	cmdloop_branch_init(&branch, &lagg.pk_parser);
	status_func_init(&status, lagg_status);
	usage_func_init(&usage, lagg_usage);

	register_cmdloop_branch(&branch);
	register_status(&status);
	register_usage(&usage);
}

static int
is_laggif(prop_dictionary_t env)
{
	const char *ifname;
	size_t i, len;

	if ((ifname = getifname(env)) == NULL)
		return 0;

	if (strncmp(ifname, "lagg", 4) != 0)
		return 0;

	len = strlen(ifname);
	for (i = 4; i < len; i++) {
		if (!isdigit((unsigned char)ifname[i]))
			return 0;
	}

	return 1;
}

static struct lagg_req *
getlagg(prop_dictionary_t env)
{
	struct lagg_req *req = NULL, *p;
	size_t nports, bufsiz;
	int i;

	if (!is_laggif(env)) {
		if (lagg_debug)
			warnx("valid only with lagg(4) interfaces");
		goto done;
	}

	for (i = 0, nports = 0; i < LAGG_RETRY_MAX; i++) {
		bufsiz = sizeof(*req);
		bufsiz += sizeof(req->lrq_reqports[0]) * nports;
		p = realloc(req, bufsiz);
		if (p == NULL)
			break;

		req = p;
		memset(req, 0, bufsiz);
		req->lrq_nports = nports;
		if (indirect_ioctl(env, SIOCGLAGG, req) == 0)
			goto done;

		if (errno != ENOBUFS)
			break;
		nports = req->lrq_nports + 3; /* 3: additional space */
	}

	if (req != NULL) {
		free(req);
		req = NULL;
	}

done:
	return req;
}

static void
freelagg(struct lagg_req *req)
{

	free(req);
}

static void
lagg_status(prop_dictionary_t env, prop_dictionary_t oenv)
{
	struct lagg_req *req;
	struct laggreqport *port;
	const char *proto;
	char str[256];
	size_t i;

	req = getlagg(env);
	if (req == NULL)
		return;

	if (req->lrq_proto >= LAGG_PROTO_MAX ||
	    (proto = laggprotostr[req->lrq_proto]) == NULL) {
		proto = "unknown";
	}

	printf("\tlaggproto %s", proto);
	if (vflag)
		lagg_status_proto(req->lrq_proto, &req->lrq_reqproto);
	putchar('\n');

	if (req->lrq_nports > 0) {
		printf("\tlaggport:\n");
		for (i = 0; i < req->lrq_nports; i++) {
			port = &req->lrq_reqports[i];
			snprintb(str, sizeof(str),
			    LAGG_PORT_BITS, port->rp_flags);

			printf("\t\t%.*s pri=%u flags=%s",
			    IFNAMSIZ, port->rp_portname,
			    (unsigned int)port->rp_prio,
			    str);
			if (vflag)
				lagg_status_port(req->lrq_proto, port);
			putchar('\n');
		}
	}

	freelagg(req);
}

static int
setlaggproto(prop_dictionary_t env, prop_dictionary_t oenv)
{
	prop_object_t obj;
	struct lagg_req req;
	const char *proto;
	size_t i, proto_len;

	memset(&req, 0, sizeof(req));

	obj = prop_dictionary_get(env, "laggproto");
	if (obj == NULL) {
		errno = ENOENT;
		return -1;
	}

	switch (prop_object_type(obj)) {
	case PROP_TYPE_DATA:
		proto = prop_data_value(obj);
		proto_len = prop_data_size(obj);
		break;
	case PROP_TYPE_STRING:
		proto = prop_string_value(obj);
		proto_len = prop_string_size(obj);
		break;
	default:
		errno = EFAULT;
		return -1;
	}

	for (i = 0; i < LAGG_PROTO_MAX; i++) {
		if (strncmp(proto, laggprotostr[i], proto_len) == 0)
			break;
	}

	if (i >= LAGG_PROTO_MAX) {
		errno = EPROTONOSUPPORT;
		return -1;
	}

	req.lrq_ioctl = LAGGIOC_SETPROTO;
	req.lrq_proto = i;

	if (indirect_ioctl(env, SIOCSLAGG, &req) == -1)
		return -1;

	return 0;
}

static int
setlaggport(prop_dictionary_t env, prop_dictionary_t oenv __unused)
{
	struct lagg_req req;
	struct laggreqport *rp;
	const char *ifname;
	enum lagg_ioctl ioc;
	int64_t lpcmd, pri;

	if (!prop_dictionary_get_string(env, "laggport", &ifname)) {
		if (lagg_debug)
			warnx("%s.%d", __func__, __LINE__);
		errno = ENOENT;
		return -1;
	}

	memset(&req, 0, sizeof(req));
	req.lrq_nports = 1;
	rp = &req.lrq_reqports[0];
	strlcpy(rp->rp_portname, ifname, sizeof(rp->rp_portname));
	ioc = LAGGIOC_NOCMD;

	if (prop_dictionary_get_int64(env, "laggportcmd", &lpcmd)) {
		if (lpcmd == LAGGPORT_ADD) {
			ioc = LAGGIOC_ADDPORT;
		} else {
			ioc = LAGGIOC_DELPORT;
		}
	}

	if (prop_dictionary_get_int64(env, "laggportpri", &pri)) {
		ioc = LAGGIOC_SETPORTPRI;
		rp->rp_prio = (uint32_t)pri;
	}

	if (ioc != LAGGIOC_NOCMD) {
		req.lrq_ioctl = ioc;
		if (indirect_ioctl(env, SIOCSLAGG, &req) == -1) {
			if (lagg_debug) {
				warn("cmd=%d", ioc);
			}
			return -1;
		}
	}

	return 0;
}

static int
setlagglacp(prop_dictionary_t env, prop_dictionary_t oenv __unused)
{
	struct lagg_req req_add, req_del;
	struct laggreq_lacp *add_lacp, *del_lacp;
	int64_t v;

	memset(&req_add, 0, sizeof(req_add));
	memset(&req_del, 0, sizeof(req_del));

	req_add.lrq_proto = req_del.lrq_proto = LAGG_PROTO_LACP;
	req_add.lrq_ioctl = req_del.lrq_ioctl = LAGGIOC_SETPROTOOPT;
	add_lacp = &req_add.lrq_reqproto.rp_lacp;
	del_lacp = &req_del.lrq_reqproto.rp_lacp;

	add_lacp->command = LAGGIOC_LACPSETFLAGS;
	del_lacp->command = LAGGIOC_LACPCLRFLAGS;

	if (prop_dictionary_get_int64(env, "lacpdumpdu", &v)) {
		if (v == LAGGLACPOPT_DUMPDU) {
			add_lacp->flags |= LAGGREQLACP_DUMPDU;
		} else {
			del_lacp->flags |= LAGGREQLACP_DUMPDU;
		}
	}

	if (prop_dictionary_get_int64(env, "lacpstopdu", &v)) {
		if (v == LAGGLACPOPT_STOPDU) {
			add_lacp->flags |= LAGGREQLACP_STOPDU;
		} else {
			del_lacp->flags |= LAGGREQLACP_STOPDU;
		}
	}

	if (prop_dictionary_get_int64(env, "lacpoptimistic", &v)) {
		if (v == LAGGLACPOPT_OPTIMISTIC) {
			add_lacp->flags |= LAGGREQLACP_OPTIMISTIC;
		} else {
			del_lacp->flags |= LAGGREQLACP_OPTIMISTIC;
		}
	}

	if (prop_dictionary_get_int64(env, "lacpmultils", &v)) {
		if (v == LAGGLACPOPT_MULTILS) {
			add_lacp->flags |= LAGGREQLACP_MULTILS;
		} else {
			del_lacp->flags |= LAGGREQLACP_MULTILS;
		}
	}

	if (del_lacp->flags != 0) {
		if (indirect_ioctl(env, SIOCSLAGG, &req_del) == -1) {
			if (lagg_debug) {
				warn("cmd=%d, pcmd=%d",
				    req_del.lrq_ioctl,
				    del_lacp->command);
			}
			return -1;
		}
	}

	if (add_lacp->flags != 0) {
		if (indirect_ioctl(env, SIOCSLAGG, &req_add) == -1) {
			if (lagg_debug) {
				warn("cmd=%d, pcmd=%d",
				    req_add.lrq_ioctl,
				    add_lacp->command);
			}
			return -1;
		}
	}

	return 0;
}

static int
setlagglacpmaxports(prop_dictionary_t env,
    prop_dictionary_t oenv __unused)
{
	struct lagg_req req;
	struct laggreq_lacp *lrq_lacp;
	int64_t v;

	memset(&req, 0, sizeof(req));
	req.lrq_proto = LAGG_PROTO_LACP;
	req.lrq_ioctl = LAGGIOC_SETPROTOOPT;
	lrq_lacp = &req.lrq_reqproto.rp_lacp;

	if (!prop_dictionary_get_int64(env, "lacpmaxports", &v)) {
		if (lagg_debug)
			warnx("%s.%d", __func__, __LINE__);
		errno = ENOENT;
		return -1;
	}

	if (v <= 0) {
		lrq_lacp->command = LAGGIOC_LACPCLRMAXPORTS;
	} else if (v > 0){
		lrq_lacp->command = LAGGIOC_LACPSETMAXPORTS;
		lrq_lacp->maxports = (size_t)v;
	}

	if (indirect_ioctl(env, SIOCSLAGG, &req) == -1) {
		err(EXIT_FAILURE, "SIOCSLAGGPROTO");
	}

	return 0;
}

static int
setlaggfail(prop_dictionary_t env,
    prop_dictionary_t oenv __unused)
{
	struct lagg_req req_add, req_del;
	struct laggreq_fail *add_fail, *del_fail;
	int64_t v;

	memset(&req_add, 0, sizeof(req_add));
	memset(&req_del, 0, sizeof(req_del));

	req_add.lrq_proto = req_del.lrq_proto = LAGG_PROTO_FAILOVER;
	req_add.lrq_ioctl = req_del.lrq_ioctl = LAGGIOC_SETPROTOOPT;
	add_fail = &req_add.lrq_reqproto.rp_fail;
	del_fail = &req_del.lrq_reqproto.rp_fail;

	add_fail->command = LAGGIOC_FAILSETFLAGS;
	del_fail->command = LAGGIOC_FAILCLRFLAGS;

	if (prop_dictionary_get_int64(env, "failrxall", &v)) {
		if (v == LAGGFAILOPT_RXALL) {
			add_fail->flags |= LAGGREQFAIL_RXALL;
		} else {
			del_fail->flags |= LAGGREQFAIL_RXALL;
		}
	}

	if (del_fail->flags != 0) {
		if (indirect_ioctl(env, SIOCSLAGG, &req_del) == -1) {
			if (lagg_debug) {
				warn("cmd=%d, pcmd=%d",
				    req_del.lrq_ioctl,
				    del_fail->command);
			}
			return -1;
		}
	}

	if (add_fail->flags != 0) {
		if (indirect_ioctl(env, SIOCSLAGG, &req_add) == -1) {
			if (lagg_debug) {
				warn("cmd=%d, pcmd=%d",
				    req_add.lrq_ioctl,
				    add_fail->command);
			}
			return -1;
		}
	}

	return 0;
}

static void
lagg_usage(prop_dictionary_t env __unused)
{

	fprintf(stderr, "\t[ laggproto p ]\n");
	fprintf(stderr, "\t[ laggport i [ pri n ] ] "
	    "[ -laggport i ]\n");
	fprintf(stderr, "\t[ laggportpri i [ pri n]]\n");
	fprintf(stderr, "\t[ lagglacp [ dumpdu | -dumpdu ] "
	    "[ stopdu | -stopdu ]\n"
	    "\t\t[ maxports n | -maxports ] [ optimistic | -optimistic ] ]\n");
	fprintf(stderr, "\t[ laggfailover] [ rx-all | -rx-all ]\n");
}
static void
lacp_format_id(char *buf, size_t len,
    uint16_t system_prio, uint8_t *system_mac, uint16_t system_key)
{

	snprintf(buf, len, "[%04X,%02X-%02X-%02X-%02X-%02X-%02X,"
	    "%04X]",
	    system_prio,
	    (unsigned int)system_mac[0],(unsigned int)system_mac[1],
	    (unsigned int)system_mac[2],(unsigned int)system_mac[3],
	    (unsigned int)system_mac[4],(unsigned int)system_mac[5],
	    system_key);
}

static void
lagg_status_proto(lagg_proto pr, struct laggreqproto *req)
{
	struct laggreq_lacp *lacp;
	char str[256];

	switch (pr) {
	case LAGG_PROTO_LACP:
		lacp = &req->rp_lacp;

		printf("\n");
		snprintb(str, sizeof(str), LAGGREQLACP_BITS,
		    lacp->flags);
		printf("\t\tmax ports=%zu, flags=%s\n",
		    lacp->maxports, str);

		lacp_format_id(str, sizeof(str), lacp->actor_prio,
		    lacp->actor_mac, lacp->actor_key);
		printf("\t\tactor=%s\n", str);

		lacp_format_id(str, sizeof(str), lacp->partner_prio,
		    lacp->partner_mac, lacp->partner_key);
		printf("\t\tpartner=%s", str);
		break;
	default:
		break;
	}
}

static void
lagg_status_port(lagg_proto pr, struct laggreqport *req)
{
	struct laggreq_lacpport *lacp;
	char str[256];

	switch (pr) {
	case LAGG_PROTO_LACP:
		lacp = &req->rp_lacpport;

		putchar('\n');

		snprintb(str, sizeof(str), LACP_STATE_BITS, lacp->actor_state);
		printf("\t\t\tactor: state=%s\n",str);

		lacp_format_id(str, sizeof(str), lacp->partner_prio,
		    lacp->partner_mac, lacp->partner_key);
		printf("\t\t\tpartner=%s\n", str);
		snprintb(str, sizeof(str), LACP_STATE_BITS,
		    lacp->partner_state);
		printf("\t\t\tpartner: port=%04X prio=%04X state=%s",
		    lacp->partner_portno, lacp->partner_portprio, str);
		break;
	default:
		break;
	}
}
