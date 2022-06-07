/*-
 * Copyright (c) 2010-2020 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: npf.c,v 1.50 2022/06/07 16:27:24 christos Exp $");

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#if !defined(_NPF_STANDALONE)
#include <sys/ioctl.h>
#endif
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <net/if.h>

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <errno.h>
#include <err.h>

#include <nv.h>
#include <dnv.h>

#include <cdbw.h>

#define	_NPF_PRIVATE
#include "npf.h"

struct nl_rule {
	nvlist_t *	rule_dict;
};

struct nl_rproc {
	nvlist_t *	rproc_dict;
};

struct nl_table {
	nvlist_t *	table_dict;
};

struct nl_alg {
	nvlist_t *	alg_dict;
};

struct nl_ext {
	nvlist_t *	ext_dict;
};

struct nl_config {
	nvlist_t *	ncf_dict;

	/* Temporary rule list. */
	nvlist_t **	ncf_rule_list;
	unsigned	ncf_rule_count;

	/* Iterators. */
	unsigned	ncf_reduce[16];
	unsigned	ncf_nlevel;

	nl_rule_t	ncf_cur_rule;
	nl_table_t	ncf_cur_table;
	nl_rproc_t	ncf_cur_rproc;
};

/*
 * Various helper routines.
 */

static bool
_npf_add_addr(nvlist_t *nvl, const char *name, int af, const npf_addr_t *addr)
{
	size_t sz;

	if (af == AF_INET) {
		sz = sizeof(struct in_addr);
	} else if (af == AF_INET6) {
		sz = sizeof(struct in6_addr);
	} else {
		return false;
	}
	nvlist_add_binary(nvl, name, addr, sz);
	return nvlist_error(nvl) == 0;
}

static unsigned
_npf_get_addr(const nvlist_t *nvl, const char *name, npf_addr_t *addr)
{
	const void *d;
	size_t sz = 0;

	d = nvlist_get_binary(nvl, name, &sz);
	switch (sz) {
	case sizeof(struct in_addr):
	case sizeof(struct in6_addr):
		memcpy(addr, d, sz);
		return (unsigned)sz;
	}
	return 0;
}

static bool
_npf_dataset_lookup(const nvlist_t *dict, const char *dataset,
    const char *key, const char *name)
{
	const nvlist_t * const *items;
	size_t nitems;

	if (!nvlist_exists_nvlist_array(dict, dataset)) {
		return false;
	}
	items = nvlist_get_nvlist_array(dict, dataset, &nitems);
	for (unsigned i = 0; i < nitems; i++) {
		const char *item_name;

		item_name = dnvlist_get_string(items[i], key, NULL);
		if (item_name && strcmp(item_name, name) == 0) {
			return true;
		}
	}
	return false;
}

static const nvlist_t *
_npf_dataset_getelement(nvlist_t *dict, const char *dataset, unsigned i)
{
	const nvlist_t * const *items;
	size_t nitems;

	if (!nvlist_exists_nvlist_array(dict, dataset)) {
		return NULL;
	}
	items = nvlist_get_nvlist_array(dict, dataset, &nitems);
	if (i < nitems) {
		return items[i];
	}
	return NULL;
}

/*
 * _npf_rules_process: transform the ruleset representing nested rules
 * with sublists into a single array with skip-to marks.
 */
static void
_npf_rules_process(nl_config_t *ncf, nvlist_t *dict, const char *key)
{
	nvlist_t **items;
	size_t nitems;

	if (!nvlist_exists_nvlist_array(dict, key)) {
		return;
	}
	items = nvlist_take_nvlist_array(dict, key, &nitems);
	for (unsigned i = 0; i < nitems; i++) {
		nvlist_t *rule_dict = items[i];
		size_t len = (ncf->ncf_rule_count + 1) * sizeof(nvlist_t *);
		void *p = realloc(ncf->ncf_rule_list, len);

		/*
		 * - Add rule to the transformed array.
		 * - Process subrules recursively.
		 * - Add the skip-to position.
		 */
		ncf->ncf_rule_list = p;
		ncf->ncf_rule_list[ncf->ncf_rule_count] = rule_dict;
		ncf->ncf_rule_count++;

		if (nvlist_exists_nvlist_array(rule_dict, "subrules")) {
			unsigned idx;

			_npf_rules_process(ncf, rule_dict, "subrules");
			idx = ncf->ncf_rule_count; // post-recursion index
			nvlist_add_number(rule_dict, "skip-to", idx);
		}
		assert(nvlist_error(rule_dict) == 0);
	}
	free(items);
}

/*
 * _npf_init_error: initialize the error structure with the message
 * from the current error number
 */
static int
_npf_init_error(int error, npf_error_t *errinfo)
{
	if (error && errinfo) {
		memset(errinfo, 0, sizeof(*errinfo));
		errinfo->error_msg = strerror(error);
	}
	return error;
}

/*
 * _npf_extract_error: check the error number field and extract the
 * error details into the npf_error_t structure.
 */
static int
_npf_extract_error(nvlist_t *resp, npf_error_t *errinfo)
{
	int error;

	error = dnvlist_get_number(resp, "errno", 0);
	if (error && errinfo) {
		memset(errinfo, 0, sizeof(npf_error_t));

		errinfo->id = dnvlist_get_number(resp, "id", 0);
		errinfo->error_msg =
		    dnvlist_take_string(resp, "error-msg", NULL);
		errinfo->source_file =
		    dnvlist_take_string(resp, "source-file", NULL);
		errinfo->source_line =
		    dnvlist_take_number(resp, "source-line", 0);
	}
	return error;
}

