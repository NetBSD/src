/*	$NetBSD: npf.c,v 1.17 2013/02/10 23:47:37 rmind Exp $	*/

/*-
 * Copyright (c) 2010-2013 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: npf.c,v 1.17 2013/02/10 23:47:37 rmind Exp $");

#include <sys/types.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <net/if.h>
#include <prop/proplib.h>

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <err.h>

#define	_NPF_PRIVATE
#include "npf.h"

struct nl_config {
	/* Rules, translations, tables, procedures. */
	prop_dictionary_t	ncf_dict;
	prop_array_t		ncf_rules_list;
	prop_array_t		ncf_rproc_list;
	prop_array_t		ncf_table_list;
	prop_array_t		ncf_nat_list;
	/* Debug information. */
	prop_dictionary_t	ncf_debug;
	/* Error report. */
	prop_dictionary_t	ncf_err;
	/* Custom file to externalise property-list. */
	const char *		ncf_plist;
	bool			ncf_flush;
};

struct nl_rule {
	prop_dictionary_t	nrl_dict;
};

struct nl_rproc {
	prop_dictionary_t	nrp_dict;
};

struct nl_table {
	prop_dictionary_t	ntl_dict;
};

struct nl_ext {
	const char *		nxt_name;
	prop_dictionary_t	nxt_dict;
};

static prop_array_t	_npf_ruleset_transform(prop_array_t);

/*
 * CONFIGURATION INTERFACE.
 */

nl_config_t *
npf_config_create(void)
{
	nl_config_t *ncf;

	ncf = calloc(1, sizeof(*ncf));
	if (ncf == NULL) {
		return NULL;
	}
	ncf->ncf_rules_list = prop_array_create();
	ncf->ncf_rproc_list = prop_array_create();
	ncf->ncf_table_list = prop_array_create();
	ncf->ncf_nat_list = prop_array_create();

	ncf->ncf_plist = NULL;
	ncf->ncf_flush = false;

	return ncf;
}

int
npf_config_submit(nl_config_t *ncf, int fd)
{
	const char *plist = ncf->ncf_plist;
	prop_dictionary_t npf_dict;
	prop_array_t rlset;
	int error = 0;

	npf_dict = prop_dictionary_create();
	if (npf_dict == NULL) {
		return ENOMEM;
	}
	prop_dictionary_set_uint32(npf_dict, "version", NPF_VERSION);

	rlset = _npf_ruleset_transform(ncf->ncf_rules_list);
	if (rlset == NULL) {
		prop_object_release(npf_dict);
		return ENOMEM;
	}
	prop_dictionary_set(npf_dict, "rules", rlset);
	prop_object_release(rlset);

	prop_dictionary_set(npf_dict, "rprocs", ncf->ncf_rproc_list);
	prop_dictionary_set(npf_dict, "tables", ncf->ncf_table_list);
	prop_dictionary_set(npf_dict, "translation", ncf->ncf_nat_list);
	prop_dictionary_set_bool(npf_dict, "flush", ncf->ncf_flush);
	if (ncf->ncf_debug) {
		prop_dictionary_set(npf_dict, "debug", ncf->ncf_debug);
	}

	if (plist) {
		if (!prop_dictionary_externalize_to_file(npf_dict, plist)) {
			error = errno;
		}
		prop_object_release(npf_dict);
		return error;
	}

	error = prop_dictionary_sendrecv_ioctl(npf_dict, fd,
	    IOC_NPF_RELOAD, &ncf->ncf_err);
	if (error) {
		prop_object_release(npf_dict);
		assert(ncf->ncf_err == NULL);
		return error;
	}

	prop_dictionary_get_int32(ncf->ncf_err, "errno", &error);
	prop_object_release(npf_dict);
	return error;
}

nl_config_t *
npf_config_retrieve(int fd, bool *active, bool *loaded)
{
	prop_dictionary_t npf_dict;
	nl_config_t *ncf;
	int error;

	error = prop_dictionary_recv_ioctl(fd, IOC_NPF_GETCONF, &npf_dict);
	if (error) {
		return NULL;
	}
	ncf = calloc(1, sizeof(*ncf));
	if (ncf == NULL) {
		prop_object_release(npf_dict);
		return NULL;
	}
	ncf->ncf_dict = npf_dict;
	ncf->ncf_rules_list = prop_dictionary_get(npf_dict, "rules");
	ncf->ncf_rproc_list = prop_dictionary_get(npf_dict, "rprocs");
	ncf->ncf_table_list = prop_dictionary_get(npf_dict, "tables");
	ncf->ncf_nat_list = prop_dictionary_get(npf_dict, "translation");

	prop_dictionary_get_bool(npf_dict, "active", active);
	*loaded = (ncf->ncf_rules_list != NULL);
	return ncf;
}

