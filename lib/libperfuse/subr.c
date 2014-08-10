/*  $NetBSD: subr.c,v 1.20 2014/08/10 03:22:33 manu Exp $ */

/*-
 *  Copyright (c) 2010-2011 Emmanuel Dreyfus. All rights reserved.
 * 
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 
 *  THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 *  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 *  TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *  PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */ 

#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#include <errno.h>
#include <sysexits.h>
#include <syslog.h>
#include <puffs.h>
#include <paths.h>
#include <sys/hash.h>
#include <sys/extattr.h>

#include "perfuse_priv.h"

struct perfuse_ns_map {
	const char *pnm_ns;
	const size_t pnm_nslen;
	const int pnm_native_ns;
};

#define PERFUSE_NS_MAP(ns, native_ns)	\
	{ ns ".", sizeof(ns), native_ns }

static __inline struct perfuse_node_hashlist *perfuse_nidhash(struct 
    perfuse_state *, uint64_t);

struct puffs_node *
perfuse_new_pn(struct puffs_usermount *pu, const char *name,
	struct puffs_node *parent)
{
	struct perfuse_state *ps = puffs_getspecific(pu);
	struct puffs_node *pn;
	struct perfuse_node_data *pnd;

	if ((pnd = malloc(sizeof(*pnd))) == NULL)
		DERR(EX_OSERR, "%s: malloc failed", __func__);

	if ((pn = puffs_pn_new(pu, pnd)) == NULL)
		DERR(EX_SOFTWARE, "%s: puffs_pn_new failed", __func__);

	(void)memset(pnd, 0, sizeof(*pnd));
	pnd->pnd_rfh = FUSE_UNKNOWN_FH;
	pnd->pnd_wfh = FUSE_UNKNOWN_FH;
	pnd->pnd_nodeid = PERFUSE_UNKNOWN_NODEID;
	if (parent != NULL) {
		pnd->pnd_parent_nodeid = PERFUSE_NODE_DATA(parent)->pnd_nodeid;
	} else {
		pnd->pnd_parent_nodeid = FUSE_ROOT_ID;
	}
	pnd->pnd_fuse_nlookup = 0;
	pnd->pnd_puffs_nlookup = 0;
	pnd->pnd_pn = (puffs_cookie_t)pn;
	if (strcmp(name, "..") != 0)
		(void)strlcpy(pnd->pnd_name, name, MAXPATHLEN);
	else
		pnd->pnd_name[0] = 0; /* anonymous for now */
	TAILQ_INIT(&pnd->pnd_pcq);

	puffs_pn_setpriv(pn, pnd);

	ps->ps_nodecount++;

	return pn;
}

void
perfuse_destroy_pn(struct puffs_usermount *pu, struct puffs_node *pn)
{
	struct perfuse_state *ps = puffs_getspecific(pu);
	struct perfuse_node_data *pnd;

	if ((pnd = puffs_pn_getpriv(pn)) != NULL) {
		if (pnd->pnd_all_fd != NULL)
			free(pnd->pnd_all_fd);

		if (pnd->pnd_dirent != NULL)
			free(pnd->pnd_dirent);
		
#ifdef PERFUSE_DEBUG
		if (pnd->pnd_flags & PND_OPEN)
			DERRX(EX_SOFTWARE, "%s: file open", __func__);

		if (!TAILQ_EMPTY(&pnd->pnd_pcq))
			DERRX(EX_SOFTWARE, "%s: non empty pnd_pcq", __func__);

		pnd->pnd_flags |= PND_INVALID;
#endif /* PERFUSE_DEBUG */

		free(pnd);
	}

	puffs_pn_put(pn);

	ps->ps_nodecount--;

	return;
}

void
perfuse_new_fh(puffs_cookie_t opc, uint64_t fh, int mode)
{
	struct perfuse_node_data *pnd;

	pnd = PERFUSE_NODE_DATA(opc);

	if (mode & FWRITE) {
		if (pnd->pnd_flags & PND_WFH)
			DERRX(EX_SOFTWARE, "%s: opc = %p, write fh already set",
			      __func__, (void *)opc);	
		pnd->pnd_wfh = fh;
		pnd->pnd_flags |= PND_WFH;
	} 

	if (mode & FREAD) {
		if (pnd->pnd_flags & PND_RFH)
			DERRX(EX_SOFTWARE, "%s: opc = %p, read fh already set",
			      __func__, (void *)opc);	
		pnd->pnd_rfh = fh;
		pnd->pnd_flags |= PND_RFH;
	}

	return;
}

void
perfuse_destroy_fh(puffs_cookie_t opc, uint64_t fh)
{
	struct perfuse_node_data *pnd;

	pnd = PERFUSE_NODE_DATA(opc);

	if (fh == pnd->pnd_rfh) {
		if (!(pnd->pnd_flags & PND_RFH) && (fh != FUSE_UNKNOWN_FH))
			DERRX(EX_SOFTWARE, 
			      "%s: opc = %p, unset rfh = %"PRIx64"",
			      __func__, (void *)opc, fh);	
		pnd->pnd_rfh = FUSE_UNKNOWN_FH;
		pnd->pnd_flags &= ~PND_RFH;
	}

	if (fh == pnd->pnd_wfh) {
		if (!(pnd->pnd_flags & PND_WFH) && (fh != FUSE_UNKNOWN_FH))
			DERRX(EX_SOFTWARE,
			      "%s: opc = %p, unset wfh = %"PRIx64"",
			      __func__, (void *)opc, fh);	
		pnd->pnd_wfh = FUSE_UNKNOWN_FH;
		pnd->pnd_flags &= ~PND_WFH;
	} 

	return;
}

