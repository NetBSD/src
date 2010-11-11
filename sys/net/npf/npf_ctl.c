/*	$NetBSD: npf_ctl.c,v 1.3 2010/11/11 06:30:39 rmind Exp $	*/

/*-
 * Copyright (c) 2009-2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This material is based upon work partially supported by The
 * NetBSD Foundation under a contract with Mindaugas Rasiukevicius.
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
 * NPF device control.
 *
 * Implementation of (re)loading, construction of tables and rules.
 * NPF proplib(9) dictionary consumer.
 *
 * TODO:
 * - Consider implementing 'sync' functionality.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: npf_ctl.c,v 1.3 2010/11/11 06:30:39 rmind Exp $");

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/kernel.h>

#include <prop/proplib.h>

#include "npf_ncode.h"
#include "npf_impl.h"

/*
 * npfctl_switch: enable or disable packet inspection.
 */
int
npfctl_switch(void *data)
{
	const bool onoff = *(int *)data ? true : false;
	int error;

	if (onoff) {
		/* Enable: add pfil hooks. */
		error = npf_register_pfil();
	} else {
		/* Disable: remove pfil hooks. */
		npf_unregister_pfil();
		error = 0;
	}
	return error;
}

static int
npf_mk_tables(npf_tableset_t *tblset, prop_array_t tables)
{
	prop_object_iterator_t it;
	prop_dictionary_t tbldict;
	prop_object_t obj;
	int error = 0;

	/* Tables - array. */
	if (prop_object_type(tables) != PROP_TYPE_ARRAY)
		return EINVAL;

	it = prop_array_iterator(tables);
	if (it == NULL)
		return ENOMEM;

	while ((tbldict = prop_object_iterator_next(it)) != NULL) {
		prop_dictionary_t ent;
		prop_object_iterator_t eit;
		prop_array_t entries;
		npf_table_t *t;
		u_int tid;
		int type;

		/* Table - dictionary. */
		if (prop_object_type(tbldict) != PROP_TYPE_DICTIONARY) {
			error = EINVAL;
			break;
		}

		/* Table ID and type. */
		obj = prop_dictionary_get(tbldict, "id");
		tid = (u_int)prop_number_integer_value(obj);
		obj = prop_dictionary_get(tbldict, "type");
		type = (int)prop_number_integer_value(obj);
		/* Validate them. */
		error = npf_table_check(tblset, tid, type);
		if (error)
			break;

		/* Create and insert the table. */
		t = npf_table_create(tid, type, 1024);	/* XXX */
		if (t == NULL) {
			error = ENOMEM;
			break;
		}
		error = npf_tableset_insert(tblset, t);
		KASSERT(error == 0);

		/* Entries. */
		entries = prop_dictionary_get(tbldict, "entries");
		if (prop_object_type(entries) != PROP_TYPE_ARRAY) {
			error = EINVAL;
			break;
		}
		eit = prop_array_iterator(entries);
		if (eit == NULL) {
			error = ENOMEM;
			break;
		}
		while ((ent = prop_object_iterator_next(eit)) != NULL) {
			in_addr_t addr, mask;	/* XXX: IPv6 */

			/* Address. */
			obj = prop_dictionary_get(ent, "addr");
			addr = (in_addr_t)prop_number_integer_value(obj);
			/* Mask. */
			obj = prop_dictionary_get(ent, "mask");
			mask = (in_addr_t)prop_number_integer_value(obj);
			/* Add a table entry. */
			error = npf_table_add_v4cidr(tblset, tid, addr, mask);
			if (error)
				break;
		}
		prop_object_iterator_release(eit);
		if (error)
			break;
	}
	prop_object_iterator_release(it);
	/*
	 * Note: in a case of error, caller will free entire tableset.
	 */
	return error;
}

static void *
npf_mk_ncode(const void *ncptr, size_t nc_size)
{
	int npf_err, errat;
	void *nc;

	/*
	 * Allocate and copy n-code.
	 *
	 * XXX: Inefficient; consider extending proplib(9) to provide
	 * interface for custom allocator and avoid copy.
	 */
	nc = npf_ncode_alloc(nc_size);
	if (nc == NULL) {
		return NULL;
	}
	memcpy(nc, ncptr, nc_size);
	npf_err = npf_ncode_validate(nc, nc_size, &errat);
	if (npf_err) {
		npf_ncode_free(nc, nc_size);
		/* TODO: return error details via proplib */
		return NULL;
	}
	return nc;
}

