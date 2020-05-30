/*-
 * Copyright (c) 2009-2020 The NetBSD Foundation, Inc.
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
 * NPF nvlist(3) consumer.
 */

#ifdef _KERNEL
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: npf_ctl.c,v 1.60 2020/05/30 14:16:56 rmind Exp $");

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/kmem.h>
#include <net/bpf.h>
#endif

#include "npf_impl.h"
#include "npf_conn.h"

#define	NPF_ERR_DEBUG(e) \
	nvlist_add_string((e), "source-file", __FILE__); \
	nvlist_add_number((e), "source-line", __LINE__);

static int __noinline
npf_mk_params(npf_t *npf, const nvlist_t *req, nvlist_t *resp, bool set)
{
	const nvlist_t *params;
	int type, error, val;
	const char *name;
	void *cookie;

	params = dnvlist_get_nvlist(req, "params", NULL);
	if (params == NULL) {
		return 0;
	}
	cookie = NULL;
	while ((name = nvlist_next(params, &type, &cookie)) != NULL) {
		if (type != NV_TYPE_NUMBER) {
			NPF_ERR_DEBUG(resp);
			return EINVAL;
		}
		val = (int)nvlist_get_number(params, name);
		if (set) {
			/* Actually set the parameter. */
			error = npfk_param_set(npf, name, val);
			KASSERT(error == 0);
			continue;
		}

		/* Validate the parameter and its value. */
		error = npf_param_check(npf, name, val);
		if (__predict_true(error == 0)) {
			continue;
		}
		if (error == ENOENT) {
			nvlist_add_stringf(resp, "error-msg",
			    "invalid parameter `%s`", name);
		}
		if (error == EINVAL) {
			nvlist_add_stringf(resp, "error-msg",
			    "invalid parameter `%s` value %d", name, val);
		}
		return error;
	}
	return 0;
}

static int __noinline
npf_mk_table_entries(npf_table_t *t, const nvlist_t *req, nvlist_t *resp)
{
	const nvlist_t * const *entries;
	size_t nitems;
	int error = 0;

	if (!nvlist_exists_nvlist_array(req, "entries")) {
		return 0;
	}
	entries = nvlist_get_nvlist_array(req, "entries", &nitems);
	for (unsigned i = 0; i < nitems; i++) {
		const nvlist_t *entry = entries[i];
		const npf_addr_t *addr;
		npf_netmask_t mask;
		size_t alen;

		/* Get address and mask; add a table entry. */
		addr = dnvlist_get_binary(entry, "addr", &alen, NULL, 0);
		mask = dnvlist_get_number(entry, "mask", NPF_NO_NETMASK);
		if (addr == NULL || alen == 0) {
			NPF_ERR_DEBUG(resp);
			error = EINVAL;
			break;
		}
		error = npf_table_insert(t, alen, addr, mask);
		if (__predict_false(error)) {
			if (error == EEXIST) {
				nvlist_add_stringf(resp, "error-msg",
				    "table `%s' has a duplicate entry",
				    nvlist_get_string(req, "name"));
			} else {
				NPF_ERR_DEBUG(resp);
			}
			break;
		}
	}
	return error;
}

/*
 * npf_mk_table: create a table from provided nvlist.
 */