/*
 * npf_xfer_fd: transfer the given request and receive a response.
 *
 * => Sets the 'operation' key on the 'req' dictionary.
 * => On success: returns 0 and valid nvlist in 'resp'.
 * => On failure: returns an error number.
 */
static int
_npf_xfer_fd(int fd, unsigned long cmd, nvlist_t *req, nvlist_t **resp)
{
	struct stat st;
	int kernver;

	/*
	 * Set the NPF version and operation.
	 */
	if (!nvlist_exists(req, "version")) {
		nvlist_add_number(req, "version", NPF_VERSION);
	}
	nvlist_add_number(req, "operation", cmd);

	/*
	 * Determine the type of file descriptor:
	 * - If socket, then perform nvlist_send()/nvlist_recv().
	 * - If a character device, then use ioctl.
	 */
	if (fstat(fd, &st) == -1) {
		goto err;
	}
	switch (st.st_mode & S_IFMT) {
#if !defined(__NetBSD__)
	case S_IFSOCK:
		if (nvlist_send(fd, req) == -1) {
			goto err;
		}
		if (resp && (*resp = nvlist_recv(fd, 0)) == NULL) {
			goto err;
		}
		break;
#endif
#if !defined(_NPF_STANDALONE)
	case S_IFBLK:
	case S_IFCHR:
		if (ioctl(fd, IOC_NPF_VERSION, &kernver) == -1) {
			goto err;
		}
		if (kernver != NPF_VERSION) {
			errno = EPROGMISMATCH;
			goto err;
		}
		if (nvlist_xfer_ioctl(fd, cmd, req, resp) == -1) {
			goto err;
		}
		break;
#else
		(void)kernver;
#endif
	default:
		errno = ENOTSUP;
		goto err;
	}
	return 0;
err:
	return errno ? errno : EIO;
}

/*
 * npf_xfer_fd_errno: same as npf_xfer_fd(), but:
 *
 * => After successful retrieval of the response, inspects it, extracts
 *    the 'errno' value (if any) and returns it.
 * => Destroys the response.
 */
static int
_npf_xfer_fd_errno(int fd, unsigned long cmd, nvlist_t *req)
{
	nvlist_t *resp;
	int error;

	error = _npf_xfer_fd(fd, cmd, req, &resp);
	if (error) {
		return error;
	}
	error = _npf_extract_error(resp, NULL);
	nvlist_destroy(resp);
	return error;
}

/*
 * CONFIGURATION INTERFACE.
 */

nl_config_t *
npf_config_create(void)
{
	nl_config_t *ncf;

	ncf = calloc(1, sizeof(nl_config_t));
	if (!ncf) {
		return NULL;
	}
	ncf->ncf_dict = nvlist_create(0);
	nvlist_add_number(ncf->ncf_dict, "version", NPF_VERSION);
	return ncf;
}

int
npf_config_submit(nl_config_t *ncf, int fd, npf_error_t *errinfo)
{
	nvlist_t *resp = NULL;
	int error;

	/* Ensure the config is built. */
	(void)npf_config_build(ncf);

	error = _npf_xfer_fd(fd, IOC_NPF_LOAD, ncf->ncf_dict, &resp);
	if (error) {
		return _npf_init_error(errno, errinfo);
	}
	error = _npf_extract_error(resp, errinfo);
	nvlist_destroy(resp);
	return error;
}

nl_config_t *
npf_config_retrieve(int fd)
{
	nl_config_t *ncf;
	nvlist_t *req, *resp = NULL;
	int error;

	ncf = calloc(1, sizeof(nl_config_t));
	if (!ncf) {
		return NULL;
	}

	req = nvlist_create(0);
	error = _npf_xfer_fd(fd, IOC_NPF_SAVE, req, &resp);
	nvlist_destroy(req);

	if (error || _npf_extract_error(resp, NULL) != 0) {
		nvlist_destroy(resp);
		free(ncf);
		return NULL;
	}
	ncf->ncf_dict = resp;
	return ncf;
}

void *
npf_config_export(nl_config_t *ncf, size_t *length)
{
	/* Ensure the config is built. */
	(void)npf_config_build(ncf);
	return nvlist_pack(ncf->ncf_dict, length);
}

nl_config_t *
npf_config_import(const void *blob, size_t len)
{
	nl_config_t *ncf;

	ncf = calloc(1, sizeof(nl_config_t));
	if (!ncf) {
		return NULL;
	}
	ncf->ncf_dict = nvlist_unpack(blob, len, 0);
	if (!ncf->ncf_dict) {
		free(ncf);
		return NULL;
	}
	return ncf;
}

int
npf_config_flush(int fd)
{
	nl_config_t *ncf;
	npf_error_t errinfo;
	int error;

	ncf = npf_config_create();
	if (!ncf) {
		return ENOMEM;
	}
	nvlist_add_bool(ncf->ncf_dict, "flush", true);
	error = npf_config_submit(ncf, fd, &errinfo);
	npf_config_destroy(ncf);
	return error;
}

bool
npf_config_active_p(nl_config_t *ncf)
{
	return dnvlist_get_bool(ncf->ncf_dict, "active", false);
}

bool
npf_config_loaded_p(nl_config_t *ncf)
{
	return nvlist_exists_nvlist_array(ncf->ncf_dict, "rules");
}

