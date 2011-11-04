/*	$NetBSD: npf_parser.c,v 1.7 2011/11/04 01:00:28 zoltan Exp $	*/

/*-
 * Copyright (c) 2009-2011 The NetBSD Foundation, Inc.
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

/*
 * XXX: This needs clean-up!
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: npf_parser.c,v 1.7 2011/11/04 01:00:28 zoltan Exp $");

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <err.h>

#include "npfctl.h"

/*
 * Few ugly helpers.
 */

#define	PARSE_ERR()			(-1)

#define	PARSE_TOKEN(_arg_)					\
	if ((p = strtok_r(_arg_, " \t", &sptr)) == NULL)	\
		return PARSE_ERR();

#define	PARSE_FIRST_TOKEN()		PARSE_TOKEN(p)
#define	PARSE_NEXT_TOKEN()		PARSE_TOKEN(NULL)
#define	PARSE_NEXT_TOKEN_NOCHECK()	p = strtok_r(NULL, " \t", &sptr)

/*
 * Global variable list.
 *
 * npfctl_lookup_varlist(): lookups the list by key.
 */

static var_t *			var_list = NULL;

static var_t *
npfctl_lookup_varlist(char *key)
{
	var_t *it;

	for (it = var_list; it != NULL; it = it->v_next)
		if (strcmp(it->v_key, key) == 0)
			break;
	return it;
}

/*
 * npfctl_parsevalue: helper function to parse a value.
 *
 * => Value could be a single element (no quotes),
 * => or an array of elements between { }.
 */
static var_t *
npfctl_parsevalue(char *buf)
{
	var_t *vr = NULL;
	element_t *el = NULL, *it = NULL;
	char *p = buf, *tend, *sptr;

	switch (*p) {
	case '$':
		/* Definition - lookup. */
		vr = npfctl_lookup_varlist(++p);
		if (vr == NULL) {
			errx(EXIT_FAILURE, "variable '%s' is not defined", p);
		}
		break;
	case '{':
		/* Array. */
		vr = zalloc(sizeof(var_t));
		p = strtok_r(buf, ", \t", &sptr);
		while (p) {
			if (*p == '}')
				break;
			el = zalloc(sizeof(element_t));
			el->e_data = xstrdup(p);
			el->e_next = it;
			vr->v_count++;
			it = el;
			p = strtok_r(NULL, ", \t", &sptr);
		}
		if (el) {
			vr->v_type = VAR_ARRAY;
			vr->v_elements = el;
		} else {
			free(vr);
			vr = NULL;
		}
		break;
	case '<':
		/* Table. */
		if ((tend = strchr(++p, '>')) == NULL) {
			return NULL;
		}
		*tend = '\0';
		if (!npf_table_exists_p(npf_conf, (u_int)atoi(p))) {
			errx(EXIT_FAILURE, "table '%s' is not defined", p);
		}
		vr = zalloc(sizeof(var_t));
		vr->v_type = VAR_TABLE;
		/* FALLTHROUGH */
	default:
		/* Data. */
		el = zalloc(sizeof(element_t));
		el->e_data = xstrdup(p);
		if (vr == NULL) {
			vr = zalloc(sizeof(var_t));
			vr->v_type = VAR_SINGLE;
		}
		vr->v_elements = el;
		vr->v_count = 1;
	}
	return vr;
}

static char *
npfctl_val_single(var_t *v, char *p)
{
	element_t *el;

	if (v->v_type != VAR_SINGLE) {
		errx(EXIT_FAILURE, "multiple elements in variable '%s'", p);
	}
	el = v->v_elements;
	return el->e_data;
}

static u_int
npfctl_val_interface(var_t *v, char *p, bool reqaddr)
{
	char *iface = npfctl_val_single(v, p);
	u_int if_idx;

	if (iface == NULL ||
		((npfctl_getif(iface, &if_idx, reqaddr, AF_INET) == NULL) &&
		 (npfctl_getif(iface, &if_idx, reqaddr, AF_INET6) == NULL))) {
		errx(EXIT_FAILURE, "invalid interface '%s'", iface);
	}
	return if_idx;
}