static int __noinline
npf_mk_table(npf_t *npf, const nvlist_t *req, nvlist_t *resp,
    npf_tableset_t *tblset, npf_table_t **tblp, bool replacing)
{
	npf_table_t *t;
	const char *name;
	const void *blob;
	uint64_t tid;
	size_t size;
	int type;
	int error = 0;

	KASSERT(tblp != NULL);

	/* Table name, ID and type.  Validate them. */
	name = dnvlist_get_string(req, "name", NULL);
	if (!name) {
		NPF_ERR_DEBUG(resp);
		error = EINVAL;
		goto out;
	}
	tid = dnvlist_get_number(req, "id", UINT64_MAX);
	type = dnvlist_get_number(req, "type", UINT64_MAX);
	error = npf_table_check(tblset, name, tid, type, replacing);
	if (error) {
		NPF_ERR_DEBUG(resp);
		goto out;
	}

	/* Get the entries or binary data. */
	blob = dnvlist_get_binary(req, "data", &size, NULL, 0);
	if (type == NPF_TABLE_CONST && (blob == NULL || size == 0)) {
		NPF_ERR_DEBUG(resp);
		error = EINVAL;
		goto out;
	}

	t = npf_table_create(name, (unsigned)tid, type, blob, size);
	if (t == NULL) {
		NPF_ERR_DEBUG(resp);
		error = ENOMEM;
		goto out;
	}

	if ((error = npf_mk_table_entries(t, req, resp)) != 0) {
		npf_table_destroy(t);
		goto out;
	}

	*tblp = t;
out:
	return error;
}

static int __noinline
npf_mk_tables(npf_t *npf, const nvlist_t *req, nvlist_t *resp, npf_config_t *nc)
{
	const nvlist_t * const *tables;
	npf_tableset_t *tblset;
	size_t nitems;
	int error = 0;

	if (nvlist_exists_nvlist_array(req, "tables")) {
		tables = nvlist_get_nvlist_array(req, "tables", &nitems);
		if (nitems > NPF_MAX_TABLES) {
			NPF_ERR_DEBUG(resp);
			return E2BIG;
		}
	} else {
		tables = NULL;
		nitems = 0;
	}
	tblset = npf_tableset_create(nitems);
	for (unsigned i = 0; i < nitems; i++) {
		const nvlist_t *table = tables[i];
		npf_table_t *t;

		error = npf_mk_table(npf, table, resp, tblset, &t, 0);
		if (error) {
			break;
		}

		error = npf_tableset_insert(tblset, t);
		KASSERT(error == 0);
	}
	nc->tableset = tblset;
	return error;
}

static npf_rproc_t *
npf_mk_singlerproc(npf_t *npf, const nvlist_t *rproc, nvlist_t *resp)
{
	const nvlist_t * const *extcalls;
	size_t nitems;
	npf_rproc_t *rp;

	if ((rp = npf_rproc_create(rproc)) == NULL) {
		NPF_ERR_DEBUG(resp);
		return NULL;
	}
	if (!nvlist_exists_nvlist_array(rproc, "extcalls")) {
		return rp;
	}
	extcalls = nvlist_get_nvlist_array(rproc, "extcalls", &nitems);
	for (unsigned i = 0; i < nitems; i++) {
		const nvlist_t *extcall = extcalls[i];
		const char *name;

		name = dnvlist_get_string(extcall, "name", NULL);
		if (!name || npf_ext_construct(npf, name, rp, extcall)) {
			NPF_ERR_DEBUG(resp);
			npf_rproc_release(rp);
			rp = NULL;
			break;
		}
	}
	return rp;
}

static int __noinline
npf_mk_rprocs(npf_t *npf, const nvlist_t *req, nvlist_t *resp, npf_config_t *nc)
{
	const nvlist_t * const *rprocs;
	npf_rprocset_t *rpset;
	size_t nitems;
	int error = 0;

	if (nvlist_exists_nvlist_array(req, "rprocs")) {
		rprocs = nvlist_get_nvlist_array(req, "rprocs", &nitems);
		if (nitems > NPF_MAX_RPROCS) {
			NPF_ERR_DEBUG(resp);
			return E2BIG;
		}
	} else {
		rprocs = NULL;
		nitems = 0;
	}
	rpset = npf_rprocset_create();
	for (unsigned i = 0; i < nitems; i++) {
		const nvlist_t *rproc = rprocs[i];
		npf_rproc_t *rp;

		if ((rp = npf_mk_singlerproc(npf, rproc, resp)) == NULL) {
			error = EINVAL;
			break;
		}
		npf_rprocset_insert(rpset, rp);
	}
	nc->rule_procs = rpset;
	return error;
}