int
npf_config_flush(int fd)
{
	nl_config_t *ncf;
	int error;

	ncf = npf_config_create();
	if (ncf == NULL) {
		return ENOMEM;
	}
	ncf->ncf_flush = true;
	error = npf_config_submit(ncf, fd);
	npf_config_destroy(ncf);
	return error;
}

void
_npf_config_error(nl_config_t *ncf, nl_error_t *ne)
{
	memset(ne, 0, sizeof(*ne));
	prop_dictionary_get_int32(ncf->ncf_err, "id", &ne->ne_id);
	prop_dictionary_get_cstring(ncf->ncf_err,
	    "source-file", &ne->ne_source_file);
	prop_dictionary_get_uint32(ncf->ncf_err,
	    "source-line", &ne->ne_source_line);
	prop_dictionary_get_int32(ncf->ncf_err,
	    "code-error", &ne->ne_ncode_error);
	prop_dictionary_get_int32(ncf->ncf_err,
	    "code-errat", &ne->ne_ncode_errat);
}

void
npf_config_destroy(nl_config_t *ncf)
{

	if (!ncf->ncf_dict) {
		prop_object_release(ncf->ncf_rules_list);
		prop_object_release(ncf->ncf_rproc_list);
		prop_object_release(ncf->ncf_table_list);
		prop_object_release(ncf->ncf_nat_list);
	}
	if (ncf->ncf_err) {
		prop_object_release(ncf->ncf_err);
	}
	if (ncf->ncf_debug) {
		prop_object_release(ncf->ncf_debug);
	}
	free(ncf);
}

void
_npf_config_setsubmit(nl_config_t *ncf, const char *plist_file)
{

	ncf->ncf_plist = plist_file;
}

static bool
_npf_prop_array_lookup(prop_array_t array, const char *key, const char *name)
{
	prop_dictionary_t dict;
	prop_object_iterator_t it;

	it = prop_array_iterator(array);
	while ((dict = prop_object_iterator_next(it)) != NULL) {
		const char *lname;
		prop_dictionary_get_cstring_nocopy(dict, key, &lname);
		if (strcmp(name, lname) == 0)
			break;
	}
	prop_object_iterator_release(it);
	return dict ? true : false;
}

/*
 * DYNAMIC RULESET INTERFACE.
 */

int
npf_ruleset_add(int fd, const char *rname, nl_rule_t *rl, uintptr_t *id)
{
	prop_dictionary_t rldict = rl->nrl_dict;
	prop_dictionary_t ret;
	uint64_t id64;
	int error;

	prop_dictionary_set_cstring(rldict, "ruleset-name", rname);
	prop_dictionary_set_uint32(rldict, "command", NPF_CMD_RULE_ADD);
	error = prop_dictionary_sendrecv_ioctl(rldict, fd, IOC_NPF_RULE, &ret);
	if (!error) {
		prop_dictionary_get_uint64(ret, "id", &id64);
		*id = (uintptr_t)id64;
	}
	return error;
}

int
npf_ruleset_remove(int fd, const char *rname, uintptr_t id)
{
	prop_dictionary_t rldict;

	rldict = prop_dictionary_create();
	if (rldict == NULL) {
		return ENOMEM;
	}
	prop_dictionary_set_cstring(rldict, "ruleset-name", rname);
	prop_dictionary_set_uint32(rldict, "command", NPF_CMD_RULE_REMOVE);
	__CTASSERT(sizeof(uintptr_t) <= sizeof(uint64_t));
	prop_dictionary_set_uint64(rldict, "id", (uint64_t)id);
	return prop_dictionary_send_ioctl(rldict, fd, IOC_NPF_RULE);
}