static int
npfctl_parsenorm(char *buf, nl_rproc_t *rp)
{
	char *p = buf, *sptr;
	int minttl = 0, maxmss = 0;
	bool rnd = false, no_df = false;

	p = strtok_r(buf, ", \t", &sptr);
	if (p == NULL) {
		return -1;
	}
	do {
		if (strcmp(p, "random-id") == 0) {
			rnd = true;
		} else if (strcmp(p, "min-ttl") == 0) {
			p = strtok_r(NULL, ", \t", &sptr);
			minttl = atoi(p);
		} else if (strcmp(p, "max-mss") == 0) {
			p = strtok_r(NULL, ", \t", &sptr);
			maxmss = atoi(p);
		} else if (strcmp(p, "no-df") == 0) {
			no_df = true;
		} else {
			return -1;
		}
	} while ((p = strtok_r(NULL, ", \t", &sptr)) != 0);

	return _npf_rproc_setnorm(rp, rnd, no_df, minttl, maxmss);
}

static int
npfctl_parserproc(char *buf, nl_rproc_t **rp)
{
	char *p = buf, *end;

	DPRINTF(("rproc\t|%s|\n", buf));

	if ((p = strchr(p, '"')) == NULL)
		return -1;
	if ((end = strchr(++p, '"')) == NULL)
		return -1;
	*end = '\0';

	*rp = npf_rproc_create(p);
	if (*rp == NULL) {
		return -1;
	}
	return npf_rproc_insert(npf_conf, *rp) ? -1 : 0;
}

static int
npfctl_parserproc_lines(char *buf, nl_rproc_t *rp)
{
	char *p = buf, *sptr;

	DPRINTF(("rproc\t|%s|\n", p));
	PARSE_FIRST_TOKEN();

	/* log <interface> */
	if (strcmp(p, "log") == 0) {
		var_t *ifvar;
		u_int if_idx;

		PARSE_NEXT_TOKEN();
		if ((ifvar = npfctl_parsevalue(p)) == NULL)
			return PARSE_ERR();
		if_idx = npfctl_val_interface(ifvar, p, false);
		(void)_npf_rproc_setlog(rp, if_idx);

	} else if (strcmp(p, "normalize") == 0) {
		/* normalize ( .. ) */
		p = strtok_r(NULL, "()", &sptr);
		if (p == NULL) {
			return PARSE_ERR();
		}
		if (npfctl_parsenorm(p, rp)) {
			return PARSE_ERR();
		}
		PARSE_NEXT_TOKEN_NOCHECK();
	}
	return 0;
}

/*
 * npfctl_parserule: main routine to parse a rule.  Syntax:
 *
 *	{ pass | block } [ in | out ] [ quick ]
 *	    [on <if>] [inet | inet6 ] proto <array>
 *	    from <addr/mask> port <port(s)|range>
 *	    to <addr/mask> port <port(s)|range>
 *	    [ keep state ] [ apply "<rproc>" ]
 */
