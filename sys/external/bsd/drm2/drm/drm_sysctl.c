/*	$NetBSD: drm_sysctl.c,v 1.4.2.2 2015/09/22 12:06:05 skrll Exp $	*/

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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
__KERNEL_RCSID(0, "$NetBSD: drm_sysctl.c,v 1.4.2.2 2015/09/22 12:06:05 skrll Exp $");

#include <sys/param.h>
#include <sys/types.h>
#include <sys/systm.h>
#include <sys/sysctl.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
/* We need to specify the type programmatically */
#undef sysctl_createv

#include <drm/drm_sysctl.h>

#ifdef SYSCTL_INCLUDE_DESCR
static const char *
drm_sysctl_get_description(const struct linux_module_param_info *p,
    const struct drm_sysctl_def *def)
{
	const void * const *b = def->bd, * const *e = def->ed;

	for (; b < e; b++) {
		const struct linux_module_param_desc *d = *b;
		if (strcmp(p->dname, d->name) == 0)
			return d->description;
	}
	return NULL;
}
#endif

#ifdef notyet
static uint64_t
drm_sysctl_get_value(const struct linux_module_param_info *p)
{
	switch (p->type) {
	case MTYPE_bool:
		return *(bool *)p->ptr;
	case MTYPE_int:
		return *(int *)p->ptr;
	default:
		aprint_error("unhandled module param type %d for %s\n",
		    p->type, p->name);
		return 0;
	}
}

static size_t
drm_sysctl_get_size(const struct linux_module_param_info *p)
{
	switch (p->type) {
	case MTYPE_bool:
		return sizeof(bool);
	case MTYPE_int:
		return sizeof(int);
	default:
		aprint_error("unhandled module param type %d for %s\n",
		    p->type, p->name);
		return sizeof(void *);
	}
}
#endif

static int
drm_sysctl_get_type(const struct linux_module_param_info *p)
{
	switch (p->type) {
	case MTYPE_bool:
		return CTLTYPE_BOOL;
	case MTYPE_int:
		return CTLTYPE_INT;
	case MTYPE_charp:
		return CTLTYPE_STRING;
	default:
		aprint_error("unhandled module param type %d for %s\n",
		    p->type, p->name);
		return CTLTYPE_NODE;
	}
}


static int
drm_sysctl_node(const char *name, const struct sysctlnode **node,
    struct sysctllog **log)
{
	return sysctl_createv(log, 0, node, node,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE, name, NULL,
	    NULL, 0, NULL, 0, CTL_CREATE, CTL_EOL);
}


void
drm_sysctl_init(struct drm_sysctl_def *def)
{
	const void * const *b = def->bp, * const *e = def->ep;
	const struct sysctlnode *rnode = NULL, *cnode;
	const char *name = "drm2";

	int error;
	if ((error = sysctl_createv(&def->log, 0, NULL, &rnode,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE, name,
	    SYSCTL_DESCR("DRM driver parameters"),
	    NULL, 0, NULL, 0, CTL_HW, CTL_CREATE, CTL_EOL)) != 0) {
		aprint_error("sysctl_createv returned %d, "
		    "for %s ignoring\n", error, name);
		return;
	}

	for (; b < e; b++) {
		const struct linux_module_param_info *p = *b;
		char copy[256], *n, *nn;
		strlcpy(copy, p->name, sizeof(copy));
		cnode = rnode;
		for (n = copy; (nn = strchr(n, '.')) != NULL; n = nn) {
			*nn++ = '\0';
			if ((error = drm_sysctl_node(n, &cnode, &def->log))
			    != 0) {
				aprint_error("sysctl_createv returned %d, "
				    "for %s ignoring\n", error, n);
				continue;
			}
		}

	        if ((error = sysctl_createv(&def->log, 0, &cnode,
		    &cnode, p->mode == 0600 ? CTLFLAG_READWRITE : 0,
		    drm_sysctl_get_type(p), n,
		    SYSCTL_DESCR(drm_sysctl_get_description(p, def)),
		    NULL, 0, p->ptr, 0, CTL_CREATE, CTL_EOL)) != 0)
			aprint_error("sysctl_createv returned %d, "
			    "for %s ignoring\n", error, n);
	}
}

void
drm_sysctl_fini(struct drm_sysctl_def *def)
{
	sysctl_teardown(&def->log);
}
