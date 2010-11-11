/*	$NetBSD: npf_parser.c,v 1.3 2010/11/11 06:30:39 rmind Exp $	*/

/*-
 * Copyright (c) 2009-2010 The NetBSD Foundation, Inc.
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
__RCSID("$NetBSD: npf_parser.c,v 1.3 2010/11/11 06:30:39 rmind Exp $");

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
			errx(EXIT_FAILURE, "invalid variable '%s'", p);
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
		if (npfctl_lookup_table(p) == NULL) {
			errx(EXIT_FAILURE, "invalid table '%s'", p);
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

static inline int
npfctl_parsenorm(char *buf, bool *rnd, int *minttl, int *maxmss)
{
	char *p = buf, *sptr;

	DPRINTF(("norm\t|%s|\n", p));

	p = strtok_r(buf, ", \t", &sptr);
	if (p == NULL) {
		return -1;
	}
	do {
		if (strcmp(p, "random-id") == 0) {
			*rnd = true;
		} else if (strcmp(p, "min-ttl") == 0) {
			p = strtok_r(NULL, ", \t", &sptr);
			*minttl = atoi(p);
		} else if (strcmp(p, "max-mss") == 0) {
			p = strtok_r(NULL, ", \t", &sptr);
			*maxmss = atoi(p);
		} else {
			return -1;
		}
	} while ((p = strtok_r(NULL, ", \t", &sptr)) != 0);

	return 0;
}

/*
 * npfctl_parserule: main routine to parse a rule.  Syntax:
 *
 *	{ pass | block | count } [ in | out ] [ log ] [ quick ]
 *	    [on <if>] [inet | inet6 ] proto <array>
 *	    from <addr/mask> port <port(s)|range>
 *	    too <addr/mask> port <port(s)|range>
 *	    [ keep state ]
 */