static int
npfctl_parserule(char *buf, nl_rule_t *parent)
{
	var_t *from_v = NULL, *fports = NULL, *to_v = NULL, *tports = NULL;
	char *p, *sptr, *proto = NULL, *tcp_flags = NULL, *rproc = NULL;
	int icmp_type = -1, icmp_code = -1;
	bool icmp = false, tcp = false;
	u_int if_idx = 0;
	int ret, attr = 0;
	nl_rule_t *rl;
	sa_family_t addrfamily = AF_UNSPEC;

	DPRINTF(("rule\t|%s|\n", buf));

	p = buf;
	PARSE_FIRST_TOKEN();

	/* pass or block (mandatory) */
	if (strcmp(p, "block") == 0) {
		PARSE_NEXT_TOKEN();
		/* return-rst or return-icmp */
		if (strcmp(p, "return-rst") == 0) {
			attr |= NPF_RULE_RETRST;
			PARSE_NEXT_TOKEN();
		} else if (strcmp(p, "return-icmp") == 0) {
			attr |= NPF_RULE_RETICMP;
			PARSE_NEXT_TOKEN();
		} else if (strcmp(p, "return") == 0) {
			attr |= NPF_RULE_RETRST | NPF_RULE_RETICMP;
			PARSE_NEXT_TOKEN();
		}
	} else if (strcmp(p, "pass") == 0) {
		attr |= NPF_RULE_PASS;
		PARSE_NEXT_TOKEN();
	} else {
		return PARSE_ERR();
	}

	/* in or out */
	if (strcmp(p, "in") == 0) {
		attr |= NPF_RULE_IN;
		PARSE_NEXT_TOKEN();
	} else if (strcmp(p, "out") == 0) {
		attr |= NPF_RULE_OUT;
		PARSE_NEXT_TOKEN();
	} else {
		attr |= (NPF_RULE_IN | NPF_RULE_OUT);
	}

	/* quick */
	if (strcmp(p, "quick") == 0) {
		attr |= NPF_RULE_FINAL;
		PARSE_NEXT_TOKEN();
	}

	/* on <interface> */
	if (strcmp(p, "on") == 0) {
		var_t *ifvar;

		PARSE_NEXT_TOKEN();
		if ((ifvar = npfctl_parsevalue(p)) == NULL)
			return PARSE_ERR();
		if_idx = npfctl_val_interface(ifvar, p, true);
		PARSE_NEXT_TOKEN();
	}

	if (strcmp(p, "inet") == 0) {
		addrfamily = AF_INET;
		PARSE_NEXT_TOKEN();
	} else if (strcmp(p, "inet6") == 0) {
		addrfamily = AF_INET6;
		PARSE_NEXT_TOKEN();
	}

	/* proto <proto> */
	if (strcmp(p, "proto") == 0) {
		PARSE_NEXT_TOKEN();
		var_t *pvar = npfctl_parsevalue(p);
		PARSE_NEXT_TOKEN();
		if (pvar->v_type != VAR_SINGLE) {
			errx(EXIT_FAILURE, "only one protocol can be specified");
		}
		element_t *el = pvar->v_elements;
		proto = el->e_data;
		/* Determine TCP, ICMP. */
		tcp = (strcmp(proto, "tcp") == 0);
		icmp = (strcmp(proto, "icmp") == 0);
	}

	/*
	 * Can be: "all", "from", "to" or "from + to".
	 */

	if (strcmp(p, "all") == 0) {
		/* Should be no "from"/"to" after it. */
		PARSE_NEXT_TOKEN_NOCHECK();
		goto last;
	}

	ret = PARSE_ERR();

	/* from <addr> port <port | range> */
	if (strcmp(p, "from") == 0) {
		PARSE_NEXT_TOKEN();
		if (addrfamily == AF_UNSPEC)
			addrfamily = npfctl_get_addrfamily(p);
		from_v = npfctl_parsevalue(p);

		PARSE_NEXT_TOKEN_NOCHECK();
		if (p && strcmp(p, "port") == 0) {
			PARSE_NEXT_TOKEN();
			fports = npfctl_parsevalue(p);
			PARSE_NEXT_TOKEN_NOCHECK();
		}
		ret = 0;
	}

	/* to <addr> port <port | range> */
	if (p && strcmp(p, "to") == 0) {
		PARSE_NEXT_TOKEN();
		if (addrfamily == AF_UNSPEC)
			addrfamily = npfctl_get_addrfamily(p);
		to_v = npfctl_parsevalue(p);

		PARSE_NEXT_TOKEN_NOCHECK();
		if (p && strcmp(p, "port") == 0) {
			PARSE_NEXT_TOKEN();
			tports = npfctl_parsevalue(p);
			PARSE_NEXT_TOKEN_NOCHECK();
		}
		ret = 0;
	}

	if (ret) {
		return ret;
	}

	/* flags <fl/mask> */
	if (p && strcmp(p, "flags") == 0) {
		if (icmp) {
			errx(EXIT_FAILURE,
			    "TCP flags used with ICMP protocol");
		}
		PARSE_NEXT_TOKEN();
		var_t *tfvar = npfctl_parsevalue(p);
		tcp_flags = npfctl_val_single(tfvar, p);
		PARSE_NEXT_TOKEN_NOCHECK();
	}

	/* icmp-type <t> code <c> */
	if (p && strcmp(p, "icmp-type") == 0) {
		if (tcp) {
			errx(EXIT_FAILURE,
			    "ICMP options used with TCP protocol");
		}
		PARSE_NEXT_TOKEN();
		icmp_type = atoi(p);
		PARSE_NEXT_TOKEN_NOCHECK();
		if (p && strcmp(p, "code") == 0) {
			PARSE_NEXT_TOKEN();
			icmp_code = atoi(p);
			PARSE_NEXT_TOKEN_NOCHECK();
		}
	}
last:
	/* keep state */
	if (p && strcmp(p, "keep") == 0) {
		attr |= NPF_RULE_KEEPSTATE;
		PARSE_NEXT_TOKEN();
		if (p == NULL || strcmp(p, "state") != 0) {
			return PARSE_ERR();
		}
		PARSE_NEXT_TOKEN_NOCHECK();
	}

	/* apply "<rproc>" */
	if (p && strcmp(p, "apply") == 0) {
		char *end;
		PARSE_NEXT_TOKEN();
		if ((p = strchr(p, '"')) == NULL)
			return PARSE_ERR();
		if ((end = strchr(++p, '"')) == NULL)
			return PARSE_ERR();
		*end = '\0';
		if (!npf_rproc_exists_p(npf_conf, p)) {
			errx(EXIT_FAILURE, "procedure '%s' is not defined", p);
		}
		rproc = p;
		PARSE_NEXT_TOKEN_NOCHECK();
	}

	/* Should have nothing more. */
	if (p != NULL) {
		return PARSE_ERR();
	}

	/*
	 * Set the rule attributes and interface.  Generate all protocol data.
	 */
	rl = npf_rule_create(NULL, attr, if_idx);
	npfctl_rule_ncode(rl, proto, tcp_flags, icmp_type, icmp_code,
	    from_v, addrfamily, fports, to_v, tports);
	if (rproc && npf_rule_setproc(npf_conf, rl, rproc) != 0) {
		errx(EXIT_FAILURE, "procedure '%s' is not defined", rproc);
	}
	npf_rule_insert(npf_conf, parent, rl, NPF_PRI_NEXT);
	return 0;
}