static int __noinline
npf_mk_algs(npf_t *npf, const nvlist_t *req, nvlist_t *resp)
{
	const nvlist_t * const *algs;
	size_t nitems;

	if (nvlist_exists_nvlist_array(req, "algs")) {
		algs = nvlist_get_nvlist_array(req, "algs", &nitems);
	} else {
		algs = NULL;
		nitems = 0;
	}
	for (unsigned i = 0; i < nitems; i++) {
		const nvlist_t *alg = algs[i];
		const char *name;

		name = dnvlist_get_string(alg, "name", NULL);
		if (!name) {
			NPF_ERR_DEBUG(resp);
			return EINVAL;
		}
		if (!npf_alg_construct(npf, name)) {
			NPF_ERR_DEBUG(resp);
			return EINVAL;
		}
	}
	return 0;
}

static int __noinline
npf_mk_singlerule(npf_t *npf, const nvlist_t *req, nvlist_t *resp,
    npf_rprocset_t *rpset, npf_rule_t **rlret)
{
	npf_rule_t *rl;
	const char *rname;
	const void *code;
	size_t clen;
	int error = 0;

	if ((rl = npf_rule_alloc(npf, req)) == NULL) {
		NPF_ERR_DEBUG(resp);
		return EINVAL;
	}

	/* Assign the rule procedure, if any. */
	if ((rname = dnvlist_get_string(req, "rproc", NULL)) != NULL) {
		npf_rproc_t *rp;

		if (rpset == NULL) {
			NPF_ERR_DEBUG(resp);
			error = EINVAL;
			goto err;
		}
		if ((rp = npf_rprocset_lookup(rpset, rname)) == NULL) {
			NPF_ERR_DEBUG(resp);
			error = EINVAL;
			goto err;
		}
		npf_rule_setrproc(rl, rp);
	}

	/* Filter byte-code (binary data). */
	code = dnvlist_get_binary(req, "code", &clen, NULL, 0);
	if (code) {
		void *bc;
		int type;

		type = dnvlist_get_number(req, "code-type", UINT64_MAX);
		if (type != NPF_CODE_BPF) {
			NPF_ERR_DEBUG(resp);
			error = ENOTSUP;
			goto err;
		}
		if (clen == 0) {
			NPF_ERR_DEBUG(resp);
			error = EINVAL;
			goto err;
		}
		if (!npf_bpf_validate(code, clen)) {
			NPF_ERR_DEBUG(resp);
			error = EINVAL;
			goto err;
		}
		bc = kmem_alloc(clen, KM_SLEEP);
		memcpy(bc, code, clen); // XXX: use nvlist_take
		npf_rule_setcode(rl, type, bc, clen);
	}

	*rlret = rl;
	return 0;
err:
	nvlist_add_number(resp, "id", dnvlist_get_number(req, "prio", 0));
	npf_rule_free(rl);
	return error;
}

static int __noinline
npf_mk_rules(npf_t *npf, const nvlist_t *req, nvlist_t *resp, npf_config_t *nc)
{
	const nvlist_t * const *rules;
	npf_ruleset_t *rlset;
	size_t nitems;
	int error = 0;

	if (nvlist_exists_nvlist_array(req, "rules")) {
		rules = nvlist_get_nvlist_array(req, "rules", &nitems);
		if (nitems > NPF_MAX_RULES) {
			NPF_ERR_DEBUG(resp);
			return E2BIG;
		}
	} else {
		rules = NULL;
		nitems = 0;
	}
	rlset = npf_ruleset_create(nitems);
	for (unsigned i = 0; i < nitems; i++) {
		const nvlist_t *rule = rules[i];
		npf_rule_t *rl = NULL;
		const char *name;

		error = npf_mk_singlerule(npf, rule, resp, nc->rule_procs, &rl);
		if (error) {
			break;
		}
		name = dnvlist_get_string(rule, "name", NULL);
		if (name && npf_ruleset_lookup(rlset, name)) {
			NPF_ERR_DEBUG(resp);
			npf_rule_free(rl);
			error = EEXIST;
			break;
		}
		npf_ruleset_insert(rlset, rl);
	}
	nc->ruleset = rlset;
	return error;
}

