/*	$NetBSD: npf_ctl.c,v 1.8 2011/11/04 02:57:28 jakllsch Exp $	*/

/*-
 * Copyright (c) 2009-2011 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: npf_ctl.c,v 1.8 2011/11/04 02:57:28 jakllsch Exp $");

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

static int __noinline
npf_mk_tables(npf_tableset_t *tblset, prop_array_t tables)
{
	prop_object_iterator_t it;
	prop_dictionary_t tbldict;
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
		prop_dictionary_get_uint32(tbldict, "id", &tid);
		prop_dictionary_get_int32(tbldict, "type", &type);

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
			const npf_addr_t *addr;
			uint8_t mask; /* XXX should be npf_netmask_t */

			/* Get address and mask.  Add a table entry. */
			addr = (const npf_addr_t *)prop_data_data_nocopy(prop_dictionary_get(ent, "addr"));
			prop_dictionary_get_uint8(ent, "mask", &mask);
			error = npf_table_add_cidr(tblset, tid, addr, mask);
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

static npf_rproc_t *
npf_mk_rproc(prop_array_t rprocs, const char *rpname)
{
	prop_object_iterator_t it;
	prop_dictionary_t rpdict;
	npf_rproc_t *rp;

	it = prop_array_iterator(rprocs);
	while ((rpdict = prop_object_iterator_next(it)) != NULL) {
		const char *iname;
		prop_dictionary_get_cstring_nocopy(rpdict, "name", &iname);
		if (strcmp(rpname, iname) == 0)
			break;
	}
	prop_object_iterator_release(it);
	if (rpdict == NULL) {
		return NULL;
	}
	CTASSERT(sizeof(uintptr_t) <= sizeof(uint64_t));
	if (!prop_dictionary_get_uint64(rpdict, "rproc-ptr", (uint64_t *)&rp)) {
		rp = npf_rproc_create(rpdict);
		prop_dictionary_set_uint64(rpdict, "rproc-ptr",
		    (uint64_t)(uintptr_t)rp);
	}
	return rp;
}

static int __noinline
npf_mk_singlerule(prop_dictionary_t rldict, prop_array_t rps, npf_rule_t **rl)
{
	const char *rnm;
	npf_rproc_t *rp;
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

	/* Check for rule procedure. */
	if (rps && prop_dictionary_get_cstring_nocopy(rldict, "rproc", &rnm)) {
		rp = npf_mk_rproc(rps, rnm);
		if (rp == NULL) {
			if (nc) {
				npf_ncode_free(nc, nc_size);	/* XXX */
			}
			return EINVAL;
		}
	} else {
		rp = NULL;
	}

	/* Finally, allocate and return the rule. */
	*rl = npf_rule_alloc(rldict, rp, nc, nc_size);
	return 0;
}

static int __noinline
npf_mk_subrules(npf_ruleset_t *rlset, prop_array_t rules, prop_array_t rprocs)
{
	prop_object_iterator_t it;
	prop_dictionary_t rldict;
	int error = 0;

	if (prop_object_type(rules) != PROP_TYPE_ARRAY) {
		return EINVAL;
	}
	it = prop_array_iterator(rules);
	while ((rldict = prop_object_iterator_next(it)) != NULL) {
		npf_rule_t *rl;
		error = npf_mk_singlerule(rldict, rprocs, &rl);
		if (error) {
			break;
		}
		npf_ruleset_insert(rlset, rl);
	}
	prop_object_iterator_release(it);
	return error;
}