/*
 * npfctl_parsegroup: parse group definition.  Syntax:
 *
 *	group (name <name>, interface <if>, [ in | out ]) { <rules> }
 *	group (default) { <rules> }
 */

#define	GROUP_ATTRS	(NPF_RULE_PASS | NPF_RULE_FINAL)

static int
npfctl_parsegroup(char *buf, nl_rule_t **rl)
{
	char *p = buf, *end, *sptr, *rname = NULL;
	u_int if_idx = 0;
	int attr_dir;

	DPRINTF(("group\t|%s|\n", buf));

	p = strchr(p, '(');
	if (p == NULL)
		return -1;
	*p = '\0';
	end = strchr(++p, ')');
	if (end == NULL)
		return -1;
	*end = '\0';
	if (strchr(++end, '{') == NULL)
		return -1;
	while (isspace((unsigned char)*p))
		p++;

	/*
	 * If default group - no other options.
	 */
	if (strcmp(p, "default") == 0) {
		attr_dir = NPF_RULE_DEFAULT | (NPF_RULE_IN | NPF_RULE_OUT);
		goto done;
	}

	PARSE_FIRST_TOKEN();

	/* Name of the group (mandatory). */
	if (strcmp(p, "name") == 0) {
		PARSE_NEXT_TOKEN()
		if (*p != '"')
			return -1;
		rname = ++p;
		if ((p = strchr(rname, '"')) == NULL)
			return -1;
		*p = '\0';
		PARSE_NEXT_TOKEN_NOCHECK();
	}

	/* Interface for this group (optional). */
	if (p && strcmp(p, "interface") == 0) {
		var_t *ifvar;
		PARSE_NEXT_TOKEN();
		if ((ifvar = npfctl_parsevalue(p)) == NULL)
			return -1;
		if_idx = npfctl_val_interface(ifvar, p, true);
		PARSE_NEXT_TOKEN_NOCHECK();
	}

	/* Direction (optional). */
	if (p) {
		if (strcmp(p, "in") == 0)
			attr_dir = NPF_RULE_IN;
		else if (strcmp(p, "out") == 0)
			attr_dir = NPF_RULE_OUT;
		else
			return -1;
	} else {
		attr_dir = NPF_RULE_IN | NPF_RULE_OUT;
	}
done:
	*rl = npf_rule_create(rname, GROUP_ATTRS | attr_dir, if_idx);
	if (*rl == NULL) {
		return -1;
	}
	npf_rule_insert(npf_conf, NULL, *rl, NPF_PRI_NEXT);
	return 0;
}