int
npf_ruleset_remkey(int fd, const char *rname, const void *key, size_t len)
{
	prop_dictionary_t rldict;
	prop_data_t keyobj;

	rldict = prop_dictionary_create();
	if (rldict == NULL) {
		return ENOMEM;
	}
	prop_dictionary_set_cstring(rldict, "ruleset-name", rname);
	prop_dictionary_set_uint32(rldict, "command", NPF_CMD_RULE_REMKEY);

	keyobj = prop_data_create_data(key, len);
	if (keyobj == NULL) {
		prop_object_release(rldict);
		return ENOMEM;
	}
	prop_dictionary_set(rldict, "key", keyobj);
	prop_object_release(keyobj);

	return prop_dictionary_send_ioctl(rldict, fd, IOC_NPF_RULE);
}

int
npf_ruleset_flush(int fd, const char *rname)
{
	prop_dictionary_t rldict;

	rldict = prop_dictionary_create();
	if (rldict == NULL) {
		return ENOMEM;
	}
	prop_dictionary_set_cstring(rldict, "ruleset-name", rname);
	prop_dictionary_set_uint32(rldict, "command", NPF_CMD_RULE_FLUSH);
	return prop_dictionary_send_ioctl(rldict, fd, IOC_NPF_RULE);
}

/*
 * _npf_ruleset_transform: transform the ruleset representing nested
 * rules with lists into an array.
 */

static void
_npf_ruleset_transform1(prop_array_t rlset, prop_array_t rules)
{
	prop_object_iterator_t it;
	prop_dictionary_t rldict;
	prop_array_t subrlset;

	it = prop_array_iterator(rules);
	while ((rldict = prop_object_iterator_next(it)) != NULL) {
		unsigned idx;

		/* Add rules to the array (reference is retained). */
		prop_array_add(rlset, rldict);

		subrlset = prop_dictionary_get(rldict, "subrules");
		if (subrlset) {
			/* Process subrules recursively. */
			_npf_ruleset_transform1(rlset, subrlset);
			/* Add the skip-to position. */
			idx = prop_array_count(rlset);
			prop_dictionary_set_uint32(rldict, "skip-to", idx);
			prop_dictionary_remove(rldict, "subrules");
		}
	}
	prop_object_iterator_release(it);
}

static prop_array_t
_npf_ruleset_transform(prop_array_t rlset)
{
	prop_array_t nrlset;

	nrlset = prop_array_create();
	_npf_ruleset_transform1(nrlset, rlset);
	return nrlset;
}

/*
 * NPF EXTENSION INTERFACE.
 */

nl_ext_t *
npf_ext_construct(const char *name)
{
	nl_ext_t *ext;

	ext = malloc(sizeof(*ext));
	if (ext == NULL) {
		return NULL;
	}
	ext->nxt_name = strdup(name);
	if (ext->nxt_name == NULL) {
		free(ext);
		return NULL;
	}
	ext->nxt_dict = prop_dictionary_create();

	return ext;
}

void
npf_ext_param_u32(nl_ext_t *ext, const char *key, uint32_t val)
{
	prop_dictionary_t extdict = ext->nxt_dict;
	prop_dictionary_set_uint32(extdict, key, val);
}

void
npf_ext_param_bool(nl_ext_t *ext, const char *key, bool val)
{
	prop_dictionary_t extdict = ext->nxt_dict;
	prop_dictionary_set_bool(extdict, key, val);
}

/*
 * RULE INTERFACE.
 */

nl_rule_t *
npf_rule_create(const char *name, uint32_t attr, u_int if_idx)
{
	prop_dictionary_t rldict;
	nl_rule_t *rl;

	rl = malloc(sizeof(*rl));
	if (rl == NULL) {
		return NULL;
	}
	rldict = prop_dictionary_create();
	if (rldict == NULL) {
		free(rl);
		return NULL;
	}
	if (name) {
		prop_dictionary_set_cstring(rldict, "name", name);
	}
	prop_dictionary_set_uint32(rldict, "attributes", attr);

	if (if_idx) {
		prop_dictionary_set_uint32(rldict, "interface", if_idx);
	}
	rl->nrl_dict = rldict;
	return rl;
}

int
npf_rule_setcode(nl_rule_t *rl, int type, const void *code, size_t len)
{
	prop_dictionary_t rldict = rl->nrl_dict;
	prop_data_t cdata;

	switch (type) {
	case NPF_CODE_NC:
	case NPF_CODE_BPF:
		break;
	default:
		return ENOTSUP;
	}
	prop_dictionary_set_uint32(rldict, "code-type", type);
	if ((cdata = prop_data_create_data(code, len)) == NULL) {
		return ENOMEM;
	}
	prop_dictionary_set(rldict, "code", cdata);
	prop_object_release(cdata);
	return 0;
}