static int
npf_mk_singlerule(prop_dictionary_t rldict,
    npf_ruleset_t *rlset, npf_rule_t **parent)
{
	npf_rule_t *rl;
	prop_object_t obj;
	int attr, ifidx, minttl, maxmss;
	pri_t pri;
	bool rnd_ipid;
	size_t nc_size;
	void *nc;

	/* Rule - dictionary. */
	if (prop_object_type(rldict) != PROP_TYPE_DICTIONARY)
		return EINVAL;

	/* Attributes (integer). */
	obj = prop_dictionary_get(rldict, "attributes");
	attr = prop_number_integer_value(obj);

	/* Priority (integer). */
	obj = prop_dictionary_get(rldict, "priority");
	pri = prop_number_integer_value(obj);

	/* Interface ID (integer). */
	obj = prop_dictionary_get(rldict, "interface");
	ifidx = prop_number_integer_value(obj);

	/* Randomize IP ID (bool). */
	obj = prop_dictionary_get(rldict, "randomize-id");
	rnd_ipid = prop_bool_true(obj);

	/* Minimum IP TTL (integer). */
	obj = prop_dictionary_get(rldict, "min-ttl");
	minttl = prop_number_integer_value(obj);

	/* Maximum TCP MSS (integer). */
	obj = prop_dictionary_get(rldict, "max-mss");
	maxmss = prop_number_integer_value(obj);

	/* N-code (binary data). */
	obj = prop_dictionary_get(rldict, "ncode");
	if (obj) {
		const void *ncptr;

		/* Perform n-code validation. */
		nc_size = prop_data_size(obj);
		ncptr = prop_data_data_nocopy(obj);
		if (ncptr == NULL || nc_size > NPF_NCODE_LIMIT) {
			return EINVAL;
		}
		nc = npf_mk_ncode(ncptr, nc_size);
		if (nc == NULL) {
			return EINVAL;
		}
	} else {
		/* No n-code. */
		nc = NULL;
		nc_size = 0;
	}

	/* Allocate and setup NPF rule. */
	rl = npf_rule_alloc(attr, pri, ifidx, nc, nc_size,
	    rnd_ipid, minttl, maxmss);
	if (rl == NULL) {
		if (nc) {
			npf_ncode_free(nc, nc_size);	/* XXX */
		}
		return ENOMEM;
	}
	npf_ruleset_insert(rlset, rl);
	if (parent) {
		*parent = rl;
	}
	return 0;
}

static int
npf_mk_rules(npf_ruleset_t *rlset, prop_array_t rules)
{
	prop_object_iterator_t it;
	prop_dictionary_t rldict;
	int error;

	/* Ruleset - array. */
	if (prop_object_type(rules) != PROP_TYPE_ARRAY)
		return EINVAL;

	it = prop_array_iterator(rules);
	if (it == NULL)
		return ENOMEM;

	error = 0;
	while ((rldict = prop_object_iterator_next(it)) != NULL) {
		prop_object_iterator_t sit;
		prop_array_t subrules;
		prop_dictionary_t srldict;
		npf_rule_t *myrl;

		/* Generate a single rule. */
		error = npf_mk_singlerule(rldict, rlset, &myrl);
		if (error)
			break;

		/* Check for subrules. */
		subrules = prop_dictionary_get(rldict, "subrules");
		if (subrules == NULL) {
			/* No subrules, next.. */
			continue;
		}
		/* Generate subrules, if any. */
		if (prop_object_type(subrules) != PROP_TYPE_ARRAY) {
			error = EINVAL;
			break;
		}
		sit = prop_array_iterator(subrules);
		if (sit == NULL) {
			error = ENOMEM;
			break;
		}
		while ((srldict = prop_object_iterator_next(sit)) != NULL) {
			/* For subrule, pass ruleset pointer of parent. */
			error = npf_mk_singlerule(srldict,
			    npf_rule_subset(myrl), NULL);
			if (error)
				break;
		}
		prop_object_iterator_release(sit);
		if (error)
			break;
	}
	prop_object_iterator_release(it);
	/*
	 * Note: in a case of error, caller will free entire ruleset.
	 */
	return error;
}

