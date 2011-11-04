/*	$NetBSD: npf.c,v 1.3 2011/11/04 01:00:28 zoltan Exp $	*/

/*-
 * Copyright (c) 2010-2011 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: npf.c,v 1.3 2011/11/04 01:00:28 zoltan Exp $");

#include <sys/types.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <prop/proplib.h>

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <err.h>

#define	_NPF_PRIVATE
#include "npf.h"

struct nl_config {
	/* Rules, translations, tables, procedures. */
	prop_array_t		ncf_rules_list;
	prop_array_t		ncf_rproc_list;
	prop_array_t		ncf_table_list;
	prop_array_t		ncf_nat_list;
	/* Priority counters. */
	pri_t			ncf_rule_pri;
	pri_t			ncf_nat_pri;
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

/*
 * CONFIGURATION INTERFACE.
 */

nl_config_t *
npf_config_create(void)
{
	nl_config_t *ncf;

	ncf = malloc(sizeof(nl_config_t));
	if (ncf == NULL) {
		return NULL;
	}
	ncf->ncf_rules_list = prop_array_create();
	ncf->ncf_rproc_list = prop_array_create();
	ncf->ncf_table_list = prop_array_create();
	ncf->ncf_nat_list = prop_array_create();

	ncf->ncf_rule_pri = 1;
	ncf->ncf_nat_pri = 1;

	return ncf;
}

int
npf_config_submit(nl_config_t *ncf, int fd)
{
	prop_dictionary_t npf_dict;
	int error = 0;

	npf_dict = prop_dictionary_create();
	if (npf_dict == NULL) {
		return ENOMEM;
	}
	prop_dictionary_set(npf_dict, "rules", ncf->ncf_rules_list);
	prop_dictionary_set(npf_dict, "rprocs", ncf->ncf_rproc_list);
	prop_dictionary_set(npf_dict, "tables", ncf->ncf_table_list);
	prop_dictionary_set(npf_dict, "translation", ncf->ncf_nat_list);

#ifndef _NPF_TESTING
	error = prop_dictionary_send_ioctl(npf_dict, fd, IOC_NPF_RELOAD);
#else
	if (!prop_dictionary_externalize_to_file(npf_dict, "./npf.plist")) {
		error = errno;
	}
#endif
	prop_object_release(npf_dict);
	return error;
}

void
npf_config_destroy(nl_config_t *ncf)
{

	prop_object_release(ncf->ncf_rules_list);
	prop_object_release(ncf->ncf_rproc_list);
	prop_object_release(ncf->ncf_table_list);
	prop_object_release(ncf->ncf_nat_list);
	free(ncf);
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
 * RULE INTERFACE.
 */

nl_rule_t *
npf_rule_create(const char *name, uint32_t attr, u_int if_idx)
{
	prop_dictionary_t rldict;
	nl_rule_t *rl;

	rl = malloc(sizeof(nl_rule_t));
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
npf_rule_setcode(nl_rule_t *rl, int type, const void *code, size_t sz)
{
	prop_dictionary_t rldict = rl->nrl_dict;
	prop_data_t cdata;

	if (type != NPF_CODE_NCODE) {
		return ENOTSUP;
	}
	cdata = prop_data_create_data(code, sz);
	if (cdata == NULL) {
		return ENOMEM;
	}
	prop_dictionary_set(rldict, "ncode", cdata);
	prop_object_release(cdata);
	return 0;
}

int
npf_rule_setproc(nl_config_t *ncf, nl_rule_t *rl, const char *name)
{
	prop_dictionary_t rldict = rl->nrl_dict;

	if (!npf_rproc_exists_p(ncf, name)) {
		return ENOENT;
	}
	prop_dictionary_set_cstring(rldict, "rproc", name);
	return 0;
}

bool
npf_rule_exists_p(nl_config_t *ncf, const char *name)
{

	return _npf_prop_array_lookup(ncf->ncf_rules_list, "name", name);
}

int
npf_rule_insert(nl_config_t *ncf, nl_rule_t *parent, nl_rule_t *rl, pri_t pri)
{
	prop_dictionary_t rldict = rl->nrl_dict;
	prop_array_t rlset;

	if (pri == NPF_PRI_NEXT) {
		pri = ncf->ncf_rule_pri++;
	} else if (ncf) {
		ncf->ncf_rule_pri = pri + 1;
	}
	prop_dictionary_set_int32(rldict, "priority", pri);

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
	nrp->nrp_dict = rpdict;
	return nrp;
}

bool
npf_rproc_exists_p(nl_config_t *ncf, const char *name)
{

	return _npf_prop_array_lookup(ncf->ncf_rproc_list, "name", name);
}

int
_npf_rproc_setnorm(nl_rproc_t *rp, bool rnd, bool no_df, int minttl, int maxmss)
{
	prop_dictionary_t rpdict = rp->nrp_dict;
	uint32_t fl;

	prop_dictionary_set_bool(rpdict, "randomize-id", rnd);
	prop_dictionary_set_bool(rpdict, "no-df", no_df);
	prop_dictionary_set_uint32(rpdict, "min-ttl", minttl);
	prop_dictionary_set_uint32(rpdict, "max-mss", maxmss);

	prop_dictionary_get_uint32(rpdict, "flags", &fl);
	prop_dictionary_set_uint32(rpdict, "flags", fl | NPF_RPROC_NORMALIZE);
	return 0;
}

int
_npf_rproc_setlog(nl_rproc_t *rp, u_int if_idx)
{
	prop_dictionary_t rpdict = rp->nrp_dict;
	uint32_t fl;

	prop_dictionary_set_uint32(rpdict, "log-interface", if_idx);

	prop_dictionary_get_uint32(rpdict, "flags", &fl);
	prop_dictionary_set_uint32(rpdict, "flags", fl | NPF_RPROC_LOG);
	return 0;
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
npf_nat_create(int type, int flags, u_int if_idx,
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

	if (pri == NPF_PRI_NEXT) {
		pri = ncf->ncf_nat_pri++;
	} else {
		ncf->ncf_nat_pri = pri + 1;
	}
	prop_dictionary_set_int32(rldict, "priority", pri);
	prop_array_add(ncf->ncf_nat_list, rldict);
	return 0;
}

/*
 * TABLE INTERFACE.
 */

nl_table_t *
npf_table_create(int id, int type)
{
	prop_dictionary_t tldict;
	prop_array_t tblents;
	nl_table_t *tl;

	tl = malloc(sizeof(nl_table_t));
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
npf_table_add_entry(nl_table_t *tl, npf_addr_t *addr, npf_netmask_t mask)
{
	prop_dictionary_t tldict = tl->ntl_dict, entdict;
	prop_array_t tblents;
	prop_data_t addrdata;

	/* Create the table entry. */
	entdict = prop_dictionary_create();
	if (entdict) {
		return ENOMEM;
	}
	addrdata = prop_data_create_data(addr, sizeof(npf_addr_t));
	prop_dictionary_set(entdict, "addr", addrdata);
	prop_dictionary_set_uint8(entdict, "mask", mask);
	prop_object_release(addrdata);

	/* Insert the entry. */
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

/*
 * MISC.
 */

int
npf_update_rule(int fd, char *rname, nl_rule_t *rl)
{
	prop_dictionary_t rldict = rl->nrl_dict;
	int error;

	error = prop_dictionary_send_ioctl(rldict, fd, IOC_NPF_UPDATE_RULE);
	return error;
}

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