int
npf_rule_setkey(nl_rule_t *rl, const void *key, size_t len)
{
	prop_dictionary_t rldict = rl->nrl_dict;
	prop_data_t kdata;

	if ((kdata = prop_data_create_data(key, len)) == NULL) {
		return ENOMEM;
	}
	prop_dictionary_set(rldict, "key", kdata);
	prop_object_release(kdata);
	return 0;
}

int
npf_rule_setprio(nl_rule_t *rl, pri_t pri)
{
	prop_dictionary_t rldict = rl->nrl_dict;

	prop_dictionary_set_int32(rldict, "priority", pri);
	return 0;
}

int
npf_rule_setproc(nl_rule_t *rl, const char *name)
{
	prop_dictionary_t rldict = rl->nrl_dict;

	prop_dictionary_set_cstring(rldict, "rproc", name);
	return 0;
}

void *
npf_rule_export(nl_rule_t *rl, size_t *length)
{
	prop_dictionary_t rldict = rl->nrl_dict;
	void *xml;

	if ((xml = prop_dictionary_externalize(rldict)) == NULL) {
		return NULL;
	}
	*length = strlen(xml);
	return xml;
}

bool
npf_rule_exists_p(nl_config_t *ncf, const char *name)
{
	return _npf_prop_array_lookup(ncf->ncf_rules_list, "name", name);
}

int
npf_rule_insert(nl_config_t *ncf, nl_rule_t *parent, nl_rule_t *rl)
{
	prop_dictionary_t rldict = rl->nrl_dict;
	prop_array_t rlset;

	if (parent) {
		prop_dictionary_t pdict = parent->nrl_dict;
		rlset = prop_dictionary_get(pdict, "subrules");
		if (rlset == NULL) {
			rlset = prop_array_create();
			prop_dictionary_set(pdict, "subrules", rlset);
			prop_object_release(rlset);
		}
	} else {
		rlset = ncf->ncf_rules_list;
	}
	prop_array_add(rlset, rldict);
	return 0;
}

static int
_npf_rule_foreach1(prop_array_t rules, nl_rule_callback_t func)
{
	prop_dictionary_t rldict;
	prop_object_iterator_t it;
	unsigned reduce[16], n;
	unsigned nlevel;

	if (!rules || prop_object_type(rules) != PROP_TYPE_ARRAY) {
		return ENOENT;
	}
	it = prop_array_iterator(rules);
	if (it == NULL) {
		return ENOMEM;
	}

	nlevel = 0;
	reduce[nlevel] = 0;
	n = 0;

	while ((rldict = prop_object_iterator_next(it)) != NULL) {
		nl_rule_t nrl = { .nrl_dict = rldict };
		uint32_t skipto = 0;

		prop_dictionary_get_uint32(rldict, "skip-to", &skipto);
		(*func)(&nrl, nlevel);
		if (skipto) {
			nlevel++;
			reduce[nlevel] = skipto;
		}
		if (reduce[nlevel] == ++n) {
			assert(nlevel > 0);
			nlevel--;
		}
	}
	prop_object_iterator_release(it);
	return 0;
}

int
_npf_rule_foreach(nl_config_t *ncf, nl_rule_callback_t func)
{
	return _npf_rule_foreach1(ncf->ncf_rules_list, func);
}

int
_npf_ruleset_list(int fd, const char *rname, nl_config_t *ncf)
{
	prop_dictionary_t rldict, ret;
	int error;

	rldict = prop_dictionary_create();
	if (rldict == NULL) {
		return ENOMEM;
	}
	prop_dictionary_set_cstring(rldict, "ruleset-name", rname);
	prop_dictionary_set_uint32(rldict, "command", NPF_CMD_RULE_LIST);
	error = prop_dictionary_sendrecv_ioctl(rldict, fd, IOC_NPF_RULE, &ret);
	if (!error) {
		prop_array_t rules;

		rules = prop_dictionary_get(ret, "rules");
		if (rules == NULL) {
			return EINVAL;
		}
		prop_object_release(ncf->ncf_rules_list);
		ncf->ncf_rules_list = rules;
	}
	return error;
}

