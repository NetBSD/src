/*-
 * Copyright (c) 2009-2018 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: npf_ctl.c,v 1.52 2018/10/29 15:37:06 christos Exp $");

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/kmem.h>
#include <net/bpf.h>
#endif

#include "npf_impl.h"
#include "npf_conn.h"

#define	NPF_IOCTL_DATA_LIMIT	(4 * 1024 * 1024)

#define	NPF_ERR_DEBUG(e) \
	nvlist_add_string((e), "source-file", __FILE__); \
	nvlist_add_number((e), "source-line", __LINE__);

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

static int
npf_nvlist_copyin(npf_t *npf, void *data, nvlist_t **nvl)
{
	int error = 0;

	if (npf->mbufops != NULL)
		*nvl = (nvlist_t *)data;
	else
		error = nvlist_copyin(data, nvl, NPF_IOCTL_DATA_LIMIT);
	return error;
}

static int
npf_nvlist_copyout(npf_t *npf, void *data, nvlist_t *nvl)
{
	int error = 0;

	if (npf->mbufops == NULL)
		error = nvlist_copyout(data, nvl);
	nvlist_destroy(nvl);
	return error;
}

static int __noinline
npf_mk_table_entries(npf_table_t *t, const nvlist_t *table, nvlist_t *errdict)
{
	const nvlist_t * const *entries;
	size_t nitems;
	int error = 0;

	if (!nvlist_exists_nvlist_array(table, "entries")) {
		return 0;
	}
	entries = nvlist_get_nvlist_array(table, "entries", &nitems);
	for (unsigned i = 0; i < nitems; i++) {
		const nvlist_t *entry = entries[i];
		const npf_addr_t *addr;
		npf_netmask_t mask;
		size_t alen;

		/* Get address and mask.  Add a table entry. */
		addr = dnvlist_get_binary(entry, "addr", &alen, NULL, 0);
		mask = dnvlist_get_number(entry, "mask", NPF_NO_NETMASK);
		if (addr == NULL || alen == 0) {
			NPF_ERR_DEBUG(errdict);
			error = EINVAL;
			break;
		}
		error = npf_table_insert(t, alen, addr, mask);
		if (error) {
			NPF_ERR_DEBUG(errdict);
			break;
		}
	}
	return error;
}

static int __noinline
npf_mk_tables(npf_t *npf, nvlist_t *npf_dict, nvlist_t *errdict,
    npf_tableset_t **tblsetp)
{
	const nvlist_t * const *tables;
	npf_tableset_t *tblset;
	size_t nitems;
	int error = 0;

	if (nvlist_exists_nvlist_array(npf_dict, "tables")) {
		tables = nvlist_get_nvlist_array(npf_dict, "tables", &nitems);
		if (nitems > NPF_MAX_TABLES) {
			NPF_ERR_DEBUG(errdict);
			return E2BIG;
		}
	} else {
		tables = NULL;
		nitems = 0;
	}
	tblset = npf_tableset_create(nitems);
	for (unsigned i = 0; i < nitems; i++) {
		const nvlist_t *table = tables[i];
		const char *name;
		const void *blob;
		npf_table_t *t;
		uint64_t tid;
		size_t size;
		int type;

		/* Table name, ID and type.  Validate them. */
		name = dnvlist_get_string(table, "name", NULL);
		if (!name) {
			NPF_ERR_DEBUG(errdict);
			error = EINVAL;
			break;
		}
		tid = dnvlist_get_number(table, "id", UINT64_MAX);
		type = dnvlist_get_number(table, "type", UINT64_MAX);
		error = npf_table_check(tblset, name, tid, type);
		if (error) {
			NPF_ERR_DEBUG(errdict);
			break;
		}

		/* Get the entries or binary data. */
		blob = dnvlist_get_binary(table, "data", &size, NULL, 0);
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

		if ((error = npf_mk_table_entries(t, table, errdict)) != 0) {
			break;
		}
	}
	*tblsetp = tblset;
	return error;
}

static npf_rproc_t *
npf_mk_singlerproc(npf_t *npf, const nvlist_t *rproc, nvlist_t *errdict)
{
	const nvlist_t * const *extcalls;
	size_t nitems;
	npf_rproc_t *rp;

	if (!nvlist_exists_nvlist_array(rproc, "extcalls")) {
		NPF_ERR_DEBUG(errdict);
		return NULL;
	}
	rp = npf_rproc_create(rproc);
	if (rp == NULL) {
		NPF_ERR_DEBUG(errdict);
		return NULL;
	}
	extcalls = nvlist_get_nvlist_array(rproc, "extcalls", &nitems);
	for (unsigned i = 0; i < nitems; i++) {
		const nvlist_t *extcall = extcalls[i];
		const char *name;

		name = dnvlist_get_string(extcall, "name", NULL);
		if (!name || npf_ext_construct(npf, name, rp, extcall)) {
			NPF_ERR_DEBUG(errdict);
			npf_rproc_release(rp);
			rp = NULL;
			break;
		}
	}
	return rp;
}