static inline int
npfctl_parserule(char *buf, prop_dictionary_t rl)
{
	var_t *from_cidr = NULL, *fports = NULL;
	var_t *to_cidr = NULL, *tports = NULL;
	char *p, *sptr, *iface, *proto = NULL, *tcp_flags = NULL;
	int icmp_type = -1, icmp_code = -1, minttl = 0, maxmss = 0;
	bool icmp = false, tcp = false, rnd = false;
	int ret, attr = 0;

	DPRINTF(("rule\t|%s|\n", buf));

	p = buf;
	PARSE_FIRST_TOKEN();

	/* pass or block (mandatory) */
	if (strcmp(p, "block") == 0) {
		attr = 0;
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
		attr = NPF_RULE_PASS;
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

	/* log (XXX: NOP) */
	if (strcmp(p, "log") == 0) {
		attr |= NPF_RULE_LOG;
		PARSE_NEXT_TOKEN();
	}

	/* count */
	if (strcmp(p, "count") == 0) {
		attr |= NPF_RULE_COUNT;
		PARSE_NEXT_TOKEN();
	}

	/* quick */
	if (strcmp(p, "quick") == 0) {
		attr |= NPF_RULE_FINAL;
		PARSE_NEXT_TOKEN();
	}

	/* on <interface> */
	if (strcmp(p, "on") == 0) {
		var_t *ifvar;
		element_t *el;

		PARSE_NEXT_TOKEN();
		if ((ifvar = npfctl_parsevalue(p)) == NULL)
			return PARSE_ERR();
		if (ifvar->v_type != VAR_SINGLE) {
			errx(EXIT_FAILURE, "invalid interface value '%s'", p);
		}
		el = ifvar->v_elements;
		iface = el->e_data;

		PARSE_NEXT_TOKEN();
	} else {
		iface = NULL;
	}

	/* inet, inet6 (TODO) */
	if (strcmp(p, "inet") == 0) {
		PARSE_NEXT_TOKEN();
	} else if (strcmp(p, "inet6") == 0) {
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
		from_cidr = npfctl_parsevalue(p);

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
		to_cidr = npfctl_parsevalue(p);

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
		PARSE_NEXT_TOKEN_NOCHECK();
		if (tfvar->v_type != VAR_SINGLE) {
			errx(EXIT_FAILURE, "invalid TCP flags");
		}
		element_t *el = tfvar->v_elements;
		tcp_flags = el->e_data;
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

	/* normalize ( .. ) */
	if (p && strcmp(p, "normalize") == 0) {
		p = strtok_r(NULL, "()", &sptr);
		if (p == NULL) {
			return PARSE_ERR();
		}
		if (npfctl_parsenorm(p, &rnd, &minttl, &maxmss)) {
			return PARSE_ERR();
		}
		attr |= NPF_RULE_NORMALIZE;
		PARSE_NEXT_TOKEN_NOCHECK();
	}

	/* Should have nothing more. */
	if (p != NULL) {
		return PARSE_ERR();
	}

	/* Set the rule attributes and interface, if any. */
	npfctl_rule_setattr(rl, attr, iface, rnd, minttl, maxmss);

	/*
	 * Generate all protocol data.
	 */
	npfctl_rule_protodata(rl, proto, tcp_flags, icmp_type, icmp_code,
	    from_cidr, fports, to_cidr, tports);
	return 0;
}

/*
 * npfctl_parsegroup: parse group definition.  Syntax:
 *
 *	group (name <name>, interface <if>, [ in | out ]) { <rules> }
 *	group (default) { <rules> }
 */

#define	GROUP_ATTRS	(NPF_RULE_PASS | NPF_RULE_FINAL)

static inline int
npfctl_parsegroup(char *buf, prop_dictionary_t rl)
{
	char *p = buf, *end, *sptr, *iface;
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
		attr_dir = NPF_RULE_IN | NPF_RULE_OUT;
		npfctl_rule_setattr(rl,
		    GROUP_ATTRS | NPF_RULE_DEFAULT | attr_dir, NULL,
		    false, 0, 0);
		return 0;
	}

	PARSE_FIRST_TOKEN();

	/* Name of the group (mandatory). */
	if (strcmp(p, "name") == 0) {
		PARSE_NEXT_TOKEN()
		if (*p != '"')
			return -1;
		if ((end = strchr(++p, '"')) == NULL)
			return -1;
		*end = '\0';
		/* TODO: p == name */
		PARSE_NEXT_TOKEN_NOCHECK();
	}

	/* Interface for this group (optional). */
	if (p && strcmp(p, "interface") == 0) {
		var_t *ifvar;
		element_t *el;

		PARSE_NEXT_TOKEN();
		if ((ifvar = npfctl_parsevalue(p)) == NULL)
			return -1;
		if (ifvar->v_type != VAR_SINGLE) {
			errx(EXIT_FAILURE, "invalid key '%s'", ifvar->v_key);
		}
		el = ifvar->v_elements;
		iface = el->e_data;
		PARSE_NEXT_TOKEN_NOCHECK();
	} else {
		iface = NULL;
	}

	/* Direction (optional). */
	if (p == NULL) {
		attr_dir = NPF_RULE_IN | NPF_RULE_OUT;
	} else {
		if (strcmp(p, "in") == 0)
			attr_dir = NPF_RULE_IN;
		else if (strcmp(p, "out") == 0)
			attr_dir = NPF_RULE_OUT;
		else
			return -1;
	}
	npfctl_rule_setattr(rl, GROUP_ATTRS | attr_dir, iface, false, 0, 0);
	return 0;
}

/*
 * npfctl_parsetable: parse table definition.
 *
 *	table <num> type <t> [ dynamic | file <path> ]
 */
static inline int
npfctl_parsetable(char *buf, prop_dictionary_t tl)
{
	char *p, *sptr;
	char *id_ptr, *type_ptr, *fname;

	DPRINTF(("table\t|%s|\n", buf));

	/* Name of the set. */
	if ((p = strchr(buf, '"')) == NULL) {
		return PARSE_ERR();
	}
	id_ptr = ++p;
	p = strchr(p, '"');
	*p++ = '\0';

	PARSE_FIRST_TOKEN();

	/* Table type (mandatory). */
	if (strcmp(p, "type") != 0) {
		return PARSE_ERR();
	}
	PARSE_NEXT_TOKEN_NOCHECK();
	if (p == NULL || *p != '"') {
		return PARSE_ERR();
	}
	type_ptr = p;
	if ((p = strchr(++p, '"')) == NULL) {
		return PARSE_ERR();
	}
	*p = '\0';

	/*
	 * Setup the table.
	 */
	npfctl_table_setup(tl, id_ptr, type_ptr);
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

	/* Construct the table. */
	npfctl_construct_table(tl, fname);
	return 0;
}

/*
 * npfctl_parse_nat: parse NAT policy definition.
 *
 *	[bi]nat <if> from <net> to <net/addr> -> <ip>
 *	rdr <if> from <net> to <addr> -> <ip>
 */
static inline int
npfctl_parse_nat(char *buf, prop_dictionary_t nat)
{
	var_t *ifvar, *from_cidr, *to_cidr, *ip;
	var_t *tports = NULL, *rports = NULL;
	element_t *iface, *cidr;
	char *p, *sptr;
	bool binat, rdr;

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
	if (ifvar->v_type != VAR_SINGLE) {
		errx(EXIT_FAILURE, "invalid interface value '%s'", p);
	} else {
		iface = ifvar->v_elements;
	}
	PARSE_NEXT_TOKEN();

	/* from <addr> */
	if (strcmp(p, "from") != 0) {
		return PARSE_ERR();
	}
	PARSE_NEXT_TOKEN();
	from_cidr = npfctl_parsevalue(p);
	PARSE_NEXT_TOKEN();

	/* to <addr> */
	if (strcmp(p, "to") != 0) {
		return PARSE_ERR();
	}
	PARSE_NEXT_TOKEN();
	to_cidr = npfctl_parsevalue(p);
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
	ip = npfctl_parsevalue(p);
	cidr = ip->v_elements;

	if (rdr) {
		PARSE_NEXT_TOKEN();
		if (strcmp(p, "port") != 0) {
			return PARSE_ERR();
		}
		PARSE_NEXT_TOKEN();
		rports = npfctl_parsevalue(p);
	}

	/*
	 * Setup NAT policy (rule as filter and extra info), which is
	 * Outbound NAT (NPF_NATOUT).  Unless it is a redirect rule,
	 * in which case it is Inbound NAT with specified port.
	 *
	 * XXX mess
	 */
	if (!rdr) {
		npfctl_rule_protodata(nat, NULL, NULL, -1, -1, from_cidr,
		    NULL, to_cidr, NULL);
		npfctl_nat_setup(nat, NPF_NATOUT,
		    binat ? 0 : (NPF_NAT_PORTS | NPF_NAT_PORTMAP),
		    iface->e_data, cidr->e_data, NULL);
	} else {
		element_t *rp = rports->v_elements;

		npfctl_rule_protodata(nat, NULL, NULL, -1, -1, from_cidr,
		    NULL, to_cidr, tports);
		npfctl_nat_setup(nat, NPF_NATIN, NPF_NAT_PORTS,
		    iface->e_data, cidr->e_data, rp->e_data);
	}

	/*
	 * For bi-directional NAT case, create and setup additional
	 * Inbound NAT (NPF_NATIN) policy.  Note that translation address
	 * is local IP, and filter criteria is inverted accordingly.
	 *
	 * XXX mess
	 */
	if (binat) {
		prop_dictionary_t bn = npfctl_mk_nat();
		element_t *taddr = from_cidr->v_elements;

		npfctl_rule_protodata(bn, NULL, NULL, -1, -1,
		    to_cidr, NULL, ip, NULL);
		npfctl_nat_setup(bn, NPF_NATIN, 0, iface->e_data,
		    taddr->e_data, NULL);
		npfctl_add_nat(bn);
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
static inline int
npfctl_parsevar(char *buf)
{
	char *s = buf, *p, *key;
	var_t *vr;

	DPRINTF(("def\t|%s|\n", buf));

	if ((p = strpbrk(s, "= \t")) == NULL)
		return -1;

	/* Validation of '='. */
	if (*p != '=' && strchr(p, '=') == NULL)
		return -1;
	*p = '\0';
	key = s;

	/* Check for duplicates. */
	if (npfctl_lookup_varlist(key))
		return -1;

	/* Parse quotes before. */
	if ((s = strchr(p + 1, '"')) == NULL)
		return -1;
	if ((p = strchr(++s, '"')) == NULL)
		return -1;
	*p = '\0';

	if ((vr = npfctl_parsevalue(s)) == NULL)
		return -1;
	vr->v_key = xstrdup(key);
	vr->v_next = var_list;
	var_list = vr;
	return 0;
}

/*
 * npf_parseline: main function parsing a single configuration line.
 *
 * => Distinguishes 'group', rule (in-group), 'table' and definitions.
 * => Tracks begin-end of the group i.e. in-group state.
 */
int
npf_parseline(char *buf)
{
	static prop_dictionary_t curgr = NULL;
	char *p = buf;
	int ret;

	/* Skip emptry lines and comments. */
	while (isspace((unsigned char)*p))
		p++;
	if (*p == '\0' || *p == '\n' || *p == '#')
		return 0;

	/* At first, check if inside the group. */
	if (curgr) {
		prop_dictionary_t rl;

		/* End of the group. */
		if (*p == '}') {
			curgr = NULL;
			return 0;
		}
		/* Rule. */
		rl = npfctl_mk_rule(false);
		ret = npfctl_parserule(p, rl);
		if (ret)
			return ret;
		npfctl_add_rule(rl, curgr);

	} else if (strncmp(p, "group", 5) == 0) {

		/* Group. */
		curgr = npfctl_mk_rule(true);
		ret = npfctl_parsegroup(p, curgr);
		if (ret)
			return ret;
		npfctl_add_rule(curgr, NULL);

	} else if (strncmp(p, "table", 5) == 0) {
		prop_dictionary_t tl;

		/* Table. */
		tl = npfctl_mk_table();
		ret = npfctl_parsetable(p, tl);
		if (ret)
			return ret;
		npfctl_add_table(tl);

	} else if (strncmp(p, "nat", 3) == 0 || strncmp(p, "rdr", 3) == 0 ||
	    strncmp(p, "binat", 5) == 0) {
		prop_dictionary_t nat;

		/* NAT policy. */
		nat = npfctl_mk_nat();
		ret = npfctl_parse_nat(p, nat);
		if (ret)
			return ret;
		npfctl_add_nat(nat);

	} else {
		/* Defined variable or syntax error. */
		ret = npfctl_parsevar(p);
	}
	return ret;
}