const void *
npf_config_build(nl_config_t *ncf)
{
	_npf_rules_process(ncf, ncf->ncf_dict, "__rules");
	if (ncf->ncf_rule_list) {
		/* Set the transformed ruleset. */
		nvlist_move_nvlist_array(ncf->ncf_dict, "rules",
		    ncf->ncf_rule_list, ncf->ncf_rule_count);

		/* Clear the temporary list. */
		ncf->ncf_rule_list = NULL;
		ncf->ncf_rule_count = 0;
	}
	assert(nvlist_error(ncf->ncf_dict) == 0);
	return (void *)ncf->ncf_dict;
}

void
npf_config_destroy(nl_config_t *ncf)
{
	nvlist_destroy(ncf->ncf_dict);
	free(ncf);
}

/*
 * PARAMETERS.
 */

int
npf_param_get(nl_config_t *ncf, const char *name, int *valp)
{
	const nvlist_t *params;

	params = dnvlist_get_nvlist(ncf->ncf_dict, "params", NULL);
	if (params == NULL || !nvlist_exists(params, name)) {
		return ENOENT;
	}
	*valp = (int)dnvlist_get_number(params, name, 0);
	return 0;
}

int
npf_param_set(nl_config_t *ncf, const char *name, int val)
{
	nvlist_t *params;

	/* Ensure params dictionary. */
	if (nvlist_exists(ncf->ncf_dict, "params")) {
		params = nvlist_take_nvlist(ncf->ncf_dict, "params");
	} else {
		params = nvlist_create(0);
	}

	/*
	 * If the parameter is already set, then free it first.
	 * Set the parameter.  Note: values can be negative.
	 */
	if (nvlist_exists(params, name)) {
		nvlist_free_number(params, name);
	}
	nvlist_add_number(params, name, (uint64_t)val);
	nvlist_add_nvlist(ncf->ncf_dict, "params", params);
	return 0;
}

const char *
npf_param_iterate(nl_config_t *ncf, nl_iter_t *iter, int *val, int *defval)
{
	void *cookie = (void *)(intptr_t)*iter;
	const nvlist_t *params, *dparams;
	const char *name;
	int type;

	assert(sizeof(nl_iter_t) >= sizeof(void *));

	params = dnvlist_get_nvlist(ncf->ncf_dict, "params", NULL);
	if (params == NULL) {
		return NULL;
	}
skip:
	if ((name = nvlist_next(params, &type, &cookie)) == NULL) {
		*iter = NPF_ITER_BEGIN;
		return NULL;
	}
	if (type != NV_TYPE_NUMBER) {
		goto skip; // should never happen, though
	}
	if (defval) {
		dparams = dnvlist_get_nvlist(ncf->ncf_dict,
		    "params-defaults", NULL);
		if (dparams == NULL) {
			errno = EINVAL;
			return NULL;
		}
		*defval = (int)nvlist_get_number(dparams, name);
	}

	*val = (int)nvlist_get_number(params, name);
	*iter = (intptr_t)cookie;
	return name;
}

/*
 * DYNAMIC RULESET INTERFACE.
 */

static inline bool
_npf_nat_ruleset_p(const char *name)
{
	return strncmp(name, NPF_RULESET_MAP_PREF,
	    sizeof(NPF_RULESET_MAP_PREF) - 1) == 0;
}

int
npf_ruleset_add(int fd, const char *rname, nl_rule_t *rl, uint64_t *id)
{
	const bool natset = _npf_nat_ruleset_p(rname);
	nvlist_t *rule_nvl = rl->rule_dict, *resp;
	int error;

	nvlist_add_number(rule_nvl, "attr",
	    NPF_RULE_DYNAMIC | nvlist_take_number(rule_nvl, "attr"));

	if (natset && !dnvlist_get_bool(rule_nvl, "nat-rule", false)) {
		errno = EINVAL;
		return errno;
	}
	nvlist_add_string(rule_nvl, "ruleset-name", rname);
	nvlist_add_bool(rule_nvl, "nat-ruleset", natset);
	nvlist_add_number(rule_nvl, "command", NPF_CMD_RULE_ADD);

	error = _npf_xfer_fd(fd, IOC_NPF_RULE, rule_nvl, &resp);
	if (error) {
		return error;
	}
	*id = nvlist_get_number(resp, "id");
	nvlist_destroy(resp);
	return 0;
}

int
npf_ruleset_remove(int fd, const char *rname, uint64_t id)
{
	const bool natset = _npf_nat_ruleset_p(rname);
	nvlist_t *rule_nvl = nvlist_create(0);
	int error;

	nvlist_add_string(rule_nvl, "ruleset-name", rname);
	nvlist_add_bool(rule_nvl, "nat-ruleset", natset);
	nvlist_add_number(rule_nvl, "command", NPF_CMD_RULE_REMOVE);
	nvlist_add_number(rule_nvl, "id", id);

	error = _npf_xfer_fd_errno(fd, IOC_NPF_RULE, rule_nvl);
	nvlist_destroy(rule_nvl);
	return error;
}

int
npf_ruleset_remkey(int fd, const char *rname, const void *key, size_t len)
{
	const bool natset = _npf_nat_ruleset_p(rname);
	nvlist_t *rule_nvl = nvlist_create(0);
	int error;

	nvlist_add_string(rule_nvl, "ruleset-name", rname);
	nvlist_add_bool(rule_nvl, "nat-ruleset", natset);
	nvlist_add_number(rule_nvl, "command", NPF_CMD_RULE_REMKEY);
	nvlist_add_binary(rule_nvl, "key", key, len);

	error = _npf_xfer_fd_errno(fd, IOC_NPF_RULE, rule_nvl);
	nvlist_destroy(rule_nvl);
	return error;
}