static int __noinline
npf_mk_singlenat(npf_t *npf, const nvlist_t *nat, nvlist_t *resp,
    npf_ruleset_t *ntset, npf_tableset_t *tblset, npf_rule_t **rlp)
{
	npf_rule_t *rl = NULL;
	npf_natpolicy_t *np;
	int error;

	/*
	 * NAT rules are standard rules, plus the translation policy.
	 * We first construct the rule structure.
	 */
	error = npf_mk_singlerule(npf, nat, resp, NULL, &rl);
	if (error) {
		return error;
	}
	KASSERT(rl != NULL);
	*rlp = rl;

	/* If this rule is named, then it is a group with NAT policies. */
	if (dnvlist_get_string(nat, "name", NULL)) {
		return 0;
	}

	/* Check the table ID. */
	if (nvlist_exists_number(nat, "nat-table-id")) {
		unsigned tid = nvlist_get_number(nat, "nat-table-id");

		if (!npf_tableset_getbyid(tblset, tid)) {
			NPF_ERR_DEBUG(resp);
			error = EINVAL;
			goto out;
		}
	}

	/* Allocate a new NAT policy and assign it to the rule. */
	np = npf_natpolicy_create(npf, nat, ntset);
	if (np == NULL) {
		NPF_ERR_DEBUG(resp);
		error = ENOMEM;
		goto out;
	}
	npf_rule_setnat(rl, np);
out:
	if (error) {
		npf_rule_free(rl);
	}
	return error;
}

static int __noinline
npf_mk_natlist(npf_t *npf, const nvlist_t *req, nvlist_t *resp, npf_config_t *nc)
{
	const nvlist_t * const *nat_rules;
	npf_ruleset_t *ntset;
	size_t nitems;
	int error = 0;

	/*
	 * NAT policies must be an array, but enforce a limit.
	 */
	if (nvlist_exists_nvlist_array(req, "nat")) {
		nat_rules = nvlist_get_nvlist_array(req, "nat", &nitems);
		if (nitems > NPF_MAX_RULES) {
			NPF_ERR_DEBUG(resp);
			return E2BIG;
		}
	} else {
		nat_rules = NULL;
		nitems = 0;
	}
	ntset = npf_ruleset_create(nitems);
	for (unsigned i = 0; i < nitems; i++) {
		const nvlist_t *nat = nat_rules[i];
		npf_rule_t *rl = NULL;

		error = npf_mk_singlenat(npf, nat, resp, ntset,
		    nc->tableset, &rl);
		if (error) {
			break;
		}
		npf_ruleset_insert(ntset, rl);
	}
	nc->nat_ruleset = ntset;
	return error;
}

/*
 * npf_mk_connlist: import a list of connections and load them.
 */
static int __noinline
npf_mk_connlist(npf_t *npf, const nvlist_t *req, nvlist_t *resp,
    npf_config_t *nc, npf_conndb_t **conndb)
{
	const nvlist_t * const *conns;
	npf_conndb_t *cd;
	size_t nitems;
	int error = 0;

	if (!nvlist_exists_nvlist_array(req, "conn-list")) {
		*conndb = NULL;
		return 0;
	}
	cd = npf_conndb_create();
	conns = nvlist_get_nvlist_array(req, "conn-list", &nitems);
	for (unsigned i = 0; i < nitems; i++) {
		const nvlist_t *conn = conns[i];

		/* Construct and insert the connection. */
		error = npf_conn_import(npf, cd, conn, nc->nat_ruleset);
		if (error) {
			NPF_ERR_DEBUG(resp);
			break;
		}
	}
	if (error) {
		npf_conndb_gc(npf, cd, true, false);
		npf_conndb_destroy(cd);
	} else {
		*conndb = cd;
	}
	return error;
}

/*
 * npfctl_load: store passed data i.e. the update settings, create the
 * passed rules, tables, etc and atomically activate them all.
 */