/*
 * npfctl_parsetable: parse table definition.
 *
 *	table <num> type <t> [ dynamic | file <path> ]
 */
static int
npfctl_parsetable(char *buf)
{
	char *p, *sptr, *fname;
	unsigned int id, type;
	nl_table_t *tl;

	DPRINTF(("table\t|%s|\n", buf));

	/* Name of the set. */
	if ((p = strchr(buf, '"')) == NULL) {
		return PARSE_ERR();
	}
	id = atoi(++p);
	p = strchr(p, '"');
	*p++ = '\0';

	PARSE_FIRST_TOKEN();

	/* Table type (mandatory). */
	if (strcmp(p, "type") != 0) {
		return PARSE_ERR();
	}
	PARSE_NEXT_TOKEN_NOCHECK();
	if (p == NULL) {
		return PARSE_ERR();
	}
	if (strcmp(p, "hash") == 0) {
		type = NPF_TABLE_HASH;
	} else if (strcmp(p, "tree") == 0) {
		type = NPF_TABLE_RBTREE;
	} else {
		errx(EXIT_FAILURE, "invalid table type '%s'", p);
	}
	*p = '\0';

	/*
	 * Setup the table.
	 */
	tl = npf_table_create(id, type);
	if (npf_table_insert(npf_conf, tl)) {
		errx(EXIT_FAILURE, "table '%d' is already defined\n", id);
	}
	PARSE_NEXT_TOKEN();

	/* Dynamic. */
	if (strcmp(p, "dynamic") == 0) {
		/* No other options. */
		return 0;
	}

	/* File. */
	if (strcmp(p, "file") != 0) {
		return PARSE_ERR();
	}
	PARSE_NEXT_TOKEN();
	fname = ++p;
	p = strchr(p, '"');
	*p = '\0';

	/* Fill the table. */
	npfctl_fill_table(tl, fname);
	return 0;
}

/*
 * npfctl_parse_nat: parse NAT policy definition.
 *
 *	[bi]nat <if> from <net> to <net/addr> -> <ip>
 *	rdr <if> from <net> to <addr> -> <ip>
 *	nat <if> dynamic "<name>"
 */
