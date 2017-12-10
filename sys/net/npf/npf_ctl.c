/*	$NetBSD: npf_ctl.c,v 1.50 2017/12/10 01:18:21 rmind Exp $	*/

/*-
 * Copyright (c) 2009-2014 The NetBSD Foundation, Inc.
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

#ifdef _KERNEL
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: npf_ctl.c,v 1.50 2017/12/10 01:18:21 rmind Exp $");

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/kmem.h>
#include <net/bpf.h>

#include <prop/proplib.h>
#endif

#include "npf_impl.h"
#include "npf_conn.h"

#define	NPF_ERR_DEBUG(e) \
	prop_dictionary_set_cstring_nocopy((e), "source-file", __FILE__); \
	prop_dictionary_set_uint32((e), "source-line", __LINE__);

#ifdef _KERNEL
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
		error = npf_pfil_register(false);
	} else {
		/* Disable: remove pfil hooks. */
		npf_pfil_unregister(false);
		error = 0;
	}
	return error;
}
#endif

static int __noinline
npf_mk_table_entries(npf_table_t *t, prop_array_t entries)
{
	prop_object_iterator_t eit;
	prop_dictionary_t ent;
	int error = 0;

	if (prop_object_type(entries) != PROP_TYPE_ARRAY) {
		return EINVAL;
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

		error = npf_table_insert(t, alen, addr, mask);
		if (error)
			break;
	}
	prop_object_iterator_release(eit);
	return error;
}

static int __noinline
npf_mk_tables(npf_t *npf, npf_tableset_t *tblset, prop_array_t tables,
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
		const char *name;
		npf_table_t *t;
		uint64_t tid;
		int type;

		/* Table - dictionary. */
		if (prop_object_type(tbldict) != PROP_TYPE_DICTIONARY) {
			NPF_ERR_DEBUG(errdict);
			error = EINVAL;
			break;
		}

		/* Table name, ID and type.  Validate them. */
		if (!prop_dictionary_get_cstring_nocopy(tbldict, "name", &name)) {
			NPF_ERR_DEBUG(errdict);
			error = EINVAL;
			break;
		}
		prop_dictionary_get_uint64(tbldict, "id", &tid);
		prop_dictionary_get_int32(tbldict, "type", &type);
		error = npf_table_check(tblset, name, (u_int)tid, type);
		if (error) {
			NPF_ERR_DEBUG(errdict);
			break;
		}

		/* Get the entries or binary data. */
		prop_array_t ents = prop_dictionary_get(tbldict, "entries");
		prop_object_t obj = prop_dictionary_get(tbldict, "data");
		void *blob = prop_data_data(obj);
		size_t size = prop_data_size(obj);

		if (type == NPF_TABLE_CDB && (blob == NULL || size == 0)) {
			NPF_ERR_DEBUG(errdict);
			error = EINVAL;
			break;
		}

		/* Create and insert the table. */
		t = npf_table_create(name, (u_int)tid, type, blob, size);
		if (t == NULL) {
			NPF_ERR_DEBUG(errdict);
			error = ENOMEM;
			break;
		}
		error = npf_tableset_insert(tblset, t);
		KASSERT(error == 0);

		if (ents && (error = npf_mk_table_entries(t, ents)) != 0) {
			NPF_ERR_DEBUG(errdict);
			break;
		}
	}
	prop_object_iterator_release(it);
	/*
	 * Note: in a case of error, caller will free the tableset.
	 */
	return error;
}

static npf_rproc_t *
npf_mk_singlerproc(npf_t *npf, prop_dictionary_t rpdict)
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

		if (!prop_dictionary_get_cstring_nocopy(extdict, "name",
		    &name) || npf_ext_construct(npf, name, rp, extdict)) {
			npf_rproc_release(rp);
			rp = NULL;
			break;
		}
	}
	prop_object_iterator_release(it);
	return rp;
}

static int __noinline
npf_mk_rprocs(npf_t *npf, npf_rprocset_t *rpset, prop_array_t rprocs,
    prop_dictionary_t errdict)
{
	prop_object_iterator_t it;
	prop_dictionary_t rpdict;
	int error = 0;

	it = prop_array_iterator(rprocs);
	while ((rpdict = prop_object_iterator_next(it)) != NULL) {
		npf_rproc_t *rp;

		if ((rp = npf_mk_singlerproc(npf, rpdict)) == NULL) {
			NPF_ERR_DEBUG(errdict);
			error = EINVAL;
			break;
		}
		npf_rprocset_insert(rpset, rp);
	}
	prop_object_iterator_release(it);
	return error;
}