static int
npfctl_load(npf_t *npf, const nvlist_t *req, nvlist_t *resp)
{
	npf_config_t *nc;
	npf_conndb_t *conndb = NULL;
	bool flush;
	int error;

	nc = npf_config_create();
	error = npf_mk_params(npf, req, resp, false /* validate */);
	if (error) {
		goto fail;
	}
	error = npf_mk_algs(npf, req, resp);
	if (error) {
		goto fail;
	}
	error = npf_mk_tables(npf, req, resp, nc);
	if (error) {
		goto fail;
	}
	error = npf_mk_rprocs(npf, req, resp, nc);
	if (error) {
		goto fail;
	}
	error = npf_mk_natlist(npf, req, resp, nc);
	if (error) {
		goto fail;
	}
	error = npf_mk_rules(npf, req, resp, nc);
	if (error) {
		goto fail;
	}
	error = npf_mk_connlist(npf, req, resp, nc, &conndb);
	if (error) {
		goto fail;
	}

	flush = dnvlist_get_bool(req, "flush", false);
	nc->default_pass = flush;

	/*
	 * Finally - perform the load.
	 */
	npf_config_load(npf, nc, conndb, flush);
	npf_mk_params(npf, req, resp, true /* set the params */);
	return 0;

fail:
	npf_config_destroy(nc);
	return error;
}

/*
 * npfctl_save: export the active configuration, including the current
 * snapshot of the connections.  Additionally, set the version and indicate
 * whether the ruleset is currently active.
 */
static int
npfctl_save(npf_t *npf, const nvlist_t *req, nvlist_t *resp)
{
	npf_config_t *nc;
	int error;

	/*
	 * Serialize the whole NPF configuration, including connections.
	 */
	nvlist_add_number(resp, "version", NPF_VERSION);
	nc = npf_config_enter(npf);
	error = npf_params_export(npf, resp);
	if (error) {
		goto out;
	}
	error = npf_conndb_export(npf, resp);
	if (error) {
		goto out;
	}
	error = npf_ruleset_export(npf, nc->ruleset, "rules", resp);
	if (error) {
		goto out;
	}
	error = npf_ruleset_export(npf, nc->nat_ruleset, "nat", resp);
	if (error) {
		goto out;
	}
	error = npf_tableset_export(npf, nc->tableset, resp);
	if (error) {
		goto out;
	}
	error = npf_rprocset_export(nc->rule_procs, resp);
	if (error) {
		goto out;
	}
	error = npf_alg_export(npf, resp);
	if (error) {
		goto out;
	}
	nvlist_add_bool(resp, "active", npf_active_p());
out:
	npf_config_exit(npf);
	return error;
}

/*
 * npfctl_table_replace: atomically replace a table's contents with
 * the passed table data.
 */
static int __noinline
npfctl_table_replace(npf_t *npf, const nvlist_t *req, nvlist_t *resp)
{
	npf_table_t *tbl, *gc_tbl = NULL;
	npf_config_t *nc;
	int error = 0;

	nc = npf_config_enter(npf);
	error = npf_mk_table(npf, req, resp, nc->tableset, &tbl, true);
	if (error) {
		goto err;
	}
	gc_tbl = npf_tableset_swap(nc->tableset, tbl);
	if (gc_tbl == NULL) {
		error = EINVAL;
		gc_tbl = tbl;
		goto err;
	}
	npf_config_sync(npf);
err:
	npf_config_exit(npf);
	if (gc_tbl) {
		npf_table_destroy(gc_tbl);
	}
	return error;
}

/*
 * npfctl_rule: add or remove dynamic rules in the specified ruleset.
 */