int
npf_ruleset_flush(int fd, const char *rname)
{
	const bool natset = _npf_nat_ruleset_p(rname);
	nvlist_t *rule_nvl = nvlist_create(0);
	int error;

	nvlist_add_string(rule_nvl, "ruleset-name", rname);
	nvlist_add_bool(rule_nvl, "nat-ruleset", natset);
	nvlist_add_number(rule_nvl, "command", NPF_CMD_RULE_FLUSH);

	error = _npf_xfer_fd_errno(fd, IOC_NPF_RULE, rule_nvl);
	nvlist_destroy(rule_nvl);
	return error;
}

/*
 * NPF EXTENSION INTERFACE.
 */

nl_ext_t *
npf_ext_construct(const char *name)
{
	nl_ext_t *ext;

	ext = malloc(sizeof(*ext));
	if (!ext) {
		return NULL;
	}
	ext->ext_dict = nvlist_create(0);
	nvlist_add_string(ext->ext_dict, "name", name);
	return ext;
}

void
npf_ext_param_u32(nl_ext_t *ext, const char *key, uint32_t val)
{
	nvlist_add_number(ext->ext_dict, key, val);
}

void
npf_ext_param_bool(nl_ext_t *ext, const char *key, bool val)
{
	nvlist_add_bool(ext->ext_dict, key, val);
}

void
npf_ext_param_string(nl_ext_t *ext, const char *key, const char *val)
{
	nvlist_add_string(ext->ext_dict, key, val);
}

/*
 * RULE INTERFACE.
 */

nl_rule_t *
npf_rule_create(const char *name, uint32_t attr, const char *ifname)
{
	nl_rule_t *rl;

	rl = malloc(sizeof(nl_rule_t));
	if (!rl) {
		return NULL;
	}
	rl->rule_dict = nvlist_create(0);
	nvlist_add_number(rl->rule_dict, "attr", attr);
	if (name) {
		nvlist_add_string(rl->rule_dict, "name", name);
	}
	if (ifname) {
		nvlist_add_string(rl->rule_dict, "ifname", ifname);
	}
	return rl;
}

int
npf_rule_setcode(nl_rule_t *rl, int type, const void *code, size_t len)
{
	if (type != NPF_CODE_BPF) {
		return ENOTSUP;
	}
	nvlist_add_number(rl->rule_dict, "code-type", (unsigned)type);
	nvlist_add_binary(rl->rule_dict, "code", code, len);
	return nvlist_error(rl->rule_dict);
}

int
npf_rule_setkey(nl_rule_t *rl, const void *key, size_t len)
{
	nvlist_add_binary(rl->rule_dict, "key", key, len);
	return nvlist_error(rl->rule_dict);
}

int
npf_rule_setinfo(nl_rule_t *rl, const void *info, size_t len)
{
	nvlist_add_binary(rl->rule_dict, "info", info, len);
	return nvlist_error(rl->rule_dict);
}

int
npf_rule_setprio(nl_rule_t *rl, int pri)
{
	nvlist_add_number(rl->rule_dict, "prio", (uint64_t)pri);
	return nvlist_error(rl->rule_dict);
}

int
npf_rule_setproc(nl_rule_t *rl, const char *name)
{
	nvlist_add_string(rl->rule_dict, "rproc", name);
	return nvlist_error(rl->rule_dict);
}

void *
npf_rule_export(nl_rule_t *rl, size_t *length)
{
	return nvlist_pack(rl->rule_dict, length);
}

bool
npf_rule_exists_p(nl_config_t *ncf, const char *name)
{
	const char *key = nvlist_exists_nvlist_array(ncf->ncf_dict,
	    "rules") ? "rules" : "__rules"; // config may not be built yet
	return _npf_dataset_lookup(ncf->ncf_dict, key, "name", name);
}

int
npf_rule_insert(nl_config_t *ncf, nl_rule_t *parent, nl_rule_t *rl)
{
	nvlist_t *rule_dict = rl->rule_dict;
	nvlist_t *target;
	const char *key;

	if (parent) {
		/* Subrule of the parent. */
		target = parent->rule_dict;
		key = "subrules";
	} else {
		/* Global ruleset. */
		target = ncf->ncf_dict;
		key = "__rules";
	}
	nvlist_append_nvlist_array(target, key, rule_dict);
	nvlist_destroy(rule_dict);
	free(rl);
	return 0;
}

static nl_rule_t *
_npf_rule_iterate1(nl_config_t *ncf, const char *key,
    nl_iter_t *iter, unsigned *level)
{
	unsigned i = *iter;
	const nvlist_t *rule_dict;
	uint32_t skipto;

	if (i == 0) {
		/* Initialise the iterator. */
		ncf->ncf_nlevel = 0;
		ncf->ncf_reduce[0] = 0;
	}

	rule_dict = _npf_dataset_getelement(ncf->ncf_dict, key, i);
	if (!rule_dict) {
		*iter = NPF_ITER_BEGIN;
		return NULL;
	}
	*iter = i + 1; // next
	*level = ncf->ncf_nlevel;

	skipto = dnvlist_get_number(rule_dict, "skip-to", 0);
	if (skipto) {
		ncf->ncf_nlevel++;
		ncf->ncf_reduce[ncf->ncf_nlevel] = skipto;
	}
	if (ncf->ncf_reduce[ncf->ncf_nlevel] == (i + 1)) {
		assert(ncf->ncf_nlevel > 0);
		ncf->ncf_nlevel--;
	}

	ncf->ncf_cur_rule.rule_dict = __UNCONST(rule_dict); // XXX
	return &ncf->ncf_cur_rule;
}