pri_t
_npf_rule_getinfo(nl_rule_t *nrl, const char **rname, uint32_t *attr,
    u_int *if_idx)
{
	prop_dictionary_t rldict = nrl->nrl_dict;
	pri_t prio;

	prop_dictionary_get_cstring_nocopy(rldict, "name", rname);
	prop_dictionary_get_uint32(rldict, "attributes", attr);
	prop_dictionary_get_int32(rldict, "priority", &prio);
	prop_dictionary_get_uint32(rldict, "interface", if_idx);
	return prio;
}

const void *
_npf_rule_ncode(nl_rule_t *nrl, size_t *size)
{
	prop_dictionary_t rldict = nrl->nrl_dict;
	prop_object_t obj = prop_dictionary_get(rldict, "code");
	*size = prop_data_size(obj);
	return prop_data_data_nocopy(obj);
}

const char *
_npf_rule_rproc(nl_rule_t *nrl)
{
	prop_dictionary_t rldict = nrl->nrl_dict;
	const char *rpname = NULL;

	prop_dictionary_get_cstring_nocopy(rldict, "rproc", &rpname);
	return rpname;
}

void
npf_rule_destroy(nl_rule_t *rl)
{

	prop_object_release(rl->nrl_dict);
	free(rl);
}

/*
 * RULE PROCEDURE INTERFACE.
 */

nl_rproc_t *
npf_rproc_create(const char *name)
{
	prop_dictionary_t rpdict;
	prop_array_t extcalls;
	nl_rproc_t *nrp;

	nrp = malloc(sizeof(nl_rproc_t));
	if (nrp == NULL) {
		return NULL;
	}
	rpdict = prop_dictionary_create();
	if (rpdict == NULL) {
		free(nrp);
		return NULL;
	}
	prop_dictionary_set_cstring(rpdict, "name", name);

	extcalls = prop_array_create();
	if (extcalls == NULL) {
		prop_object_release(rpdict);
		free(nrp);
		return NULL;
	}
	prop_dictionary_set(rpdict, "extcalls", extcalls);
	prop_object_release(extcalls);

	nrp->nrp_dict = rpdict;
	return nrp;
}

int
npf_rproc_extcall(nl_rproc_t *rp, nl_ext_t *ext)
{
	prop_dictionary_t rpdict = rp->nrp_dict;
	prop_dictionary_t extdict = ext->nxt_dict;
	prop_array_t extcalls;

	extcalls = prop_dictionary_get(rpdict, "extcalls");
	if (_npf_prop_array_lookup(extcalls, "name", ext->nxt_name)) {
		return EEXIST;
	}
	prop_dictionary_set_cstring(extdict, "name", ext->nxt_name);
	prop_array_add(extcalls, extdict);
	return 0;
}

bool
npf_rproc_exists_p(nl_config_t *ncf, const char *name)
{

	return _npf_prop_array_lookup(ncf->ncf_rproc_list, "name", name);
}

int
npf_rproc_insert(nl_config_t *ncf, nl_rproc_t *rp)
{
	prop_dictionary_t rpdict = rp->nrp_dict;
	const char *name;

	if (!prop_dictionary_get_cstring_nocopy(rpdict, "name", &name)) {
		return EINVAL;
	}
	if (npf_rproc_exists_p(ncf, name)) {
		return EEXIST;
	}
	prop_array_add(ncf->ncf_rproc_list, rpdict);
	return 0;
}

/*
 * TRANSLATION INTERFACE.
 */