static npf_alg_t *
npf_mk_singlealg(npf_t *npf, prop_dictionary_t aldict)
{
	const char *name;

	if (!prop_dictionary_get_cstring_nocopy(aldict, "name", &name))
		return NULL;
	return npf_alg_construct(npf, name);
}

static int __noinline
npf_mk_algs(npf_t *npf, prop_array_t alglist, prop_dictionary_t errdict)
{
	prop_object_iterator_t it;
	prop_dictionary_t nadict;
	int error = 0;

	it = prop_array_iterator(alglist);
	while ((nadict = prop_object_iterator_next(it)) != NULL) {
		if (npf_mk_singlealg(npf, nadict) == NULL) {
			NPF_ERR_DEBUG(errdict);
			error = EINVAL;
			break;
		}
	}
	prop_object_iterator_release(it);
	return error;
}

static int __noinline
npf_mk_code(prop_object_t obj, int type, void **code, size_t *csize,
    prop_dictionary_t errdict)
{
	const void *cptr;
	size_t clen;
	void *bc;

	if (type != NPF_CODE_BPF) {
		return ENOTSUP;
	}
	cptr = prop_data_data_nocopy(obj);
	if (cptr == NULL || (clen = prop_data_size(obj)) == 0) {
		NPF_ERR_DEBUG(errdict);
		return EINVAL;
	}
	if (!npf_bpf_validate(cptr, clen)) {
		NPF_ERR_DEBUG(errdict);
		return EINVAL;
	}
	bc = kmem_alloc(clen, KM_SLEEP);
	memcpy(bc, cptr, clen);

	*code = bc;
	*csize = clen;
	return 0;
}