nl_rule_t *
npf_rule_iterate(nl_config_t *ncf, nl_iter_t *iter, unsigned *level)
{
	return _npf_rule_iterate1(ncf, "rules", iter, level);
}

const char *
npf_rule_getname(nl_rule_t *rl)
{
	return dnvlist_get_string(rl->rule_dict, "name", NULL);
}

uint32_t
npf_rule_getattr(nl_rule_t *rl)
{
	return dnvlist_get_number(rl->rule_dict, "attr", 0);
}

const char *
npf_rule_getinterface(nl_rule_t *rl)
{
	return dnvlist_get_string(rl->rule_dict, "ifname", NULL);
}

const void *
npf_rule_getinfo(nl_rule_t *rl, size_t *len)
{
	return dnvlist_get_binary(rl->rule_dict, "info", len, NULL, 0);
}

const char *
npf_rule_getproc(nl_rule_t *rl)
{
	return dnvlist_get_string(rl->rule_dict, "rproc", NULL);
}

uint64_t
npf_rule_getid(nl_rule_t *rl)
{
	return dnvlist_get_number(rl->rule_dict, "id", 0);
}

const void *
npf_rule_getcode(nl_rule_t *rl, int *type, size_t *len)
{
	*type = (int)dnvlist_get_number(rl->rule_dict, "code-type", 0);
	return dnvlist_get_binary(rl->rule_dict, "code", len, NULL, 0);
}

int
_npf_ruleset_list(int fd, const char *rname, nl_config_t *ncf)
{
	const bool natset = _npf_nat_ruleset_p(rname);
	nvlist_t *req, *resp;
	int error;

	req = nvlist_create(0);
	nvlist_add_string(req, "ruleset-name", rname);
	nvlist_add_bool(req, "nat-ruleset", natset);
	nvlist_add_number(req, "command", NPF_CMD_RULE_LIST);

	error = _npf_xfer_fd(fd, IOC_NPF_RULE, req, &resp);
	nvlist_destroy(req);
	if (error) {
		return error;
	}

	if (nvlist_exists_nvlist_array(resp, "rules")) {
		nvlist_t **rules;
		size_t n;

		rules = nvlist_take_nvlist_array(resp, "rules", &n);
		nvlist_move_nvlist_array(ncf->ncf_dict, "rules", rules, n);
	}
	nvlist_destroy(resp);
	return 0;
}

void
npf_rule_destroy(nl_rule_t *rl)
{
	nvlist_destroy(rl->rule_dict);
	free(rl);
}

/*
 * RULE PROCEDURE INTERFACE.
 */

nl_rproc_t *
npf_rproc_create(const char *name)
{
	nl_rproc_t *rp;

	rp = malloc(sizeof(nl_rproc_t));
	if (!rp) {
		return NULL;
	}
	rp->rproc_dict = nvlist_create(0);
	nvlist_add_string(rp->rproc_dict, "name", name);
	return rp;
}

int
npf_rproc_extcall(nl_rproc_t *rp, nl_ext_t *ext)
{
	nvlist_t *rproc_dict = rp->rproc_dict;
	const char *name = dnvlist_get_string(ext->ext_dict, "name", NULL);

	if (_npf_dataset_lookup(rproc_dict, "extcalls", "name", name)) {
		return EEXIST;
	}
	nvlist_append_nvlist_array(rproc_dict, "extcalls", ext->ext_dict);
	nvlist_destroy(ext->ext_dict);
	free(ext);
	return 0;
}

bool
npf_rproc_exists_p(nl_config_t *ncf, const char *name)
{
	return _npf_dataset_lookup(ncf->ncf_dict, "rprocs", "name", name);
}

int
npf_rproc_insert(nl_config_t *ncf, nl_rproc_t *rp)
{
	const char *name;

	name = dnvlist_get_string(rp->rproc_dict, "name", NULL);
	if (!name) {
		return EINVAL;
	}
	if (npf_rproc_exists_p(ncf, name)) {
		return EEXIST;
	}
	nvlist_append_nvlist_array(ncf->ncf_dict, "rprocs", rp->rproc_dict);
	nvlist_destroy(rp->rproc_dict);
	free(rp);
	return 0;
}

nl_rproc_t *
npf_rproc_iterate(nl_config_t *ncf, nl_iter_t *iter)
{
	const nvlist_t *rproc_dict;
	unsigned i = *iter;

	rproc_dict = _npf_dataset_getelement(ncf->ncf_dict, "rprocs", i);
	if (!rproc_dict) {
		*iter = NPF_ITER_BEGIN;
		return NULL;
	}
	*iter = i + 1; // next
	ncf->ncf_cur_rproc.rproc_dict = __UNCONST(rproc_dict); // XXX
	return &ncf->ncf_cur_rproc;
}

const char *
npf_rproc_getname(nl_rproc_t *rp)
{
	return dnvlist_get_string(rp->rproc_dict, "name", NULL);
}

/*
 * NAT INTERFACE.
 */

nl_nat_t *
npf_nat_create(int type, unsigned flags, const char *ifname)
{
	nl_rule_t *rl;
	nvlist_t *rule_dict;
	uint32_t attr;

	attr = NPF_RULE_PASS | NPF_RULE_FINAL |
	    (type == NPF_NATOUT ? NPF_RULE_OUT : NPF_RULE_IN);

	/* Create a rule for NAT policy.  Next, will add NAT data. */
	rl = npf_rule_create(NULL, attr, ifname);
	if (!rl) {
		return NULL;
	}
	rule_dict = rl->rule_dict;

	/* Translation type and flags. */
	nvlist_add_number(rule_dict, "type", type);
	nvlist_add_number(rule_dict, "flags", flags);
	nvlist_add_bool(rule_dict, "nat-rule", true);
	return (nl_nat_t *)rl;
}