nl_nat_t *
npf_nat_create(int type, u_int flags, u_int if_idx,
    npf_addr_t *addr, int af, in_port_t port)
{
	nl_rule_t *rl;
	prop_dictionary_t rldict;
	prop_data_t addrdat;
	uint32_t attr;
	size_t sz;

	if (af == AF_INET) {
		sz = sizeof(struct in_addr);
	} else if (af == AF_INET6) {
		sz = sizeof(struct in6_addr);
	} else {
		return NULL;
	}

	attr = NPF_RULE_PASS | NPF_RULE_FINAL |
	    (type == NPF_NATOUT ? NPF_RULE_OUT : NPF_RULE_IN);

	/* Create a rule for NAT policy.  Next, will add translation data. */
	rl = npf_rule_create(NULL, attr, if_idx);
	if (rl == NULL) {
		return NULL;
	}
	rldict = rl->nrl_dict;

	/* Translation type and flags. */
	prop_dictionary_set_int32(rldict, "type", type);
	prop_dictionary_set_uint32(rldict, "flags", flags);

	/* Translation IP. */
	addrdat = prop_data_create_data(addr, sz);
	if (addrdat == NULL) {
		npf_rule_destroy(rl);
		return NULL;
	}
	prop_dictionary_set(rldict, "translation-ip", addrdat);
	prop_object_release(addrdat);

	/* Translation port (for redirect case). */
	prop_dictionary_set_uint16(rldict, "translation-port", port);

	return (nl_nat_t *)rl;
}

int
npf_nat_insert(nl_config_t *ncf, nl_nat_t *nt, pri_t pri)
{
	prop_dictionary_t rldict = nt->nrl_dict;

	prop_dictionary_set_int32(rldict, "priority", NPF_PRI_LAST);
	prop_array_add(ncf->ncf_nat_list, rldict);
	return 0;
}

int
_npf_nat_foreach(nl_config_t *ncf, nl_rule_callback_t func)
{
	return _npf_rule_foreach1(ncf->ncf_nat_list, func);
}

void
_npf_nat_getinfo(nl_nat_t *nt, int *type, u_int *flags, npf_addr_t *addr,
    size_t *alen, in_port_t *port)
{
	prop_dictionary_t rldict = nt->nrl_dict;

	prop_dictionary_get_int32(rldict, "type", type);
	prop_dictionary_get_uint32(rldict, "flags", flags);

	prop_object_t obj = prop_dictionary_get(rldict, "translation-ip");
	*alen = prop_data_size(obj);
	memcpy(addr, prop_data_data_nocopy(obj), *alen);

	prop_dictionary_get_uint16(rldict, "translation-port", port);
}

/*
 * TABLE INTERFACE.
 */

nl_table_t *
npf_table_create(u_int id, int type)
{
	prop_dictionary_t tldict;
	prop_array_t tblents;
	nl_table_t *tl;

	tl = malloc(sizeof(*tl));
	if (tl == NULL) {
		return NULL;
	}
	tldict = prop_dictionary_create();
	if (tldict == NULL) {
		free(tl);
		return NULL;
	}
	prop_dictionary_set_uint32(tldict, "id", id);
	prop_dictionary_set_int32(tldict, "type", type);

	tblents = prop_array_create();
	if (tblents == NULL) {
		prop_object_release(tldict);
		free(tl);
		return NULL;
	}
	prop_dictionary_set(tldict, "entries", tblents);
	prop_object_release(tblents);

	tl->ntl_dict = tldict;
	return tl;
}

int
npf_table_add_entry(nl_table_t *tl, int af, const npf_addr_t *addr,
    const npf_netmask_t mask)
{
	prop_dictionary_t tldict = tl->ntl_dict, entdict;
	prop_array_t tblents;
	prop_data_t addrdata;
	unsigned alen;

	/* Create the table entry. */
	entdict = prop_dictionary_create();
	if (entdict == NULL) {
		return ENOMEM;
	}

	switch (af) {
	case AF_INET:
		alen = sizeof(struct in_addr);
		break;
	case AF_INET6:
		alen = sizeof(struct in6_addr);
		break;
	default:
		return EINVAL;
	}

	addrdata = prop_data_create_data(addr, alen);
	prop_dictionary_set(entdict, "addr", addrdata);
	prop_dictionary_set_uint8(entdict, "mask", mask);
	prop_object_release(addrdata);

	tblents = prop_dictionary_get(tldict, "entries");
	prop_array_add(tblents, entdict);
	prop_object_release(entdict);
	return 0;
}

bool
npf_table_exists_p(nl_config_t *ncf, u_int tid)
{
	prop_dictionary_t tldict;
	prop_object_iterator_t it;

	it = prop_array_iterator(ncf->ncf_table_list);
	while ((tldict = prop_object_iterator_next(it)) != NULL) {
		u_int i;
		if (prop_dictionary_get_uint32(tldict, "id", &i) && tid == i)
			break;
	}
	prop_object_iterator_release(it);
	return tldict ? true : false;
}