static int
npf_mk_natlist(npf_ruleset_t *nset, prop_array_t natlist)
{
	prop_object_iterator_t it;
	prop_dictionary_t natdict;
	int error;

	/* NAT policies - array. */
	if (prop_object_type(natlist) != PROP_TYPE_ARRAY)
		return EINVAL;

	it = prop_array_iterator(natlist);
	if (it == NULL)
		return ENOMEM;

	error = 0;
	while ((natdict = prop_object_iterator_next(it)) != NULL) {
		prop_object_t obj;
		npf_natpolicy_t *np;
		npf_rule_t *rl;
		const npf_addr_t *taddr;
		size_t taddr_sz;
		in_port_t tport;
		int type, flags;

		/* NAT policy - dictionary. */
		if (prop_object_type(natdict) != PROP_TYPE_DICTIONARY) {
			error = EINVAL;
			break;
		}

		/* Translation type. */
		obj = prop_dictionary_get(natdict, "type");
		type = prop_number_integer_value(obj);

		/* Translation type. */
		obj = prop_dictionary_get(natdict, "flags");
		flags = prop_number_integer_value(obj);

		/* Translation IP. */
		obj = prop_dictionary_get(natdict, "translation-ip");
		taddr_sz = prop_data_size(obj);
		taddr = (const npf_addr_t *)prop_data_data_nocopy(obj);

		/* Translation port (for redirect case). */
		obj = prop_dictionary_get(natdict, "translation-port");
		tport = (in_port_t)prop_number_integer_value(obj);

		/*
		 * NAT policies are standard rules, plus additional
		 * information for translation.  Make a rule.
		 */
		error = npf_mk_singlerule(natdict, nset, &rl);
		if (error)
			break;

		/* Allocate a new NAT policy and assign to the rule. */
		np = npf_nat_newpolicy(type, flags, taddr, taddr_sz, tport);
		if (np == NULL) {
			error = ENOMEM;
			break;
		}
		npf_rule_setnat(rl, np);
	}
	prop_object_iterator_release(it);
	/*
	 * Note: in a case of error, caller will free entire NAT ruleset
	 * with assigned NAT policies.
	 */
	return error;
}

/*
 * npfctl_reload: store passed data i.e. update settings, create passed
 * tables, rules and atomically activate all them.
 */
int
npfctl_reload(u_long cmd, void *data)
{
	const struct plistref *pref = data;
	npf_tableset_t *tblset = NULL;
	npf_ruleset_t *rlset = NULL;
	npf_ruleset_t *nset = NULL;
	prop_dictionary_t dict;
	prop_array_t natlist, tables, rules;
	prop_object_t ver;
	int error;

	/* Retrieve the dictionary. */
#ifdef _KERNEL
	error = prop_dictionary_copyin_ioctl(pref, cmd, &dict);
	if (error)
		return error;
#else
	dict = prop_dictionary_internalize_from_file(data);
	if (dict == NULL)
		return EINVAL;
#endif
	/* Version. */
	ver = prop_dictionary_get(dict, "version");
	if (ver == NULL || prop_number_integer_value(ver) != NPF_VERSION) {
		error = EINVAL;
		goto fail;
	}

	/* XXX: Hard way for now. */
	(void)npf_session_tracking(false);

	/* NAT policies. */
	nset = npf_ruleset_create();
	natlist = prop_dictionary_get(dict, "translation");
	error = npf_mk_natlist(nset, natlist);
	if (error)
		goto fail;

	/* Tables. */
	tblset = npf_tableset_create();
	tables = prop_dictionary_get(dict, "tables");
	error = npf_mk_tables(tblset, tables);
	if (error)
		goto fail;

	/* Rules. */
	rlset = npf_ruleset_create();
	rules = prop_dictionary_get(dict, "rules");
	error = npf_mk_rules(rlset, rules);
	if (error)
		goto fail;

	/* Flush and reload NAT policies. */
	npf_nat_reload(nset);

	/*
	 * Finally, reload the ruleset.  It will also reload the tableset.
	 * Operation will be performed as a single transaction.
	 */
	npf_ruleset_reload(rlset, tblset);

	(void)npf_session_tracking(true);

	/* Done.  Since data is consumed now, we shall not destroy it. */
	tblset = NULL;
	rlset = NULL;
	nset = NULL;
fail:
	prop_object_release(dict);
	/*
	 * Note: destroy rulesets first, to drop references to the tableset.
	 */
	KASSERT(error == 0 || (nset || rlset || tblset));
	if (nset) {
		npf_ruleset_destroy(nset);
	}
	if (rlset) {
		npf_ruleset_destroy(rlset);
	}
	if (tblset) {
		npf_tableset_destroy(tblset);
	}
	return error;
}

/*
 * npfctl_table: add, remove or query entries in the specified table.
 *
 * For maximum performance, interface is avoiding proplib(3)'s overhead.
 */
int
npfctl_table(void *data)
{
	npf_ioctl_table_t *nct = data;
	int error;

	switch (nct->nct_action) {
	case NPF_IOCTL_TBLENT_ADD:
		error = npf_table_add_v4cidr(NULL, nct->nct_tid,
		    nct->nct_addr, nct->nct_mask);
		break;
	case NPF_IOCTL_TBLENT_REM:
		error = npf_table_rem_v4cidr(NULL, nct->nct_tid,
		    nct->nct_addr, nct->nct_mask);
		break;
	default:
		/* XXX */
		error = npf_table_match_v4addr(nct->nct_tid, nct->nct_addr);
		if (error) {
			error = EINVAL;
		}
	}
	return error;
}