int
npf_nat_insert(nl_config_t *ncf, nl_nat_t *nt)
{
	nvlist_append_nvlist_array(ncf->ncf_dict, "nat", nt->rule_dict);
	nvlist_destroy(nt->rule_dict);
	free(nt);
	return 0;
}

nl_nat_t *
npf_nat_iterate(nl_config_t *ncf, nl_iter_t *iter)
{
	unsigned level;
	return _npf_rule_iterate1(ncf, "nat", iter, &level);
}

int
npf_nat_setaddr(nl_nat_t *nt, int af, npf_addr_t *addr, npf_netmask_t mask)
{
	/* Translation IP and mask. */
	if (!_npf_add_addr(nt->rule_dict, "nat-addr", af, addr)) {
		return nvlist_error(nt->rule_dict);
	}
	nvlist_add_number(nt->rule_dict, "nat-mask", (uint32_t)mask);
	return nvlist_error(nt->rule_dict);
}

int
npf_nat_setport(nl_nat_t *nt, in_port_t port)
{
	/* Translation port (for redirect case). */
	nvlist_add_number(nt->rule_dict, "nat-port", port);
	return nvlist_error(nt->rule_dict);
}

int
npf_nat_settable(nl_nat_t *nt, unsigned tid)
{
	/*
	 * Translation table ID; the address/mask will then serve as a filter.
	 */
	nvlist_add_number(nt->rule_dict, "nat-table-id", tid);
	return nvlist_error(nt->rule_dict);
}

int
npf_nat_setalgo(nl_nat_t *nt, unsigned algo)
{
	nvlist_add_number(nt->rule_dict, "nat-algo", algo);
	return nvlist_error(nt->rule_dict);
}

int
npf_nat_setnpt66(nl_nat_t *nt, uint16_t adj)
{
	int error;

	if ((error = npf_nat_setalgo(nt, NPF_ALGO_NPT66)) != 0) {
		return error;
	}
	nvlist_add_number(nt->rule_dict, "npt66-adj", adj);
	return nvlist_error(nt->rule_dict);
}

int
npf_nat_gettype(nl_nat_t *nt)
{
	return dnvlist_get_number(nt->rule_dict, "type", 0);
}

unsigned
npf_nat_getflags(nl_nat_t *nt)
{
	return dnvlist_get_number(nt->rule_dict, "flags", 0);
}

unsigned
npf_nat_getalgo(nl_nat_t *nt)
{
	return dnvlist_get_number(nt->rule_dict, "nat-algo", 0);
}

const npf_addr_t *
npf_nat_getaddr(nl_nat_t *nt, size_t *alen, npf_netmask_t *mask)
{
	const void *data;

	if (nvlist_exists(nt->rule_dict, "nat-addr")) {
		data = nvlist_get_binary(nt->rule_dict, "nat-addr", alen);
		*mask = nvlist_get_number(nt->rule_dict, "nat-mask");
	} else {
		data = NULL;
		*alen = 0;
		*mask = NPF_NO_NETMASK;
	}
	return data;
}

in_port_t
npf_nat_getport(nl_nat_t *nt)
{
	return (uint16_t)dnvlist_get_number(nt->rule_dict, "nat-port", 0);
}

unsigned
npf_nat_gettable(nl_nat_t *nt)
{
	return dnvlist_get_number(nt->rule_dict, "nat-table-id", 0);
}

/*
 * TABLE INTERFACE.
 */

nl_table_t *
npf_table_create(const char *name, unsigned id, int type)
{
	nl_table_t *tl;

	tl = malloc(sizeof(*tl));
	if (!tl) {
		return NULL;
	}
	tl->table_dict = nvlist_create(0);
	nvlist_add_string(tl->table_dict, "name", name);
	nvlist_add_number(tl->table_dict, "id", id);
	nvlist_add_number(tl->table_dict, "type", type);
	return tl;
}

int
npf_table_add_entry(nl_table_t *tl, int af, const npf_addr_t *addr,
    const npf_netmask_t mask)
{
	nvlist_t *entry;

	entry = nvlist_create(0);
	if (!entry) {
		return ENOMEM;
	}
	if (!_npf_add_addr(entry, "addr", af, addr)) {
		nvlist_destroy(entry);
		return EINVAL;
	}
	nvlist_add_number(entry, "mask", mask);
	nvlist_append_nvlist_array(tl->table_dict, "entries", entry);
	nvlist_destroy(entry);
	return 0;
}