static int
npfctl_rule(npf_t *npf, const nvlist_t *req, nvlist_t *resp)
{
	npf_ruleset_t *rlset;
	npf_rule_t *rl = NULL;
	const char *ruleset_name;
	npf_config_t *nc;
	uint32_t rcmd;
	int error = 0;
	bool natset;

	rcmd = dnvlist_get_number(req, "command", 0);
	natset = dnvlist_get_bool(req, "nat-ruleset", false);
	ruleset_name = dnvlist_get_string(req, "ruleset-name", NULL);
	if (!ruleset_name) {
		error = EINVAL;
		goto out;
	}

	nc = npf_config_enter(npf);
	rlset = natset ? nc->nat_ruleset : nc->ruleset;
	switch (rcmd) {
	case NPF_CMD_RULE_ADD: {
		if (natset) {
			/*
			 * Translation rule.
			 */
			error = npf_mk_singlenat(npf, req, resp, rlset,
			    nc->tableset, &rl);
		} else {
			/*
			 * Standard rule.
			 */
			error = npf_mk_singlerule(npf, req, resp, NULL, &rl);
		}
		if (error) {
			goto out;
		}
		if ((error = npf_ruleset_add(rlset, ruleset_name, rl)) == 0) {
			/* Success. */
			uint64_t id = npf_rule_getid(rl);
			nvlist_add_number(resp, "id", id);
			rl = NULL;
		}
		break;
	}
	case NPF_CMD_RULE_REMOVE: {
		uint64_t id = dnvlist_get_number(req, "id", UINT64_MAX);
		error = npf_ruleset_remove(rlset, ruleset_name, id);
		break;
	}
	case NPF_CMD_RULE_REMKEY: {
		const void *key;
		size_t len;

		key = dnvlist_get_binary(req, "key", &len, NULL, 0);
		if (len == 0 || len > NPF_RULE_MAXKEYLEN) {
			error = EINVAL;
			break;
		}
		error = npf_ruleset_remkey(rlset, ruleset_name, key, len);
		break;
	}
	case NPF_CMD_RULE_LIST: {
		error = npf_ruleset_list(npf, rlset, ruleset_name, resp);
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
out:
	npf_config_exit(npf);

	if (rl) {
		KASSERT(error);
		npf_rule_free(rl);
	}
	return error;
}

/*
 * npfctl_table: add, remove or query entries in the specified table.
 *
 * For maximum performance, the interface is using plain structures.
 */
int
npfctl_table(npf_t *npf, void *data)
{
	const npf_ioctl_table_t *nct = data;
	char tname[NPF_TABLE_MAXNAMELEN];
	npf_config_t *nc;
	npf_table_t *t;
	int error;

	error = copyinstr(nct->nct_name, tname, sizeof(tname), NULL);
	if (error) {
		return error;
	}

	nc = npf_config_enter(npf);
	if ((t = npf_tableset_getbyname(nc->tableset, tname)) == NULL) {
		npf_config_exit(npf);
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
	npf_table_gc(npf, t);
	npf_config_exit(npf);

	return error;
}

/*
 * npfctl_run_op: run a particular NPF operation with a given the request.
 *
 * => Checks the ABI version.
 * => Sets the error number for the response.
 */
int
npfctl_run_op(npf_t *npf, unsigned op, const nvlist_t *req, nvlist_t *resp)
{
	uint64_t ver;
	int error;

	ver = dnvlist_get_number(req, "version", UINT64_MAX);
	if (__predict_false(ver != UINT64_MAX && ver != NPF_VERSION)) {
		return EPROGMISMATCH;
	}
	switch (op) {
	case IOC_NPF_LOAD:
		error = npfctl_load(npf, req, resp);
		break;
	case IOC_NPF_SAVE:
		error = npfctl_save(npf, req, resp);
		break;
	case IOC_NPF_RULE:
		error = npfctl_rule(npf, req, resp);
		break;
	case IOC_NPF_CONN_LOOKUP:
		error = npf_conn_find(npf, req, resp);
		break;
	case IOC_NPF_TABLE_REPLACE:
		error = npfctl_table_replace(npf, req, resp);
		break;
	default:
		error = ENOTTY;
		break;
	}
	nvlist_add_number(resp, "errno", error);
	return error;
}
