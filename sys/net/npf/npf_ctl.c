/*	$NetBSD: npf_ctl.c,v 1.4 2010/12/18 01:07:25 rmind Exp $	*/

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
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: npf_ctl.c,v 1.4 2010/12/18 01:07:25 rmind Exp $");

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
	 * Note: in a case of error, caller will free the tableset.
	 */
	return error;
}

static int
npf_mk_singlerule(prop_dictionary_t rldict,
    npf_ruleset_t *rlset, npf_rule_t **parent)
{
	npf_rule_t *rl;
	prop_object_t obj;
	size_t nc_size;
	void *nc;

	/* Rule - dictionary. */
	if (prop_object_type(rldict) != PROP_TYPE_DICTIONARY)
		return EINVAL;

	/* N-code (binary data). */
	obj = prop_dictionary_get(rldict, "ncode");
	if (obj) {
		const void *ncptr;
		int npf_err, errat;

		/*
		 * Allocate, copy and validate n-code. XXX: Inefficient.
		 */
		ncptr = prop_data_data_nocopy(obj);
		nc_size = prop_data_size(obj);
		if (ncptr == NULL || nc_size > NPF_NCODE_LIMIT) {
			return EINVAL;
		}
		nc = npf_ncode_alloc(nc_size);
		if (nc == NULL) {
			return EINVAL;
		}
		memcpy(nc, ncptr, nc_size);
		npf_err = npf_ncode_validate(nc, nc_size, &errat);
		if (npf_err) {
			npf_ncode_free(nc, nc_size);
			/* TODO: return error details via proplib */
			return EINVAL;
		}
	} else {
		/* No n-code. */
		nc = NULL;
		nc_size = 0;
	}

	/* Allocate and setup NPF rule. */
	rl = npf_rule_alloc(rldict, nc, nc_size);
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

	error = 0;
	it = prop_array_iterator(rules);
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
	 * Note: in a case of error, caller will free the ruleset.
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

	error = 0;
	it = prop_array_iterator(natlist);
	while ((natdict = prop_object_iterator_next(it)) != NULL) {
		npf_natpolicy_t *np;
		npf_rule_t *rl;

		/* NAT policy - dictionary. */
		if (prop_object_type(natdict) != PROP_TYPE_DICTIONARY) {
			error = EINVAL;
			break;
		}

		/*
		 * NAT policies are standard rules, plus additional
		 * information for translation.  Make a rule.
		 */
		error = npf_mk_singlerule(natdict, nset, &rl);
		if (error)
			break;

		/* Allocate a new NAT policy and assign to the rule. */
		np = npf_nat_newpolicy(natdict);
		if (np == NULL) {
			npf_rule_free(rl);
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

	/*
	 * Finally - reload ruleset, tableset and NAT policies.
	 * Operation will be performed as a single transaction.
	 */
	npf_reload(rlset, tblset, nset);

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
 * npfctl_sessions_save: construct a list of sessions and export for saving.
 */
int
npfctl_sessions_save(u_long cmd, void *data)
{
	struct plistref *pref = data;
	prop_dictionary_t sesdict;
	prop_array_t selist, nplist;
	int error;

	/* Create a dictionary and two lists. */
	sesdict = prop_dictionary_create();
	selist = prop_array_create();
	nplist = prop_array_create();

	/* Save the sessions. */
	error = npf_session_save(selist, nplist);
	if (error) {
		goto fail;
	}

	/* Set the session list, NAT policy list and export the dictionary. */
	prop_dictionary_set(sesdict, "session-list", selist);
	prop_dictionary_set(sesdict, "nat-policy-list", nplist);
#ifdef _KERNEL
	error = prop_dictionary_copyout_ioctl(pref, cmd, sesdict);
#else
	error = prop_dictionary_externalize_to_file(sesdict, data) ? 0 : errno;
#endif
fail:
	prop_object_release(sesdict);
	return error;
}

/*
 * npfctl_sessions_load: import a list of sessions, reconstruct them and load.
 */
int
npfctl_sessions_load(u_long cmd, void *data)
{
	const struct plistref *pref = data;
	npf_sehash_t *sehasht = NULL;
	prop_dictionary_t sesdict, sedict;
	prop_object_iterator_t it;
	prop_array_t selist;
	int error;

	/* Retrieve the dictionary containing session and NAT policy lists. */
#ifdef _KERNEL
	error = prop_dictionary_copyin_ioctl(pref, cmd, &sesdict);
	if (error)
		return error;
#else
	sesdict = prop_dictionary_internalize_from_file(data);
	if (sesdict == NULL)
		return EINVAL;
#endif
	/*
	 * Note: session objects contain the references to the NAT policy
	 * entries.  Therefore, no need to directly access it.
	 */
	selist = prop_dictionary_get(sesdict, "session-list");
	if (prop_object_type(selist) != PROP_TYPE_ARRAY) {
		error = EINVAL;
		goto fail;
	}

	/* Create a session hash table. */
	sehasht = sess_htable_create();
	if (sehasht == NULL) {
		error = ENOMEM;
		goto fail;
	}

	/*
	 * Iterate through and construct each session.
	 */
	error = 0;
	it = prop_array_iterator(selist);
	npf_core_enter();
	while ((sedict = prop_object_iterator_next(it)) != NULL) {
		/* Session - dictionary. */
		if (prop_object_type(sedict) != PROP_TYPE_DICTIONARY) {
			error = EINVAL;
			goto fail;
		}
		/* Construct and insert real session structure. */
		error = npf_session_restore(sehasht, sedict);
		if (error) {
			goto fail;
		}
	}
	npf_core_exit();
	sess_htable_reload(sehasht);
fail:
	prop_object_release(selist);
	if (error && sehasht) {
		/* Destroy session table. */
		sess_htable_destroy(sehasht);
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