static int __noinline
npf_mk_rprocs(npf_t *npf, nvlist_t *npf_dict, nvlist_t *errdict,
    npf_rprocset_t **rpsetp)
{
	const nvlist_t * const *rprocs;
	npf_rprocset_t *rpset;
	size_t nitems;
	int error = 0;

	if (nvlist_exists_nvlist_array(npf_dict, "rprocs")) {
		rprocs = nvlist_get_nvlist_array(npf_dict, "rprocs", &nitems);
		if (nitems > NPF_MAX_RPROCS) {
			NPF_ERR_DEBUG(errdict);
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

		if ((rp = npf_mk_singlerproc(npf, rproc, errdict)) == NULL) {
			error = EINVAL;
			break;
		}
		npf_rprocset_insert(rpset, rp);
	}
	*rpsetp = rpset;
	return error;
}

static int __noinline
npf_mk_algs(npf_t *npf, nvlist_t *npf_dict, nvlist_t *errdict)
{
	const nvlist_t * const *algs;
	size_t nitems;

	if (nvlist_exists_nvlist_array(npf_dict, "algs")) {
		algs = nvlist_get_nvlist_array(npf_dict, "algs", &nitems);
	} else {
		algs = NULL;
		nitems = 0;
	}
	for (unsigned i = 0; i < nitems; i++) {
		const nvlist_t *alg = algs[i];
		const char *name;

		name = dnvlist_get_string(alg, "name", NULL);
		if (!name) {
			NPF_ERR_DEBUG(errdict);
			return EINVAL;
		}
		if (!npf_alg_construct(npf, name)) {
			NPF_ERR_DEBUG(errdict);
			return EINVAL;
		}
	}
	return 0;
}

static int __noinline
npf_mk_singlerule(npf_t *npf, const nvlist_t *rule, npf_rprocset_t *rpset,
    npf_rule_t **rlret, nvlist_t *errdict)
{
	npf_rule_t *rl;
	const char *rname;
	const void *code;
	size_t clen;
	int error = 0;

	if ((rl = npf_rule_alloc(npf, rule)) == NULL) {
		NPF_ERR_DEBUG(errdict);
		return EINVAL;
	}

	/* Assign the rule procedure, if any. */
	if ((rname = dnvlist_get_string(rule, "rproc", NULL)) != NULL) {
		npf_rproc_t *rp;

		if (rpset == NULL) {
			NPF_ERR_DEBUG(errdict);
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

	/* Filter byte-code (binary data). */
	code = dnvlist_get_binary(rule, "code", &clen, NULL, 0);
	if (code) {
		void *bc;
		int type;

		type = dnvlist_get_number(rule, "code-type", UINT64_MAX);
		if (type != NPF_CODE_BPF) {
			NPF_ERR_DEBUG(errdict);
			error = ENOTSUP;
			goto err;
		}
		if (clen == 0) {
			NPF_ERR_DEBUG(errdict);
			error = EINVAL;
			goto err;
		}
		if (!npf_bpf_validate(code, clen)) {
			NPF_ERR_DEBUG(errdict);
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
	nvlist_add_number(errdict, "id", dnvlist_get_number(rule, "prio", 0));
	npf_rule_free(rl);
	return error;
}

static int __noinline
npf_mk_rules(npf_t *npf, nvlist_t *npf_dict, nvlist_t *errdict,
     npf_rprocset_t *rpset, npf_ruleset_t **rlsetp)
{
	const nvlist_t * const *rules;
	npf_ruleset_t *rlset;
	size_t nitems;
	int error = 0;

	if (nvlist_exists_nvlist_array(npf_dict, "rules")) {
		rules = nvlist_get_nvlist_array(npf_dict, "rules", &nitems);
		if (nitems > NPF_MAX_RULES) {
			NPF_ERR_DEBUG(errdict);
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

		error = npf_mk_singlerule(npf, rule, rpset, &rl, errdict);
		if (error) {
			break;
		}
		name = dnvlist_get_string(rule, "name", NULL);
		if (name && npf_ruleset_lookup(rlset, name)) {
			NPF_ERR_DEBUG(errdict);
			npf_rule_free(rl);
			error = EEXIST;
			break;
		}
		npf_ruleset_insert(rlset, rl);
	}
	*rlsetp = rlset;
	return error;
}

static int __noinline
npf_mk_natlist(npf_t *npf, nvlist_t *npf_dict, nvlist_t *errdict,
    npf_ruleset_t **ntsetp)
{
	const nvlist_t * const *nat_rules;
	npf_ruleset_t *ntset;
	size_t nitems;
	int error = 0;

	/*
	 * NAT policies must be an array, but enforce a limit.
	 */
	if (nvlist_exists_nvlist_array(npf_dict, "nat")) {
		nat_rules = nvlist_get_nvlist_array(npf_dict, "nat", &nitems);
		if (nitems > NPF_MAX_RULES) {
			NPF_ERR_DEBUG(errdict);
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
		npf_natpolicy_t *np;

		/*
		 * NAT policies are standard rules, plus the additional
		 * translation information.  First, make a rule.
		 */
		error = npf_mk_singlerule(npf, nat, NULL, &rl, errdict);
		if (error) {
			break;
		}
		npf_ruleset_insert(ntset, rl);

		/* If rule is named, it is a group with NAT policies. */
		if (dnvlist_get_string(nat, "name", NULL)) {
			continue;
		}

		/* Allocate a new NAT policy and assign to the rule. */
		np = npf_nat_newpolicy(npf, nat, ntset);
		if (np == NULL) {
			NPF_ERR_DEBUG(errdict);
			error = ENOMEM;
			break;
		}
		npf_rule_setnat(rl, np);
	}
	*ntsetp = ntset;
	return error;
}

/*
 * npf_mk_connlist: import a list of connections and load them.
 */
static int __noinline
npf_mk_connlist(npf_t *npf, nvlist_t *npf_dict, nvlist_t *errdict,
    npf_ruleset_t *natlist, npf_conndb_t **conndb)
{
	const nvlist_t * const *conns;
	npf_conndb_t *cd;
	size_t nitems;
	int error = 0;

	if (!nvlist_exists_nvlist_array(npf_dict, "conn-list")) {
		*conndb = NULL;
		return 0;
	}
	cd = npf_conndb_create();
	conns = nvlist_get_nvlist_array(npf_dict, "conn-list", &nitems);
	for (unsigned i = 0; i < nitems; i++) {
		const nvlist_t *conn = conns[i];

		/* Construct and insert the connection. */
		error = npf_conn_import(npf, cd, conn, natlist);
		if (error) {
			NPF_ERR_DEBUG(errdict);
			break;
		}
	}
	if (error) {
		npf_conn_gc(npf, cd, true, false);
		npf_conndb_destroy(cd);
	} else {
		*conndb = cd;
	}
	return error;
}

/*
 * npfctl_load_nvlist: store passed data i.e. the update settings, create
 * the passed tables, rules, etc and atomically activate all them.
 */
static int
npfctl_load_nvlist(npf_t *npf, nvlist_t *npf_dict, nvlist_t *errdict)
{
	npf_tableset_t *tblset = NULL;
	npf_ruleset_t *ntset = NULL;
	npf_rprocset_t *rpset = NULL;
	npf_ruleset_t *rlset = NULL;
	npf_conndb_t *conndb = NULL;
	uint64_t ver;
	bool flush;
	int error;

	ver = dnvlist_get_number(npf_dict, "version", UINT64_MAX);
	if (ver != NPF_VERSION) {
		error = EPROGMISMATCH;
		goto fail;
	}
	error = npf_mk_algs(npf, npf_dict, errdict);
	if (error) {
		goto fail;
	}
	error = npf_mk_natlist(npf, npf_dict, errdict, &ntset);
	if (error) {
		goto fail;
	}
	error = npf_mk_tables(npf, npf_dict, errdict, &tblset);
	if (error) {
		goto fail;
	}
	error = npf_mk_rprocs(npf, npf_dict, errdict, &rpset);
	if (error) {
		goto fail;
	}
	error = npf_mk_rules(npf, npf_dict, errdict, rpset, &rlset);
	if (error) {
		goto fail;
	}
	error = npf_mk_connlist(npf, npf_dict, errdict, ntset, &conndb);
	if (error) {
		goto fail;
	}

	/*
	 * Finally - perform the load.
	 */
	flush = dnvlist_get_bool(npf_dict, "flush", false);
	npf_config_load(npf, rlset, tblset, ntset, rpset, conndb, flush);

	/* Done.  Since data is consumed now, we shall not destroy it. */
	tblset = NULL;
	rpset = NULL;
	rlset = NULL;
	ntset = NULL;
fail:
	/*
	 * Note: the rulesets must be destroyed first, in order to drop
	 * any references to the tableset.
	 */
	if (ntset) {
		npf_ruleset_destroy(ntset);
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
	nvlist_destroy(npf_dict);
	return error;
}

int
npfctl_load(npf_t *npf, u_long cmd, void *data)
{
	nvlist_t *request, *response;
	int error;

	/*
	 * Retrieve the configuration and check the version.
	 * Construct a response with error reporting.
	 */
	error = npf_nvlist_copyin(npf, data, &request);
	if (error) {
		return error;
	}
	response = nvlist_create(0);
	error = npfctl_load_nvlist(npf, request, response);
	nvlist_add_number(response, "errno", error);
	return npf_nvlist_copyout(npf, data, response);
}

/*
 * npfctl_save: export the config dictionary as it was submitted,
 * including the current snapshot of the connections.  Additionally,
 * indicate whether the ruleset is currently active.
 */
int
npfctl_save(npf_t *npf, u_long cmd, void *data)
{
	nvlist_t *npf_dict;
	int error;

	npf_dict = nvlist_create(0);
	nvlist_add_number(npf_dict, "version", NPF_VERSION);

	/*
	 * Serialise the whole NPF config, including connections.
	 */
	npf_config_enter(npf);
	error = npf_conndb_export(npf, npf_dict);
	if (error) {
		goto out;
	}
	error = npf_ruleset_export(npf, npf_config_ruleset(npf), "rules", npf_dict);
	if (error) {
		goto out;
	}
	error = npf_ruleset_export(npf, npf_config_natset(npf), "nat", npf_dict);
	if (error) {
		goto out;
	}
	error = npf_tableset_export(npf, npf_config_tableset(npf), npf_dict);
	if (error) {
		goto out;
	}
	error = npf_rprocset_export(npf_config_rprocs(npf), npf_dict);
	if (error) {
		goto out;
	}
	error = npf_alg_export(npf, npf_dict);
	if (error) {
		goto out;
	}
	nvlist_add_bool(npf_dict, "active", npf_pfil_registered_p());
	error = npf_nvlist_copyout(npf, data, npf_dict);
	npf_dict = NULL;
out:
	npf_config_exit(npf);
	if (npf_dict) {
		nvlist_destroy(npf_dict);
	}
	return error;
}

/*
 * npfctl_conn_lookup: lookup a connection in the list of connections
 */
int
npfctl_conn_lookup(npf_t *npf, u_long cmd, void *data)
{
	nvlist_t *conn_data, *conn_result;
	int error;

	error = npf_nvlist_copyin(npf, data, &conn_data);
	if (error) {
		return error;
	}
	error = npf_conn_find(npf, conn_data, &conn_result);
	if (error) {
		goto out;
	}
	error = npf_nvlist_copyout(npf, data, conn_result);
out:
	nvlist_destroy(conn_data);
	return error;
}

/*
 * npfctl_rule: add or remove dynamic rules in the specified ruleset.
 */
int
npfctl_rule(npf_t *npf, u_long cmd, void *data)
{
	nvlist_t *npf_rule, *retdict = NULL;
	npf_ruleset_t *rlset;
	npf_rule_t *rl = NULL;
	const char *ruleset_name;
	uint32_t rcmd;
	int error = 0;

	error = npf_nvlist_copyin(npf, data, &npf_rule);
	if (error) {
		return error;
	}
	rcmd = dnvlist_get_number(npf_rule, "command", 0);
	ruleset_name = dnvlist_get_string(npf_rule, "ruleset-name", NULL);
	if (!ruleset_name) {
		error = EINVAL;
		goto out;
	}

	if (rcmd == NPF_CMD_RULE_ADD) {
		retdict = nvlist_create(0);
		error = npf_mk_singlerule(npf, npf_rule, NULL, &rl, retdict);
		if (error) {
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
			nvlist_add_number(retdict, "id", id);
			rl = NULL;
		}
		break;
	}
	case NPF_CMD_RULE_REMOVE: {
		uint64_t id = dnvlist_get_number(npf_rule, "id", UINT64_MAX);
		error = npf_ruleset_remove(rlset, ruleset_name, id);
		break;
	}
	case NPF_CMD_RULE_REMKEY: {
		const void *key;
		size_t len;

		key = dnvlist_get_binary(npf_rule, "key", &len, NULL, 0);
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
	if (retdict && npf_nvlist_copyout(npf, data, retdict) != 0) {
		error = EFAULT; // copyout failure
	}
	nvlist_destroy(npf_rule);
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