uint64_t
perfuse_get_fh(puffs_cookie_t opc, int mode)
{
	struct perfuse_node_data *pnd;

	pnd = PERFUSE_NODE_DATA(opc);

	if (mode & FWRITE) {
		if (pnd->pnd_flags & PND_WFH)
			return pnd->pnd_wfh;
	}

	if (mode & FREAD) {
		if (pnd->pnd_flags & PND_RFH)
			return pnd->pnd_rfh;

		if (pnd->pnd_flags & PND_WFH)
			return pnd->pnd_wfh;

	}

	return FUSE_UNKNOWN_FH;
}

/* ARGSUSED0 */
char *
perfuse_node_path(struct perfuse_state *ps, puffs_cookie_t opc)
{
	static char buf[MAXPATHLEN + 1];

#if 0
	if (node_path(ps, opc, buf, sizeof(buf)) == 0)
		sprintf(buf, "/");
#endif
	sprintf(buf, "%s", PERFUSE_NODE_DATA(opc)->pnd_name);

	return buf;
}

int
perfuse_ns_match(const int attrnamespace, const char *attrname)
{
	const char *system_ns[] = { "system.", "trusted.", "security", NULL };
	int i;

        for (i = 0; system_ns[i]; i++) {
                if (strncmp(attrname, system_ns[i], strlen(system_ns[i])) == 0)
			return (attrnamespace == EXTATTR_NAMESPACE_SYSTEM);
        }

	return (attrnamespace == EXTATTR_NAMESPACE_USER);
}

const char *
perfuse_native_ns(const int attrnamespace, const char *attrname,
	char *fuse_attrname)
{
	const struct perfuse_ns_map *pnm;
	const struct perfuse_ns_map perfuse_ns_map[] = {
		PERFUSE_NS_MAP("trusted", EXTATTR_NAMESPACE_SYSTEM),
		PERFUSE_NS_MAP("security", EXTATTR_NAMESPACE_SYSTEM),
		PERFUSE_NS_MAP("system", EXTATTR_NAMESPACE_SYSTEM),
		PERFUSE_NS_MAP("user", EXTATTR_NAMESPACE_USER),
		{ NULL, 0, EXTATTR_NAMESPACE_USER },
	};

	/*
	 * If attribute has a reserved Linux namespace (e.g.: trusted.foo)
	 * and that namespace matches the requested native namespace
	 * we have nothing to do. 
	 * Otherwise we have either:
	 * (system|trusted|security).* with user namespace: prepend user.
	 * anything else with system napespace: prepend system.
	 */
	for (pnm = perfuse_ns_map; pnm->pnm_ns; pnm++) {
		if (strncmp(attrname, pnm->pnm_ns, pnm->pnm_nslen) != 0) 
			continue;

		if (attrnamespace == pnm->pnm_native_ns)
			return attrname;

	 	/* (system|trusted|security).* with user namespace */
		if (attrnamespace == EXTATTR_NAMESPACE_USER) {
			(void)snprintf(fuse_attrname, LINUX_XATTR_NAME_MAX, 
				       "user.%s", attrname);
			return (const char *)fuse_attrname;
		}
	}

	/* anything else with system napespace */
	if (attrnamespace == EXTATTR_NAMESPACE_SYSTEM) {
		(void)snprintf(fuse_attrname, LINUX_XATTR_NAME_MAX, 
			       "system.%s", attrname);
		return (const char *)fuse_attrname;
	}

	return (const char *)attrname;
}

static __inline struct perfuse_node_hashlist *
perfuse_nidhash(struct perfuse_state *ps, uint64_t nodeid)
{       
        uint32_t hash;
        
        hash = hash32_buf(&nodeid, sizeof(nodeid), HASH32_BUF_INIT);

	return &ps->ps_nidhash[hash % ps->ps_nnidhash];
}    

struct perfuse_node_data *
perfuse_node_bynodeid(struct perfuse_state *ps, uint64_t nodeid)
{
	struct perfuse_node_hashlist *plist;
	struct perfuse_node_data *pnd;

	plist = perfuse_nidhash(ps, nodeid);

	LIST_FOREACH(pnd, plist, pnd_nident)
		if (pnd->pnd_nodeid == nodeid)
			break;

	return pnd;
}

void
perfuse_node_cache(struct perfuse_state *ps, puffs_cookie_t opc)
{
	struct perfuse_node_data *pnd = PERFUSE_NODE_DATA(opc);
	struct perfuse_node_hashlist *nidlist;

	if (pnd->pnd_flags & PND_REMOVED)
		DERRX(EX_SOFTWARE, "%s: \"%s\" already removed", 
		      __func__, pnd->pnd_name);

	nidlist = perfuse_nidhash(ps, pnd->pnd_nodeid);
	LIST_INSERT_HEAD(nidlist, pnd, pnd_nident);

	return;
}

void
perfuse_cache_flush(puffs_cookie_t opc)
{
	struct perfuse_node_data *pnd = PERFUSE_NODE_DATA(opc);

	if (pnd->pnd_flags & PND_REMOVED)
		DERRX(EX_SOFTWARE, "%s: \"%s\" already removed", 
		      __func__, pnd->pnd_name);

	LIST_REMOVE(pnd, pnd_nident);

	return;
}