static inline int
_npf_table_build_const(nl_table_t *tl)
{
	struct cdbw *cdbw;
	const nvlist_t * const *entries;
	int error = 0, fd = -1;
	size_t nitems, len;
	void *cdb, *buf;
	struct stat sb;
	char sfn[32];

	if (dnvlist_get_number(tl->table_dict, "type", 0) != NPF_TABLE_CONST) {
		return 0;
	}

	if (!nvlist_exists_nvlist_array(tl->table_dict, "entries")) {
		return 0;
	}

	/*
	 * Create a constant database and put all the entries.
	 */
	if ((cdbw = cdbw_open()) == NULL) {
		return errno;
	}
	entries = nvlist_get_nvlist_array(tl->table_dict, "entries", &nitems);
	for (unsigned i = 0; i < nitems; i++) {
		const nvlist_t *entry = entries[i];
		const npf_addr_t *addr;
		size_t alen;

		addr = dnvlist_get_binary(entry, "addr", &alen, NULL, 0);
		if (addr == NULL || alen == 0 || alen > sizeof(npf_addr_t)) {
			error = EINVAL;
			goto out;
		}
		if (cdbw_put(cdbw, addr, alen, addr, alen) == -1) {
			error = errno;
			goto out;
		}
	}

	/*
	 * Write the constant database into a temporary file.
	 */
	strncpy(sfn, "/tmp/npfcdb.XXXXXX", sizeof(sfn));
	sfn[sizeof(sfn) - 1] = '\0';

	if ((fd = mkstemp(sfn)) == -1) {
		error = errno;
		goto out;
	}
	unlink(sfn);

	if (cdbw_output(cdbw, fd, "npf-table-cdb", NULL) == -1) {
		error = errno;
		goto out;
	}
	if (fstat(fd, &sb) == -1) {
		error = errno;
		goto out;
	}
	len = sb.st_size;

	/*
	 * Memory-map the database and copy it into a buffer.
	 */
	buf = malloc(len);
	if (!buf) {
		error = ENOMEM;
		goto out;
	}
	cdb = mmap(NULL, len, PROT_READ, MAP_FILE | MAP_PRIVATE, fd, 0);
	if (cdb == MAP_FAILED) {
		error = errno;
		free(buf);
		goto out;
	}
	munmap(cdb, len);

	/*
	 * Move the data buffer to the nvlist.
	 */
	nvlist_move_binary(tl->table_dict, "data", buf, len);
	error = nvlist_error(tl->table_dict);
out:
	if (fd != -1) {
		close(fd);
	}
	cdbw_close(cdbw);
	return error;
}

int
npf_table_insert(nl_config_t *ncf, nl_table_t *tl)
{
	const char *name;
	int error;

	name = dnvlist_get_string(tl->table_dict, "name", NULL);
	if (!name) {
		return EINVAL;
	}
	if (_npf_dataset_lookup(ncf->ncf_dict, "tables", "name", name)) {
		return EEXIST;
	}
	if ((error = _npf_table_build_const(tl)) != 0) {
		return error;
	}
	nvlist_append_nvlist_array(ncf->ncf_dict, "tables", tl->table_dict);
	nvlist_destroy(tl->table_dict);
	free(tl);
	return 0;
}

int
npf_table_replace(int fd, nl_table_t *tl, npf_error_t *errinfo)
{
	nvlist_t *resp = NULL;
	int error;

	/* Ensure const tables are built. */
	if ((error = _npf_table_build_const(tl)) != 0) {
		return _npf_init_error(errno, errinfo);
	}
	error = _npf_xfer_fd(fd, IOC_NPF_TABLE_REPLACE, tl->table_dict, &resp);
	if (error) {
		assert(resp == NULL);
		return _npf_init_error(errno, errinfo);
	}
	error = _npf_extract_error(resp, errinfo);
	nvlist_destroy(resp);
	return error;
}

nl_table_t *
npf_table_iterate(nl_config_t *ncf, nl_iter_t *iter)
{
	const nvlist_t *table_dict;
	unsigned i = *iter;

	table_dict = _npf_dataset_getelement(ncf->ncf_dict, "tables", i);
	if (!table_dict) {
		*iter = NPF_ITER_BEGIN;
		return NULL;
	}
	*iter = i + 1; // next
	ncf->ncf_cur_table.table_dict = __UNCONST(table_dict); // XXX
	return &ncf->ncf_cur_table;
}

unsigned
npf_table_getid(nl_table_t *tl)
{
	return dnvlist_get_number(tl->table_dict, "id", (unsigned)-1);
}

const char *
npf_table_getname(nl_table_t *tl)
{
	return dnvlist_get_string(tl->table_dict, "name", NULL);
}

int
npf_table_gettype(nl_table_t *tl)
{
	return dnvlist_get_number(tl->table_dict, "type", 0);
}

void
npf_table_destroy(nl_table_t *tl)
{
	nvlist_destroy(tl->table_dict);
	free(tl);
}

/*
 * ALG INTERFACE.
 */

int
npf_alg_load(nl_config_t *ncf, const char *name)
{
	nvlist_t *alg_dict;

	if (_npf_dataset_lookup(ncf->ncf_dict, "algs", "name", name)) {
		return EEXIST;
	}
	alg_dict = nvlist_create(0);
	nvlist_add_string(alg_dict, "name", name);
	nvlist_append_nvlist_array(ncf->ncf_dict, "algs", alg_dict);
	nvlist_destroy(alg_dict);
	return 0;
}

/*
 * CONNECTION / NAT ENTRY INTERFACE.
 */

typedef struct {
	unsigned	alen;
	unsigned	proto;
	npf_addr_t	addr[3];
	in_port_t	port[3];
} npf_connpoint_t;