static int __noinline
npf_mk_singlerule(npf_t *npf, prop_dictionary_t rldict, npf_rprocset_t *rpset,
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
	if ((rl = npf_rule_alloc(npf, rldict)) == NULL) {
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
	prop_dictionary_get_int32(rldict, "prio", &p); /* XXX */
	prop_dictionary_set_int64(errdict, "id", p);
	return error;
}

static int __noinline
npf_mk_rules(npf_t *npf, npf_ruleset_t *rlset, prop_array_t rules,
    npf_rprocset_t *rpset, prop_dictionary_t errdict)
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
		const char *name;

		error = npf_mk_singlerule(npf, rldict, rpset, &rl, errdict);
		if (error) {
			break;
		}
		if (prop_dictionary_get_cstring_nocopy(rldict, "name", &name) &&
		    npf_ruleset_lookup(rlset, name) != NULL) {
			NPF_ERR_DEBUG(errdict);
			npf_rule_free(rl);
			return EEXIST;
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
npf_mk_natlist(npf_t *npf, npf_ruleset_t *nset, prop_array_t natlist,
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
		error = npf_mk_singlerule(npf, natdict, NULL, &rl, errdict);
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
		np = npf_nat_newpolicy(npf, natdict, nset);
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
 * npf_mk_connlist: import a list of connections and load them.
 */
static int __noinline
npf_mk_connlist(npf_t *npf, prop_array_t conlist, npf_ruleset_t *natlist,
    npf_conndb_t **conndb, prop_dictionary_t errdict)
{
	prop_dictionary_t condict;
	prop_object_iterator_t it;
	npf_conndb_t *cd;
	int error = 0;

	/* Connection list - array */
	if (prop_object_type(conlist) != PROP_TYPE_ARRAY) {
		NPF_ERR_DEBUG(errdict);
		return EINVAL;
	}

	/* Create a connection database. */
	cd = npf_conndb_create();
	it = prop_array_iterator(conlist);
	while ((condict = prop_object_iterator_next(it)) != NULL) {
		/* Connection - dictionary. */
		if (prop_object_type(condict) != PROP_TYPE_DICTIONARY) {
			NPF_ERR_DEBUG(errdict);
			error = EINVAL;
			break;
		}
		/* Construct and insert the connection. */
		error = npf_conn_import(npf, cd, condict, natlist);
		if (error) {
			NPF_ERR_DEBUG(errdict);
			break;
		}
	}
	prop_object_iterator_release(it);
	if (error) {
		npf_conn_gc(npf, cd, true, false);
		npf_conndb_destroy(cd);
	} else {
		*conndb = cd;
	}
	return error;
}

#if defined(_NPF_TESTING) || defined(_NPF_STANDALONE)
int npfctl_testing;
#endif

/*
 * npfctl_load: store passed data i.e. update settings, create passed
 * tables, rules and atomically activate all them.
 */
int
npfctl_load(npf_t *npf, u_long cmd, void *data)
{
	struct plistref *pref = data;
	prop_dictionary_t npf_dict, errdict;
	prop_array_t alglist, natlist, tables, rprocs, rules, conlist;
	npf_tableset_t *tblset = NULL;
	npf_rprocset_t *rpset = NULL;
	npf_ruleset_t *rlset = NULL;
	npf_ruleset_t *nset = NULL;
	npf_conndb_t *conndb = NULL;
	uint32_t ver = 0;
	size_t nitems;
	bool flush;
	int error;

	/* Retrieve the dictionary. */
#if defined(_NPF_TESTING) || defined(_NPF_STANDALONE)
	if (npfctl_testing)
		npf_dict = (prop_dictionary_t)pref;
	else
#endif
	{
		error = prop_dictionary_copyin_ioctl_size(pref, cmd, &npf_dict,
		    4 * 1024 * 1024);
		if (error)
		    return error;
	}

	/* Dictionary for error reporting and version check. */
	errdict = prop_dictionary_create();
	prop_dictionary_get_uint32(npf_dict, "version", &ver);
	if (ver != NPF_VERSION) {
		error = EPROGMISMATCH;
		goto fail;
	}

	/* ALGs. */
	alglist = prop_dictionary_get(npf_dict, "algs");
	error = npf_mk_algs(npf, alglist, errdict);
	if (error) {
		goto fail;
	}

	/* NAT policies. */
	natlist = prop_dictionary_get(npf_dict, "nat");
	if ((nitems = prop_array_count(natlist)) > NPF_MAX_RULES) {
		error = E2BIG;
		goto fail;
	}

	nset = npf_ruleset_create(nitems);
	error = npf_mk_natlist(npf, nset, natlist, errdict);
	if (error) {
		goto fail;
	}

	/* Tables. */
	tables = prop_dictionary_get(npf_dict, "tables");
	if ((nitems = prop_array_count(tables)) > NPF_MAX_TABLES) {
		error = E2BIG;
		goto fail;
	}
	tblset = npf_tableset_create(nitems);
	error = npf_mk_tables(npf, tblset, tables, errdict);
	if (error) {
		goto fail;
	}

	/* Rule procedures. */
	rprocs = prop_dictionary_get(npf_dict, "rprocs");
	if ((nitems = prop_array_count(rprocs)) > NPF_MAX_RPROCS) {
		error = E2BIG;
		goto fail;
	}
	rpset = npf_rprocset_create();
	error = npf_mk_rprocs(npf, rpset, rprocs, errdict);
	if (error) {
		goto fail;
	}

	/* Rules. */
	rules = prop_dictionary_get(npf_dict, "rules");
	if ((nitems = prop_array_count(rules)) > NPF_MAX_RULES) {
		error = E2BIG;
		goto fail;
	}

	rlset = npf_ruleset_create(nitems);
	error = npf_mk_rules(npf, rlset, rules, rpset, errdict);
	if (error) {
		goto fail;
	}

	/* Connections (if loading any). */
	if ((conlist = prop_dictionary_get(npf_dict, "conn-list")) != NULL) {
		error = npf_mk_connlist(npf, conlist, nset, &conndb, errdict);
		if (error) {
			goto fail;
		}
	}

	flush = false;
	prop_dictionary_get_bool(npf_dict, "flush", &flush);

	/*
	 * Finally - perform the load.
	 */
	npf_config_load(npf, rlset, tblset, nset, rpset, conndb, flush);

	/* Done.  Since data is consumed now, we shall not destroy it. */
	tblset = NULL;
	rpset = NULL;
	rlset = NULL;
	nset = NULL;
fail:
	/*
	 * Note: destroy rulesets first, to drop references to the tableset.
	 */
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
#if defined(_NPF_TESTING) || defined(_NPF_STANDALONE)
	/* Free only if allocated by prop_dictionary_copyin_ioctl_size. */
	if (!npfctl_testing)
#endif
		prop_object_release(npf_dict);

	/*
	 * - _NPF_STANDALONE doesn't require to set prop.
	 * - For _NPF_TESTING, if npfctl_testing, setting prop isn't needed,
	 *   otherwise it's needed.
	 */
#ifndef _NPF_STANDALONE
#ifdef _NPF_TESTING
	if (!npfctl_testing) {
#endif
		/* Error report. */
		prop_dictionary_set_int32(errdict, "errno", error);
		prop_dictionary_copyout_ioctl(pref, cmd, errdict);
		error = 0;
#ifdef _NPF_TESTING
	}
#endif
#endif /* _NPF_STANDALONE */
	prop_object_release(errdict);

	return error;
}

/*
 * npfctl_save: export the config dictionary as it was submitted,
 * including the current snapshot of the connections.  Additionally,
 * indicate whether the ruleset is currently active.
 */
int
npfctl_save(npf_t *npf, u_long cmd, void *data)
{
	struct plistref *pref = data;
	prop_array_t rulelist, natlist, tables, rprocs, conlist;
	prop_dictionary_t npf_dict = NULL;
	int error;

	rulelist = prop_array_create();
	natlist = prop_array_create();
	tables = prop_array_create();
	rprocs = prop_array_create();
	conlist = prop_array_create();

	/*
	 * Serialise the connections and NAT policies.
	 */
	npf_config_enter(npf);
	error = npf_conndb_export(npf, conlist);
	if (error) {
		goto out;
	}
	error = npf_ruleset_export(npf, npf_config_ruleset(npf), rulelist);
	if (error) {
		goto out;
	}
	error = npf_ruleset_export(npf, npf_config_natset(npf), natlist);
	if (error) {
		goto out;
	}
	error = npf_tableset_export(npf, npf_config_tableset(npf), tables);
	if (error) {
		goto out;
	}
	error = npf_rprocset_export(npf_config_rprocs(npf), rprocs);
	if (error) {
		goto out;
	}
	prop_array_t alglist = npf_alg_export(npf);

	npf_dict = prop_dictionary_create();
	prop_dictionary_set_uint32(npf_dict, "version", NPF_VERSION);
	prop_dictionary_set_and_rel(npf_dict, "algs", alglist);
	prop_dictionary_set_and_rel(npf_dict, "rules", rulelist);
	prop_dictionary_set_and_rel(npf_dict, "nat", natlist);
	prop_dictionary_set_and_rel(npf_dict, "tables", tables);
	prop_dictionary_set_and_rel(npf_dict, "rprocs", rprocs);
	prop_dictionary_set_and_rel(npf_dict, "conn-list", conlist);
#if !defined(_NPF_STANDALONE)
	prop_dictionary_set_bool(npf_dict, "active", npf_pfil_registered_p());
	error = prop_dictionary_copyout_ioctl(pref, cmd, npf_dict);
#else
	prop_dictionary_set_bool(npf_dict, "active", true);
	/* Userspace: just copy the pointer of the dictionary. */
	CTASSERT(sizeof(prop_dictionary_t) == sizeof(void *));
	memcpy(data, npf_dict, sizeof(void *));
#endif
out:
	npf_config_exit(npf);

	if (!npf_dict) {
		prop_object_release(rulelist);
		prop_object_release(natlist);
		prop_object_release(tables);
		prop_object_release(rprocs);
		prop_object_release(conlist);
	} else {
#if !defined(_NPF_STANDALONE)
		prop_object_release(npf_dict);
#endif
	}
	return error;
}

/*
 * npfctl_conn_lookup: lookup a connection in the list of connections
 */
int
npfctl_conn_lookup(npf_t *npf, u_long cmd, void *data)
{
	struct plistref *pref = data;
	prop_dictionary_t conn_data, conn_result;
	int error;

#if !defined(_NPF_STANDALONE)
	error = prop_dictionary_copyin_ioctl(pref, cmd, &conn_data);
	if (error) {
		return error;
	}
#else
	conn_data = (prop_dictionary_t)pref;
#endif
	error = npf_conn_find(npf, conn_data, &conn_result);
	if (error) {
		goto out;
	}
#if !defined(_NPF_STANDALONE)
	prop_dictionary_copyout_ioctl(pref, cmd, conn_result);
	prop_object_release(conn_result);
#else
	CTASSERT(sizeof(prop_dictionary_t) == sizeof(void *));
	memcpy(data, conn_result, sizeof(void *));
#endif
out:
	prop_object_release(conn_data);
	return error;
}

/*
 * npfctl_rule: add or remove dynamic rules in the specified ruleset.
 */
int
npfctl_rule(npf_t *npf, u_long cmd, void *data)
{
	struct plistref *pref = data;
	prop_dictionary_t npf_rule, retdict = NULL;
	npf_ruleset_t *rlset;
	npf_rule_t *rl = NULL;
	const char *ruleset_name;
	uint32_t rcmd = 0;
	int error = 0;

#if !defined(_NPF_STANDALONE)
	error = prop_dictionary_copyin_ioctl(pref, cmd, &npf_rule);
	if (error) {
		return error;
	}
#else
	npf_rule = (prop_dictionary_t)pref;
#endif
	prop_dictionary_get_uint32(npf_rule, "command", &rcmd);
	if (!prop_dictionary_get_cstring_nocopy(npf_rule,
	    "ruleset-name", &ruleset_name)) {
		error = EINVAL;
		goto out;
	}

	if (rcmd == NPF_CMD_RULE_ADD) {
		retdict = prop_dictionary_create();
		if (npf_mk_singlerule(npf, npf_rule, NULL, &rl, retdict) != 0) {
			error = EINVAL;
			goto out;
		}
	}

	npf_config_enter(npf);
	rlset = npf_config_ruleset(npf);

	switch (rcmd) {
	case NPF_CMD_RULE_ADD: {
		if ((error = npf_ruleset_add(rlset, ruleset_name, rl)) == 0) {
			/* Success. */
			uint64_t id = npf_rule_getid(rl);
			prop_dictionary_set_uint64(retdict, "id", id);
			rl = NULL;
		}
		break;
	}
	case NPF_CMD_RULE_REMOVE: {
		uint64_t id;

		if (!prop_dictionary_get_uint64(npf_rule, "id", &id)) {
			error = EINVAL;
			break;
		}
		error = npf_ruleset_remove(rlset, ruleset_name, id);
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
		retdict = npf_ruleset_list(npf, rlset, ruleset_name);
		if (!retdict) {
			error = ESRCH;
		}
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
		npf_config_sync(npf);
		npf_ruleset_gc(rlset);
	}
	npf_config_exit(npf);

	if (rl) {
		KASSERT(error);
		npf_rule_free(rl);
	}
out:
	if (retdict) {
		prop_object_release(npf_rule);
#if !defined(_NPF_STANDALONE)
		prop_dictionary_copyout_ioctl(pref, cmd, retdict);
		prop_object_release(retdict);
#else
		CTASSERT(sizeof(prop_dictionary_t) == sizeof(void *));
		memcpy(data, npf_rule, sizeof(void *));
#endif
	}
	return error;
}

/*
 * npfctl_table: add, remove or query entries in the specified table.
 *
 * For maximum performance, interface is avoiding proplib(3)'s overhead.
 */
int
npfctl_table(npf_t *npf, void *data)
{
	const npf_ioctl_table_t *nct = data;
	char tname[NPF_TABLE_MAXNAMELEN];
	npf_tableset_t *ts;
	npf_table_t *t;
	int s, error;

	error = copyinstr(nct->nct_name, tname, sizeof(tname), NULL);
	if (error) {
		return error;
	}

	s = npf_config_read_enter(); /* XXX */
	ts = npf_config_tableset(npf);
	if ((t = npf_tableset_getbyname(ts, tname)) == NULL) {
		npf_config_read_exit(s);
		return EINVAL;
	}

	switch (nct->nct_cmd) {
	case NPF_CMD_TABLE_LOOKUP:
		error = npf_table_lookup(t, nct->nct_data.ent.alen,
		    &nct->nct_data.ent.addr);
		break;
	case NPF_CMD_TABLE_ADD:
		error = npf_table_insert(t, nct->nct_data.ent.alen,
		    &nct->nct_data.ent.addr, nct->nct_data.ent.mask);
		break;
	case NPF_CMD_TABLE_REMOVE:
		error = npf_table_remove(t, nct->nct_data.ent.alen,
		    &nct->nct_data.ent.addr, nct->nct_data.ent.mask);
		break;
	case NPF_CMD_TABLE_LIST:
		error = npf_table_list(t, nct->nct_data.buf.buf,
		    nct->nct_data.buf.len);
		break;
	case NPF_CMD_TABLE_FLUSH:
		error = npf_table_flush(t);
		break;
	default:
		error = EINVAL;
		break;
	}
	npf_config_read_exit(s);

	return error;
}
