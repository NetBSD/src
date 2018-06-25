/*	$NetBSD: kstat.c,v 1.2.30.1 2018/06/25 07:25:25 pgoyette Exp $	*/

/*-
 * Copyright (c) 2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
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

/*-
 * Copyright (c) 2007 Pawel Jakub Dawidek <pjd@FreeBSD.org>
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/kmem.h>
#include <sys/sysctl.h>
#include <sys/kstat.h>

kstat_t *
kstat_create(char *module, int instance, char *name, char *class, uchar_t type,
	     ulong_t ndata, uchar_t flags)
{
	struct sysctl_oid *root;
	const struct sysctlnode *rnode, *mnode, *cnode;
	kstat_t *ks;

	KASSERT(instance == 0);
	KASSERT(type == KSTAT_TYPE_NAMED);
	KASSERT(flags == KSTAT_FLAG_VIRTUAL);

	ks = kmem_zalloc(sizeof(*ks), KM_SLEEP);
	ks->ks_ndata = ndata;

	sysctl_createv(&ks->ks_clog, 0, NULL, &rnode,
		CTLFLAG_PERMANENT,
		CTLTYPE_NODE, "kstat", NULL,
		NULL, 0, NULL, 0,
		CTL_CREATE, CTL_EOL);
	sysctl_createv(&ks->ks_clog, 0, &rnode, &mnode,
		CTLFLAG_PERMANENT,
		CTLTYPE_NODE, module, NULL,
		NULL, 0, NULL, 0,
		CTL_CREATE, CTL_EOL);
	sysctl_createv(&ks->ks_clog, 0, &mnode, &cnode,
		CTLFLAG_PERMANENT,
		CTLTYPE_NODE, class, NULL,
		NULL, 0, NULL, 0,
		CTL_CREATE, CTL_EOL);
	sysctl_createv(&ks->ks_clog, 0, &cnode, &ks->ks_node,
		CTLFLAG_PERMANENT,
		CTLTYPE_NODE, name, NULL,
		NULL, 0, NULL, 0,
		CTL_CREATE, CTL_EOL);

	return (ks);
}

void
kstat_install(kstat_t *ks)
{
	kstat_named_t *ksent;
	u_int i;

	ksent = ks->ks_data;

	for (i = 0; i < ks->ks_ndata; i++, ksent++) {
		KASSERT(ksent->data_type == KSTAT_DATA_UINT64);
		sysctl_createv(&ks->ks_clog, 0, &ks->ks_node, NULL,
			CTLFLAG_PERMANENT | CTLFLAG_READONLY,
			CTLTYPE_QUAD, ksent->name, NULL,
			NULL, 0, &ksent->value.ui64, 0,
			CTL_CREATE, CTL_EOL);
	}
}

void
kstat_delete(kstat_t *ks)
{

	sysctl_teardown(&ks->ks_clog);
	kmem_free(ks, sizeof(*ks));
}

void
kstat_set_string(char *dst, const char *src)
{

	memset(dst, 0, KSTAT_STRLEN);
	(void) strncpy(dst, src, KSTAT_STRLEN - 1);
}

void
kstat_named_init(kstat_named_t *knp, const char *name, uchar_t data_type)
{

	kstat_set_string(knp->name, name);
	knp->data_type = data_type;
}