static int
_npf_conn_lookup(int fd, const int af, npf_addr_t *addr[2], in_port_t port[2],
    unsigned proto, const char *ifname, unsigned di)
{
	nvlist_t *req = NULL, *resp = NULL, *key_nv;
	const nvlist_t *nat;
	int error = EINVAL;

	/*
	 * Setup the connection lookup key.
	 */
	if ((key_nv = nvlist_create(0)) == NULL) {
		return ENOMEM;
	}
	if (!_npf_add_addr(key_nv, "saddr", af, addr[0])) {
		nvlist_destroy(key_nv);
		goto out;
	}
	if (!_npf_add_addr(key_nv, "daddr", af, addr[1])) {
		nvlist_destroy(key_nv);
		goto out;
	}
	nvlist_add_number(key_nv, "sport", htons(port[0]));
	nvlist_add_number(key_nv, "dport", htons(port[1]));
	nvlist_add_number(key_nv, "proto", proto);
	if (ifname) {
		nvlist_add_string(key_nv, "ifname", ifname);
	}
	if (di) {
		nvlist_add_number(key_nv, "di", di);
	}

	/*
	 * Setup the request.
	 */
	if ((req = nvlist_create(0)) == NULL) {
		error = ENOMEM;
		goto out;
	}
	nvlist_move_nvlist(req, "key", key_nv);

	/* Lookup: retrieve the connection entry. */
	error = _npf_xfer_fd(fd, IOC_NPF_CONN_LOOKUP, req, &resp);
	if (error) {
		goto out;
	}

	/*
	 * Get the NAT entry and extract the translated pair.
	 */
	if ((nat = dnvlist_get_nvlist(resp, "nat", NULL)) == NULL) {
		error = ENOENT;
		goto out;
	}
	if (_npf_get_addr(nat, "oaddr", addr[0]) == 0 ||
	    _npf_get_addr(nat, "taddr", addr[1]) == 0) {
		error = EINVAL;
		goto out;
	}
	port[0] = ntohs(nvlist_get_number(nat, "oport"));
	port[1] = ntohs(nvlist_get_number(nat, "tport"));
out:
	if (resp) {
		nvlist_destroy(resp);
	}
	if (req) {
		nvlist_destroy(req);
	}
	return error;
}

int
npf_nat_lookup(int fd, int af, npf_addr_t *addr[2], in_port_t port[2],
    int proto, int di __unused)
{
	int error;

	port[0] = ntohs(port[0]); port[1] = ntohs(port[1]);
	error = _npf_conn_lookup(fd, af, addr, port, proto, NULL, 0);
	port[0] = htons(port[0]); port[1] = htons(port[1]);
	return error;
}

static bool
npf_connkey_handle(const nvlist_t *key_nv, npf_connpoint_t *ep)
{
	unsigned alen1, alen2;

	alen1 = _npf_get_addr(key_nv, "saddr", &ep->addr[0]);
	alen2 = _npf_get_addr(key_nv, "daddr", &ep->addr[1]);
	if (alen1 == 0 || alen1 != alen2) {
		return false;
	}
	ep->alen = alen1;
	ep->port[0] = ntohs(nvlist_get_number(key_nv, "sport"));
	ep->port[1] = ntohs(nvlist_get_number(key_nv, "dport"));
	ep->proto = nvlist_get_number(key_nv, "proto");
	return true;
}

static void
npf_conn_handle(const nvlist_t *conn, npf_conn_func_t func, void *arg)
{
	const nvlist_t *key_nv, *nat_nv;
	const char *ifname;
	npf_connpoint_t ep;

	memset(&ep, 0, sizeof(npf_connpoint_t));

	ifname = dnvlist_get_string(conn, "ifname", NULL);
	key_nv = dnvlist_get_nvlist(conn, "forw-key", NULL);
	if (!npf_connkey_handle(key_nv, &ep)) {
		goto err;
	}
	if ((nat_nv = dnvlist_get_nvlist(conn, "nat", NULL)) != NULL) {
		if (_npf_get_addr(nat_nv, "taddr", &ep.addr[2]) != ep.alen) {
			goto err;
		}
		ep.port[2] = ntohs(nvlist_get_number(nat_nv, "tport"));
	}
	/*
	 * XXX: add 'proto' and 'flow'; perhaps expand and pass the
	 * whole to npf_connpoint_t?
	 */
	(*func)((unsigned)ep.alen, ep.addr, ep.port, ifname, arg);
err:
	return;
}

int
npf_conn_list(int fd, npf_conn_func_t func, void *arg)
{
	nl_config_t *ncf;
	const nvlist_t * const *conns;
	size_t nitems;

	ncf = npf_config_retrieve(fd);
	if (!ncf) {
		return errno;
	}
	if (!nvlist_exists_nvlist_array(ncf->ncf_dict, "conn-list")) {
		return 0;
	}
	conns = nvlist_get_nvlist_array(ncf->ncf_dict, "conn-list", &nitems);
	for (unsigned i = 0; i < nitems; i++) {
		const nvlist_t *conn = conns[i];
		npf_conn_handle(conn, func, arg);
	}
	npf_config_destroy(ncf);
	return 0;
}

/*
 * MISC.
 */

void
_npf_debug_addif(nl_config_t *ncf, const char *ifname)
{
	nvlist_t *debug;

	/*
	 * Initialise the debug dictionary on the first call.
	 */
	debug = dnvlist_take_nvlist(ncf->ncf_dict, "debug", NULL);
	if (debug == NULL) {
		debug = nvlist_create(0);
	}
	if (!_npf_dataset_lookup(debug, "interfaces", "name", ifname)) {
		nvlist_t *ifdict = nvlist_create(0);
		nvlist_add_string(ifdict, "name", ifname);
		nvlist_add_number(ifdict, "index", if_nametoindex(ifname));
		nvlist_append_nvlist_array(debug, "interfaces", ifdict);
		nvlist_destroy(ifdict);
	}
	nvlist_move_nvlist(ncf->ncf_dict, "debug", debug);
}

void
_npf_config_dump(nl_config_t *ncf, int fd)
{
	(void)npf_config_build(ncf);
	nvlist_dump(ncf->ncf_dict, fd);
}