int
npf_table_insert(nl_config_t *ncf, nl_table_t *tl)
{
	prop_dictionary_t tldict = tl->ntl_dict;
	u_int tid;

	if (!prop_dictionary_get_uint32(tldict, "id", &tid)) {
		return EINVAL;
	}
	if (npf_table_exists_p(ncf, tid)) {
		return EEXIST;
	}
	prop_array_add(ncf->ncf_table_list, tldict);
	return 0;
}

void
npf_table_destroy(nl_table_t *tl)
{

	prop_object_release(tl->ntl_dict);
	free(tl);
}

void
_npf_table_foreach(nl_config_t *ncf, nl_table_callback_t func)
{
	prop_dictionary_t tldict;
	prop_object_iterator_t it;

	it = prop_array_iterator(ncf->ncf_table_list);
	while ((tldict = prop_object_iterator_next(it)) != NULL) {
		u_int id;
		int type;

		prop_dictionary_get_uint32(tldict, "id", &id);
		prop_dictionary_get_int32(tldict, "type", &type);
		(*func)(id, type);
	}
	prop_object_iterator_release(it);
}

/*
 * MISC.
 */

int
npf_sessions_recv(int fd, const char *fpath)
{
	prop_dictionary_t sdict;
	int error;

	error = prop_dictionary_recv_ioctl(fd, IOC_NPF_SESSIONS_SAVE, &sdict);
	if (error) {
		return error;
	}
	if (!prop_dictionary_externalize_to_file(sdict, fpath)) {
		error = errno;
	}
	prop_object_release(sdict);
	return error;
}

int
npf_sessions_send(int fd, const char *fpath)
{
	prop_dictionary_t sdict;
	int error;

	if (fpath) {
		sdict = prop_dictionary_internalize_from_file(fpath);
		if (sdict == NULL) {
			return errno;
		}
	} else {
		/* Empty: will flush the sessions. */
		prop_array_t selist = prop_array_create();
		sdict = prop_dictionary_create();
		prop_dictionary_set(sdict, "session-list", selist);
		prop_object_release(selist);
	}
	error = prop_dictionary_send_ioctl(sdict, fd, IOC_NPF_SESSIONS_LOAD);
	prop_object_release(sdict);
	return error;
}

static prop_dictionary_t
_npf_debug_initonce(nl_config_t *ncf)
{
	if (!ncf->ncf_debug) {
		prop_array_t iflist = prop_array_create();
		ncf->ncf_debug = prop_dictionary_create();
		prop_dictionary_set(ncf->ncf_debug, "interfaces", iflist);
		prop_object_release(iflist);
	}
	return ncf->ncf_debug;
}

void
_npf_debug_addif(nl_config_t *ncf, struct ifaddrs *ifa, u_int if_idx)
{
	prop_dictionary_t ifdict, dbg = _npf_debug_initonce(ncf);
	prop_array_t iflist = prop_dictionary_get(dbg, "interfaces");

	if (_npf_prop_array_lookup(iflist, "name", ifa->ifa_name)) {
		return;
	}

	ifdict = prop_dictionary_create();
	prop_dictionary_set_cstring(ifdict, "name", ifa->ifa_name);
	prop_dictionary_set_uint32(ifdict, "flags", ifa->ifa_flags);
	if (!if_idx) {
		if_idx = if_nametoindex(ifa->ifa_name);
	}
	prop_dictionary_set_uint32(ifdict, "idx", if_idx);

	const struct sockaddr *sa = ifa->ifa_addr;
	npf_addr_t addr;
	size_t alen = 0;

	switch (sa ? sa->sa_family : -1) {
	case AF_INET: {
		const struct sockaddr_in *sin = (const void *)sa;
		alen = sizeof(sin->sin_addr);
		memcpy(&addr, &sin->sin_addr, alen);
		break;
	}
	case AF_INET6: {
		const struct sockaddr_in6 *sin6 = (const void *)sa;
		alen = sizeof(sin6->sin6_addr);
		memcpy(&addr, &sin6->sin6_addr, alen);
		break;
	}
	default:
		break;
	}

	if (alen) {
		prop_data_t addrdata = prop_data_create_data(&addr, alen);
		prop_dictionary_set(ifdict, "addr", addrdata);
		prop_object_release(addrdata);
	}
	prop_array_add(iflist, ifdict);
	prop_object_release(ifdict);
}