static int
npfctl_parse_nat(char *buf)
{
	var_t *ifvar, *from_v, *to_v, *raddr_v;
	var_t *tports = NULL, *rport_v = NULL;
	char *p, *sptr, *raddr_s, *rport_s;
	npf_addr_t raddr;
	npf_netmask_t dummy;
	bool binat, rdr;
	nl_nat_t *nat;
	u_int if_idx;

	DPRINTF(("[bi]nat/rdr\t|%s|\n", buf));
	binat = (strncmp(buf, "binat", 5) == 0);
	rdr = (strncmp(buf, "rdr", 3) == 0);

	if ((p = strchr(buf, ' ')) == NULL) {
		return PARSE_ERR();
	}
	PARSE_FIRST_TOKEN();

	/* <interface> */
	if ((ifvar = npfctl_parsevalue(p)) == NULL) {
		return PARSE_ERR();
	}
	if_idx = npfctl_val_interface(ifvar, p, true);
	PARSE_NEXT_TOKEN();

	/* dynamic <name> */
	if (!binat && !rdr && strcmp(p, "dynamic") == 0) {
		char *nname;

		/* Parse name. */
		PARSE_NEXT_TOKEN()
		if (*p != '"')
			return PARSE_ERR();
		nname = ++p;
		if ((p = strchr(nname, '"')) == NULL)
			return PARSE_ERR();
		*p = '\0';

		/* Create a rule and insert into the NAT list. */
		nat = npf_rule_create(nname, NPF_RULE_PASS | NPF_RULE_FINAL |
		    NPF_RULE_OUT | NPF_RULE_IN, if_idx);
		(void)npf_nat_insert(npf_conf, nat, NPF_PRI_NEXT);
		return 0;
	}

	/* from <addr> */
	if (strcmp(p, "from") != 0) {
		return PARSE_ERR();
	}
	PARSE_NEXT_TOKEN();
	from_v = npfctl_parsevalue(p);
	PARSE_NEXT_TOKEN();

	/* to <addr> */
	if (strcmp(p, "to") != 0) {
		return PARSE_ERR();
	}
	PARSE_NEXT_TOKEN();
	to_v = npfctl_parsevalue(p);
	PARSE_NEXT_TOKEN();

	if (rdr && strcmp(p, "port") == 0) {
		PARSE_NEXT_TOKEN();
		tports = npfctl_parsevalue(p);
		PARSE_NEXT_TOKEN();
	}

	/* -> <ip> */
	if (strcmp(p, "->") != 0) {
		return PARSE_ERR();
	}
	PARSE_NEXT_TOKEN();
	raddr_v = npfctl_parsevalue(p);
	raddr_s = npfctl_val_single(raddr_v, p);
	npfctl_parse_cidr(raddr_s, npfctl_get_addrfamily(raddr_s), &raddr, &dummy);

	if (rdr) {
		PARSE_NEXT_TOKEN();
		if (strcmp(p, "port") != 0) {
			return PARSE_ERR();
		}
		PARSE_NEXT_TOKEN();
		rport_v = npfctl_parsevalue(p);
		rport_s = npfctl_val_single(rport_v, p);
	}

	/*
	 * Setup NAT policy (rule as filter and extra info), which is
	 * Outbound NAT (NPF_NATOUT).  Unless it is a redirect rule,
	 * in which case it is Inbound NAT with specified port.
	 *
	 * XXX mess
	 */
	if (!rdr) {
		nat = npf_nat_create(NPF_NATOUT,
		    binat ? 0 : (NPF_NAT_PORTS | NPF_NAT_PORTMAP),
		    if_idx, &raddr, AF_INET, 0);
	} else {
		in_port_t rport;
		bool range;

		if (!npfctl_parse_port(rport_s, &range, &rport, &rport)) {
			errx(EXIT_FAILURE, "invalid service '%s'", rport_s);
		}
		if (range) {
			errx(EXIT_FAILURE, "range is not supported for 'rdr'");
		}
		nat = npf_nat_create(NPF_NATIN, NPF_NAT_PORTS,
		    if_idx, &raddr, AF_INET, rport);
	}
	npfctl_rule_ncode(nat, NULL, NULL, -1, -1, from_v, AF_INET, NULL, to_v, tports);
	(void)npf_nat_insert(npf_conf, nat, NPF_PRI_NEXT);

	/*
	 * For bi-directional NAT case, create and setup additional
	 * Inbound NAT (NPF_NATIN) policy.  Note that translation address
	 * is local IP, and filter criteria is inverted accordingly.
	 *
	 * XXX mess
	 */
	if (binat) {
		char *taddr_s = npfctl_val_single(from_v, NULL);
		npf_addr_t taddr;
		nl_nat_t *bn;

		npfctl_parse_cidr(taddr_s, npfctl_get_addrfamily(taddr_s), &taddr, &dummy);
		bn = npf_nat_create(NPF_NATIN, 0, if_idx, &taddr, AF_INET, 0);
		npfctl_rule_ncode(bn, NULL, NULL, -1, -1,
		    to_v, AF_INET, NULL, raddr_v, NULL);
		(void)npf_nat_insert(npf_conf, bn, NPF_PRI_NEXT);
	}
	return 0;
}

