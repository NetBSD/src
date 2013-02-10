/*	$NetBSD: npf_ctl.c,v 1.22 2013/02/10 23:47:37 rmind Exp $	*/

/*-
 * Copyright (c) 2009-2013 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: npf_ctl.c,v 1.22 2013/02/10 23:47:37 rmind Exp $");

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/kmem.h>
#include <net/bpf.h>

#include <prop/proplib.h>

#include "npf_ncode.h"
#include "npf_impl.h"

#if defined(DEBUG) || defined(DIAGNOSTIC)
#define	NPF_ERR_DEBUG(e) \
	prop_dictionary_set_cstring_nocopy((e), "source-file", __FILE__); \
	prop_dictionary_set_uint32((e), "source-line", __LINE__);
#else
#define	NPF_ERR_DEBUG(e)
#endif

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
		error = npf_pfil_register();
	} else {
		/* Disable: remove pfil hooks. */
		npf_pfil_unregister();
		error = 0;
	}
	return error;
}

static int __noinline
npf_mk_tables(npf_tableset_t *tblset, prop_array_t tables,
    prop_dictionary_t errdict)
{
	prop_object_iterator_t it;
	prop_dictionary_t tbldict;
	int error = 0;

	/* Tables - array. */
	if (prop_object_type(tables) != PROP_TYPE_ARRAY) {
		NPF_ERR_DEBUG(errdict);
		return EINVAL;
	}

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
			NPF_ERR_DEBUG(errdict);
			error = EINVAL;
			break;
		}

		/* Table ID and type. */
		prop_dictionary_get_uint32(tbldict, "id", &tid);
		prop_dictionary_get_int32(tbldict, "type", &type);

		/* Validate them, check for duplicate IDs. */
		error = npf_table_check(tblset, tid, type);
		if (error)
			break;

		/* Create and insert the table. */
		t = npf_table_create(tid, type, 1024);	/* XXX */
		if (t == NULL) {
			NPF_ERR_DEBUG(errdict);
			error = ENOMEM;
			break;
		}
		error = npf_tableset_insert(tblset, t);
		KASSERT(error == 0);

		/* Entries. */
		entries = prop_dictionary_get(tbldict, "entries");
		if (prop_object_type(entries) != PROP_TYPE_ARRAY) {
			NPF_ERR_DEBUG(errdict);
			error = EINVAL;
			break;
		}
		eit = prop_array_iterator(entries);
		while ((ent = prop_object_iterator_next(eit)) != NULL) {
			const npf_addr_t *addr;
			npf_netmask_t mask;
			int alen;

			/* Get address and mask.  Add a table entry. */
			prop_object_t obj = prop_dictionary_get(ent, "addr");
			addr = (const npf_addr_t *)prop_data_data_nocopy(obj);
			prop_dictionary_get_uint8(ent, "mask", &mask);
			alen = prop_data_size(obj);

			error = npf_table_insert(tblset, tid, alen, addr, mask);
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
npf_mk_singlerproc(prop_dictionary_t rpdict)
{
	prop_object_iterator_t it;
	prop_dictionary_t extdict;
	prop_array_t extlist;
	npf_rproc_t *rp;

	extlist = prop_dictionary_get(rpdict, "extcalls");
	if (prop_object_type(extlist) != PROP_TYPE_ARRAY) {
		return NULL;
	}

	rp = npf_rproc_create(rpdict);
	if (rp == NULL) {
		return NULL;
	}

	it = prop_array_iterator(extlist);
	while ((extdict = prop_object_iterator_next(it)) != NULL) {
		const char *name;

		if (!prop_dictionary_get_cstring_nocopy(extdict,
		    "name", &name) || npf_ext_construct(name, rp, extdict)) {
			npf_rproc_release(rp);
			rp = NULL;
			break;
		}
	}
	prop_object_iterator_release(it);
	return rp;
}

static int __noinline
npf_mk_rprocs(npf_rprocset_t *rpset, prop_array_t rprocs,
    prop_dictionary_t errdict)
{
	prop_object_iterator_t it;
	prop_dictionary_t rpdict;
	int error = 0;

	it = prop_array_iterator(rprocs);
	while ((rpdict = prop_object_iterator_next(it)) != NULL) {
		npf_rproc_t *rp;

		if ((rp = npf_mk_singlerproc(rpdict)) == NULL) {
			NPF_ERR_DEBUG(errdict);
			error = EINVAL;
			break;
		}
		npf_rprocset_insert(rpset, rp);
	}
	prop_object_iterator_release(it);
	return error;
}

static int __noinline
npf_mk_code(prop_object_t obj, int type, void **code, size_t *csize,
    prop_dictionary_t errdict)
{
	const void *cptr;
	int cerr, errat;
	size_t clen;
	void *bc;

	cptr = prop_data_data_nocopy(obj);
	if (cptr == NULL || (clen = prop_data_size(obj)) == 0) {
		NPF_ERR_DEBUG(errdict);
		return EINVAL;
	}

	switch (type) {
	case NPF_CODE_NC:
		if (clen > NPF_NCODE_LIMIT) {
			NPF_ERR_DEBUG(errdict);
			return ERANGE;
		}
		if ((cerr = npf_ncode_validate(cptr, clen, &errat)) != 0) {
			prop_dictionary_set_int32(errdict, "code-error", cerr);
			prop_dictionary_set_int32(errdict, "code-errat", errat);
			return EINVAL;
		}
		break;
	case NPF_CODE_BPF:
		if (!bpf_validate(cptr, clen)) {
			return EINVAL;
		}
		break;
	default:
		return ENOTSUP;
	}

	bc = kmem_alloc(clen, KM_SLEEP);
	memcpy(bc, cptr, clen);

	*code = bc;
	*csize = clen;
	return 0;
}

static int __noinline
npf_mk_singlerule(prop_dictionary_t rldict, npf_rprocset_t *rpset,
    npf_rule_t **rlret, prop_dictionary_t errdict)
{
	npf_rule_t *rl;
	const char *rname;
	prop_object_t obj;
	int p, error = 0;

	/* Rule - dictionary. */
	if (prop_object_type(rldict) != PROP_TYPE_DICTIONARY) {
		NPF_ERR_DEBUG(errdict);
		return EINVAL;
	}
	if ((rl = npf_rule_alloc(rldict)) == NULL) {
		NPF_ERR_DEBUG(errdict);
		return EINVAL;
	}

	/* Assign rule procedure, if any. */
	if (prop_dictionary_get_cstring_nocopy(rldict, "rproc", &rname)) {
		npf_rproc_t *rp;

		if (rpset == NULL) {
			error = EINVAL;
			goto err;
		}
		if ((rp = npf_rprocset_lookup(rpset, rname)) == NULL) {
			NPF_ERR_DEBUG(errdict);
			error = EINVAL;
			goto err;
		}
		npf_rule_setrproc(rl, rp);
	}

	/* Filter code (binary data). */
	if ((obj = prop_dictionary_get(rldict, "code")) != NULL) {
		int type;
		size_t len;
		void *code;

		prop_dictionary_get_int32(rldict, "code-type", &type);
		error = npf_mk_code(obj, type, &code, &len, errdict);
		if (error) {
			goto err;
		}
		npf_rule_setcode(rl, type, code, len);
	}

	*rlret = rl;
	return 0;
err:
	npf_rule_free(rl);
	prop_dictionary_get_int32(rldict, "priority", &p); /* XXX */
	prop_dictionary_set_int32(errdict, "id", p);
	return error;
}

static int __noinline
npf_mk_rules(npf_ruleset_t *rlset, prop_array_t rules, npf_rprocset_t *rpset,
    prop_dictionary_t errdict)
{
	prop_object_iterator_t it;
	prop_dictionary_t rldict;
	int error;

	if (prop_object_type(rules) != PROP_TYPE_ARRAY) {
		NPF_ERR_DEBUG(errdict);
		return EINVAL;
	}

	error = 0;
	it = prop_array_iterator(rules);
	while ((rldict = prop_object_iterator_next(it)) != NULL) {
		npf_rule_t *rl = NULL;

		/* Generate a single rule. */
		error = npf_mk_singlerule(rldict, rpset, &rl, errdict);
		if (error) {
			break;
		}
		npf_ruleset_insert(rlset, rl);
	}
	prop_object_iterator_release(it);
	/*
	 * Note: in a case of error, caller will free the ruleset.
	 */
	return error;
}

static int __noinline
npf_mk_natlist(npf_ruleset_t *nset, prop_array_t natlist,
    prop_dictionary_t errdict)
{
	prop_object_iterator_t it;
	prop_dictionary_t natdict;
	int error;

	/* NAT policies - array. */
	if (prop_object_type(natlist) != PROP_TYPE_ARRAY) {
		NPF_ERR_DEBUG(errdict);
		return EINVAL;
	}

	error = 0;
	it = prop_array_iterator(natlist);
	while ((natdict = prop_object_iterator_next(it)) != NULL) {
		npf_rule_t *rl = NULL;
		npf_natpolicy_t *np;

		/* NAT policy - dictionary. */
		if (prop_object_type(natdict) != PROP_TYPE_DICTIONARY) {
			NPF_ERR_DEBUG(errdict);
			error = EINVAL;
			break;
		}

		/*
		 * NAT policies are standard rules, plus additional
		 * information for translation.  Make a rule.
		 */
		error = npf_mk_singlerule(natdict, NULL, &rl, errdict);
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
			NPF_ERR_DEBUG(errdict);
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
	struct plistref *pref = data;
	prop_dictionary_t npf_dict, errdict;
	prop_array_t natlist, tables, rprocs, rules;
	npf_tableset_t *tblset = NULL;
	npf_rprocset_t *rpset = NULL;
	npf_ruleset_t *rlset = NULL;
	npf_ruleset_t *nset = NULL;
	uint32_t ver = 0;
	size_t nitems;
	bool flush;
	int error;

	/* Retrieve the dictionary. */
#ifndef _NPF_TESTING
	error = prop_dictionary_copyin_ioctl(pref, cmd, &npf_dict);
	if (error)
		return error;
#else
	npf_dict = (prop_dictionary_t)pref;
#endif

	/* Dictionary for error reporting and version check. */
	errdict = prop_dictionary_create();
	prop_dictionary_get_uint32(npf_dict, "version", &ver);
	if (ver != NPF_VERSION) {
		error = EPROGMISMATCH;
		goto fail;
	}

	/* NAT policies. */
	natlist = prop_dictionary_get(npf_dict, "translation");
	nitems = prop_array_count(natlist);

	nset = npf_ruleset_create(nitems);
	error = npf_mk_natlist(nset, natlist, errdict);
	if (error) {
		goto fail;
	}

	/* Tables. */
	tblset = npf_tableset_create();
	tables = prop_dictionary_get(npf_dict, "tables");
	error = npf_mk_tables(tblset, tables, errdict);
	if (error) {
		goto fail;
	}

	/* Rule procedures. */
	rpset = npf_rprocset_create();
	rprocs = prop_dictionary_get(npf_dict, "rprocs");
	error = npf_mk_rprocs(rpset, rprocs, errdict);
	if (error) {
		goto fail;
	}

	/* Rules. */
	rules = prop_dictionary_get(npf_dict, "rules");
	nitems = prop_array_count(rules);

	rlset = npf_ruleset_create(nitems);
	error = npf_mk_rules(rlset, rules, rpset, errdict);
	if (error) {
		goto fail;
	}

	flush = false;
	prop_dictionary_get_bool(npf_dict, "flush", &flush);

	/*
	 * Finally - perform the reload.
	 */
	npf_config_reload(npf_dict, rlset, tblset, nset, rpset, flush);

	/* Turn on/off session tracking accordingly. */
	npf_session_tracking(!flush);

	/* Done.  Since data is consumed now, we shall not destroy it. */
	tblset = NULL;
	rpset = NULL;
	rlset = NULL;
	nset = NULL;
fail:
	/*
	 * Note: destroy rulesets first, to drop references to the tableset.
	 */
	KASSERT(error == 0 || (nset || rpset || rlset || tblset));
	if (nset) {
		npf_ruleset_destroy(nset);
	}
	if (rlset) {
		npf_ruleset_destroy(rlset);
	}
	if (rpset) {
		npf_rprocset_destroy(rpset);
	}
	if (tblset) {
		npf_tableset_destroy(tblset);
	}
	if (error) {
		prop_object_release(npf_dict);
	}

	/* Error report. */
#ifndef _NPF_TESTING
	prop_dictionary_set_int32(errdict, "errno", error);
	prop_dictionary_copyout_ioctl(pref, cmd, errdict);
	prop_object_release(errdict);
	error = 0;
#endif
	return error;
}

/*
 * npfctl_rule: add or remove dynamic rules in the specified ruleset.
 */
int
npfctl_rule(u_long cmd, void *data)
{
	struct plistref *pref = data;
	prop_dictionary_t npf_rule, retdict = NULL;
	npf_ruleset_t *rlset;
	npf_rule_t *rl = NULL;
	const char *ruleset_name;
	uint32_t rcmd = 0;
	int error;

	error = prop_dictionary_copyin_ioctl(pref, cmd, &npf_rule);
	if (error) {
		return error;
	}
	prop_dictionary_get_uint32(npf_rule, "command", &rcmd);
	if (!prop_dictionary_get_cstring_nocopy(npf_rule,
	    "ruleset-name", &ruleset_name)) {
		return EINVAL;
	}

	if (rcmd == NPF_CMD_RULE_ADD) {
		if ((rl = npf_rule_alloc(npf_rule)) == NULL) {
			return EINVAL;
		}
		retdict = prop_dictionary_create();
		prop_dictionary_set_uint64(retdict, "id",
		    (uint64_t)(uintptr_t)rl);
	}

	npf_config_enter();
	rlset = npf_config_ruleset();

	switch (rcmd) {
	case NPF_CMD_RULE_ADD: {
		if ((error = npf_ruleset_add(rlset, ruleset_name, rl)) == 0) {
			/* Success. */
			rl = NULL;
		}
		break;
	}
	case NPF_CMD_RULE_REMOVE: {
		uint64_t id64;

		CTASSERT(sizeof(uintptr_t) <= sizeof(uint64_t));
		if (!prop_dictionary_get_uint64(npf_rule, "id", &id64)) {
			error = EINVAL;
			break;
		}
		error = npf_ruleset_remove(rlset, ruleset_name, (uintptr_t)id64);
		break;
	}
	case NPF_CMD_RULE_REMKEY: {
		prop_object_t obj = prop_dictionary_get(npf_rule, "key");
		const void *key = prop_data_data_nocopy(obj);
		size_t len = prop_data_size(obj);

		if (len == 0 || len > NPF_RULE_MAXKEYLEN) {
			error = EINVAL;
			break;
		}
		error = npf_ruleset_remkey(rlset, ruleset_name, key, len);
		break;
	}
	case NPF_CMD_RULE_LIST: {
		retdict = npf_ruleset_list(rlset, ruleset_name);
		break;
	}
	case NPF_CMD_RULE_FLUSH: {
		error = npf_ruleset_flush(rlset, ruleset_name);
		break;
	}
	default:
		error = EINVAL;
		break;
	}

	/* Destroy any removed rules. */
	if (!error && rcmd != NPF_CMD_RULE_ADD && rcmd != NPF_CMD_RULE_LIST) {
		npf_config_sync();
		npf_ruleset_gc(rlset);
	}
	npf_config_exit();

	if (rl) {
		npf_rule_free(rl);
	}
	if (retdict) {
		prop_object_release(npf_rule);
		prop_dictionary_copyout_ioctl(pref, cmd, retdict);
		prop_object_release(retdict);
	}
	return error;
}

/*
 * npfctl_getconf: return the config dictionary as it was submitted.
 * Additionally, indicate whether the ruleset is currently active.
 */
int
npfctl_getconf(u_long cmd, void *data)
{
	struct plistref *pref = data;
	prop_dictionary_t npf_dict;
	int error;

	npf_config_enter();
	npf_dict = npf_config_dict();
	prop_dictionary_set_bool(npf_dict, "active", npf_pfil_registered_p());
	error = prop_dictionary_copyout_ioctl(pref, cmd, npf_dict);
	npf_config_exit();

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
	error = prop_dictionary_copyout_ioctl(pref, cmd, sesdict);
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
	error = prop_dictionary_copyin_ioctl(pref, cmd, &sesdict);
	if (error)
		return error;

	/*
	 * Note: session objects contain the references to the NAT policy
	 * entries.  Therefore, no need to directly access it.
	 */
	selist = prop_dictionary_get(sesdict, "session-list");
	if (prop_object_type(selist) != PROP_TYPE_ARRAY) {
		prop_object_release(selist);
		return EINVAL;
	}

	/* Create a session hash table. */
	sehasht = sess_htable_create();
	if (sehasht == NULL) {
		prop_object_release(selist);
		return ENOMEM;
	}

	/*
	 * Iterate through and construct each session.  Note: acquire the
	 * config lock as we access NAT policies during the restore.
	 */
	error = 0;
	npf_config_enter();
	it = prop_array_iterator(selist);
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
	sess_htable_reload(sehasht);
fail:
	npf_config_exit();
	prop_object_iterator_release(it);
	prop_object_release(selist);
	if (error) {
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
	const npf_ioctl_table_t *nct = data;
	npf_tableset_t *tblset;
	int error;

	//npf_config_ref();
	tblset = npf_config_tableset();

	switch (nct->nct_cmd) {
	case NPF_CMD_TABLE_LOOKUP:
		error = npf_table_lookup(tblset, nct->nct_tid,
		    nct->nct_data.ent.alen, &nct->nct_data.ent.addr);
		break;
	case NPF_CMD_TABLE_ADD:
		error = npf_table_insert(tblset, nct->nct_tid,
		    nct->nct_data.ent.alen, &nct->nct_data.ent.addr,
		    nct->nct_data.ent.mask);
		break;
	case NPF_CMD_TABLE_REMOVE:
		error = npf_table_remove(tblset, nct->nct_tid,
		    nct->nct_data.ent.alen, &nct->nct_data.ent.addr,
		    nct->nct_data.ent.mask);
		break;
	case NPF_CMD_TABLE_LIST:
		error = npf_table_list(tblset, nct->nct_tid,
		    nct->nct_data.buf.buf, nct->nct_data.buf.len);
		break;
	default:
		error = EINVAL;
		break;
	}
	//npf_config_unref();

	return error;
}
