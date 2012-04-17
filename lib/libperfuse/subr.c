/*  $NetBSD: subr.c,v 1.14.2.1 2012/04/17 00:05:30 yamt Exp $ */

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
#include <sys/extattr.h>

#include "perfuse_priv.h"

struct perfuse_ns_map {
	const char *pnm_ns;
	const size_t pnm_nslen;
	const int pnm_native_ns;
};

#define PERFUSE_NS_MAP(ns, native_ns)	\
	{ ns ".", sizeof(ns), native_ns }

static size_t node_path(puffs_cookie_t, char *, size_t);

struct puffs_node *
perfuse_new_pn(struct puffs_usermount *pu, const char *name,
	struct puffs_node *parent)
{
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
	pnd->pnd_fuse_nlookup = 1;
	pnd->pnd_puffs_nlookup = 1;
	pnd->pnd_parent = parent;
	pnd->pnd_pn = (puffs_cookie_t)pn;
	(void)strlcpy(pnd->pnd_name, name, MAXPATHLEN);
	TAILQ_INIT(&pnd->pnd_pcq);
	TAILQ_INIT(&pnd->pnd_children);

	if (parent != NULL) {
		struct perfuse_node_data *parent_pnd;

		parent_pnd = PERFUSE_NODE_DATA(parent);
		TAILQ_INSERT_TAIL(&parent_pnd->pnd_children, pnd, pnd_next);

		parent_pnd->pnd_childcount++;
	}

	return pn;
}

void
perfuse_destroy_pn(struct puffs_node *pn)
{
	struct perfuse_node_data *pnd;

	pnd = PERFUSE_NODE_DATA(pn);

	if (pnd->pnd_parent != NULL) {
		struct perfuse_node_data *parent_pnd;

		parent_pnd = PERFUSE_NODE_DATA(pnd->pnd_parent);
		TAILQ_REMOVE(&parent_pnd->pnd_children, pnd, pnd_next);
	}

	if ((pnd = puffs_pn_getpriv(pn)) != NULL) {
		if (pnd->pnd_parent != NULL)
			PERFUSE_NODE_DATA(pnd->pnd_parent)->pnd_childcount--;

		if (pnd->pnd_dirent != NULL)
			free(pnd->pnd_dirent);

		if (pnd->pnd_all_fd != NULL)
			free(pnd->pnd_all_fd);
#ifdef PERFUSE_DEBUG
		if (pnd->pnd_flags & PND_OPEN)
			DERRX(EX_SOFTWARE, "%s: file open", __func__);

		if (!TAILQ_EMPTY(&pnd->pnd_pcq))
			DERRX(EX_SOFTWARE, "%s: non empty pnd_pcq", __func__);

		if (pnd == NULL)
			DERRX(EX_SOFTWARE, "%s: pnd == NULL ???", __func__);
#endif /* PERFUSE_DEBUG */

		free(pnd);
	}

	puffs_pn_put(pn);

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

static size_t
node_path(puffs_cookie_t opc, char *buf, size_t buflen)
{
	struct perfuse_node_data *pnd;
	size_t written;
	
	pnd = PERFUSE_NODE_DATA(opc);
	if (pnd->pnd_parent == opc)
		return 0;

	written = node_path(pnd->pnd_parent, buf, buflen);
	buf += written;
	buflen -= written;
	
	return written + snprintf(buf, buflen, "/%s", pnd->pnd_name);
}

char *
perfuse_node_path(puffs_cookie_t opc)
{
	static char buf[MAXPATHLEN + 1];

	if (node_path(opc, buf, sizeof(buf)) == 0)
		sprintf(buf, "/");

	return buf;
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