/*
 * npfctl_parsevar: parse defined variable.
 *
 * => Assigned value should be with double quotes (").
 * => Value can be an array, use npf_parsevalue().
 * => Insert variable into the global list.
 */
static int
npfctl_parsevar(char *buf)
{
	char *s = buf, *p, *key;
	var_t *vr;

	DPRINTF(("def\t|%s|\n", buf));

	if ((p = strpbrk(s, "= \t")) == NULL)
		return PARSE_ERR();

	/* Validation of '='. */
	if (*p != '=' && strchr(p, '=') == NULL)
		return PARSE_ERR();
	*p = '\0';
	key = s;

	/* Check for duplicates. */
	if (npfctl_lookup_varlist(key))
		return PARSE_ERR();

	/* Parse quotes before. */
	if ((s = strchr(p + 1, '"')) == NULL)
		return PARSE_ERR();
	if ((p = strchr(++s, '"')) == NULL)
		return PARSE_ERR();
	*p = '\0';

	if ((vr = npfctl_parsevalue(s)) == NULL)
		return PARSE_ERR();
	vr->v_key = xstrdup(key);
	vr->v_next = var_list;
	var_list = vr;
	return 0;
}

/*
 * npf_parseline: main function parsing a single configuration line.
 *
 * Distinguishes 'group', rule (in-group), 'procedure', in-procedure,
 * 'table' and definitions.  Tracks begin-end of the group and procedure
 * i.e. in-group or in-procedure states.
 */
int
npf_parseline(char *buf)
{
	static nl_rule_t *curgr = NULL;
	static nl_rproc_t *currp = NULL;
	char *p = buf;
	int ret;

	/* Skip empty lines and comments. */
	while (isspace((unsigned char)*p))
		p++;
	if (*p == '\0' || *p == '\n' || *p == '#')
		return 0;

	/* At first, check if inside the group or rproc. */
	if (curgr) {
		/* End of the group. */
		if (*p == '}') {
			curgr = NULL;
			return 0;
		}
		/* Rule. */
		ret = npfctl_parserule(p, curgr);

	} else if (currp) {
		/* End of the procedure. */
		if (*p == '}') {
			currp = NULL;
			return 0;
		}
		/* Procedure contents. */
		ret = npfctl_parserproc_lines(p, currp);

	} else if (strncmp(p, "group", 5) == 0) {
		/* Group. */
		ret = npfctl_parsegroup(p, &curgr);

	} else if (strncmp(p, "procedure", 9) == 0) {
		/* Rule procedure. */
		ret = npfctl_parserproc(p, &currp);

	} else if (strncmp(p, "table", 5) == 0) {
		/* Table. */
		ret = npfctl_parsetable(p);

	} else if (strncmp(p, "nat", 3) == 0 || strncmp(p, "rdr", 3) == 0 ||
	    strncmp(p, "binat", 5) == 0) {
		/* NAT policy. */
		ret = npfctl_parse_nat(p);

	} else {
		/* Defined variable or syntax error. */
		ret = npfctl_parsevar(p);
	}
	return ret;
}