static int __noinline
npf_mk_rules(npf_ruleset_t *rlset, prop_array_t rules, prop_array_t rprocs)
{
	prop_object_iterator_t it;
	prop_dictionary_t rldict, rpdict;
	int error;

	/* Rule procedures and the ruleset - arrays. */
	if (prop_object_type(rprocs) != PROP_TYPE_ARRAY ||
	    prop_object_type(rules) != PROP_TYPE_ARRAY)
		return EINVAL;

	it = prop_array_iterator(rprocs);
	while ((rpdict = prop_object_iterator_next(it)) != NULL) {
		if (prop_dictionary_get(rpdict, "rproc-ptr")) {
			prop_object_iterator_release(it);
			return EINVAL;
		}
	}
	prop_object_iterator_release(it);

	error = 0;
	it = prop_array_iterator(rules);
	while ((rldict = prop_object_iterator_next(it)) != NULL) {
		prop_array_t subrules;
		npf_ruleset_t *rlsetsub;
		npf_rule_t *rl;

		/* Generate a single rule. */
		error = npf_mk_singlerule(rldict, rprocs, &rl);
		if (error) {
			break;
		}
		npf_ruleset_insert(rlset, rl);

		/* Check for sub-rules and generate, if any. */
		subrules = prop_dictionary_get(rldict, "subrules");
		if (subrules == NULL) {
			/* No subrules, next.. */
			continue;
		}
		rlsetsub = npf_rule_subset(rl);
		error = npf_mk_subrules(rlsetsub, subrules, rprocs);
		if (error)
			break;
	}
	prop_object_iterator_release(it);
	/*
	 * Note: in a case of error, caller will free the ruleset.
	 */
	return error;
}

static int __noinline
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
		error = npf_mk_singlerule(natdict, NULL, &rl);
		if (error) {
			break;
		}
		npf_ruleset_insert(nset, rl);

		/* If rule is named, it is a group with NAT policies. */
		if (prop_dictionary_get(natdict, "name") &&
		    prop_dictionary_get(natdict, "subrules")) {
			continue;
		}

		/* Allocate a new NAT policy and assign to the rule. */
		np = npf_nat_newpolicy(natdict, nset);
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
	prop_array_t natlist, tables, rprocs, rules;
	npf_tableset_t *tblset = NULL;
	npf_ruleset_t *rlset = NULL;
	npf_ruleset_t *nset = NULL;
	prop_dictionary_t dict;
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

	/* Rules and rule procedures. */
	rlset = npf_ruleset_create();
	rprocs = prop_dictionary_get(dict, "rprocs");
	rules = prop_dictionary_get(dict, "rules");
	error = npf_mk_rules(rlset, rules, rprocs);
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
	prop_object_release(dict);
	return error;
}

/*
 * npfctl_update_rule: reload a specific rule identified by the name.
 */
int
npfctl_update_rule(u_long cmd, void *data)
{
	const struct plistref *pref = data;
	prop_dictionary_t dict;
	prop_array_t subrules;
	prop_object_t obj;
	npf_ruleset_t *rlset;
	const char *name;
	int error;

#ifdef _KERNEL
	/* Retrieve and construct the rule. */
	error = prop_dictionary_copyin_ioctl(pref, cmd, &dict);
	if (error) {
		return error;
	}
#else
	dict = prop_dictionary_internalize_from_file(data);
	if (dict == NULL)
		return EINVAL;
#endif
	/* Create the ruleset and construct sub-rules. */
	rlset = npf_ruleset_create();
	subrules = prop_dictionary_get(dict, "subrules");
	error = npf_mk_subrules(rlset, subrules, NULL);
	if (error) {
		goto out;
	}

	/* Lookup the rule by name, and replace its subset (sub-rules). */
	obj = prop_dictionary_get(dict, "name");
	name = prop_string_cstring_nocopy(obj);
	if (npf_ruleset_replace(name, rlset) == NULL) {
		/* Not found. */
		error = ENOENT;
out:		/* Error path. */
		npf_ruleset_destroy(rlset);
	}
	prop_object_release(dict);
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

	npf_core_enter(); /* XXXSMP */
	switch (nct->nct_action) {
	case NPF_IOCTL_TBLENT_ADD:
		error = npf_table_add_cidr(NULL, nct->nct_tid,
		    &nct->nct_addr, nct->nct_mask);
		break;
	case NPF_IOCTL_TBLENT_REM:
		error = npf_table_rem_cidr(NULL, nct->nct_tid,
		    &nct->nct_addr, nct->nct_mask);
		break;
	default:
		/* XXX */
		error = npf_table_match_addr(nct->nct_tid, &nct->nct_addr);
		if (error) {
			error = EINVAL;
		}
	}
	npf_core_exit(); /* XXXSMP */
	return error;
}
