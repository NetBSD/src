/*	$NetBSD: kern_crashme.c,v 1.2.6.2 2019/06/10 22:09:03 christos Exp $	*/

/*
 * Copyright (c) 2018, 2019 Matthew R. Green
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * kern_crashme.c:  special debugging routines only enabled in DEBUG
 * enabled kernels, designed for debugging kernel crashes.
 *
 * supports crashme sysctl nodes, to test various ways the system can
 * panic or crash.  you can add and remove nodes.
 */

#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/systm.h>
#include <sys/kthread.h>
#include <sys/kmem.h>
#include <sys/mutex.h>
#include <sys/crashme.h>

#define DPRINTF(fmt, ...) \
	printf("%s:%d: " fmt "\n", __func__, __LINE__, ## __VA_ARGS__)

static int crashme_sysctl_forwarder(SYSCTLFN_PROTO);

static int crashme_panic(int);
static int crashme_null_deref(int);

#define CMNODE(name, lname, func)	\
    {					\
	.cn_name = name,		\
	.cn_longname = lname,		\
	.cn_fn = func,			\
    }

static crashme_node nodes[] = {
    CMNODE("panic", "plain old panic", crashme_panic),
    CMNODE("null_deref", "null dereference", crashme_null_deref),
};
static crashme_node *first_node;
static kmutex_t crashme_lock;
static const struct sysctlnode *crashme_root = NULL;
static bool crashme_enable = 0;

/*
 * add a crashme node dynamically.  return -1 on failure, 0 on success.
 */
int
crashme_add(crashme_node *ncn)
{
	int rv = -1;
	crashme_node *cn;
	crashme_node *last = NULL;

	if (crashme_root == NULL)
		return -1;

	mutex_enter(&crashme_lock);
	for (cn = first_node; cn; last = cn, cn = cn->cn_next) {
		if (strcmp(cn->cn_name, ncn->cn_name) == 0)
			break;
	}
	if (!cn) {
		ncn->cn_next = NULL;

		rv = sysctl_createv(NULL, 0,
				    &crashme_root, &ncn->cn_sysctl,
				    CTLFLAG_PERMANENT | CTLFLAG_READWRITE,
				    CTLTYPE_INT, ncn->cn_name,
				    SYSCTL_DESCR(ncn->cn_longname),
				    crashme_sysctl_forwarder, 0,
				    NULL, 0,
				    CTL_CREATE, CTL_EOL);

		/* don't insert upon failure */
		if (rv == 0) {
			if (last)
				last->cn_next = ncn;
			if (first_node == NULL)
				first_node = ncn;
		}
	}
	mutex_exit(&crashme_lock);

	return rv;
}

/*
 * remove a crashme node.  return -1 on failure, 0 on success.
 */
int
crashme_remove(crashme_node *rcn)
{
	crashme_node *cn, *prev = NULL;

	mutex_enter(&crashme_lock);
	for (cn = first_node; cn; prev = cn, cn = cn->cn_next) {
		int rv;

		if (cn != rcn)
			continue;

		if (cn == first_node)
			first_node = cn->cn_next;
		if (prev)
			prev->cn_next = cn->cn_next;

		if ((rv = sysctl_destroyv(NULL, CTL_DEBUG, crashme_root,
					  cn->cn_name, CTL_EOL)) == 0)
			printf("%s: unable to remove %s from sysctl\n",
			       __func__, cn->cn_name);
		break;
	}
	mutex_exit(&crashme_lock);

	if (cn == NULL)
		return -1;

	return 0;
}

/*
 * list or execute a crashme node
 */
static int
crashme_sysctl_forwarder(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	crashme_node *cn;
	int error, arg = 0;

	for (cn = first_node; cn; cn = cn->cn_next) {
		if (cn->cn_sysctl == rnode)
			break;
	}
	if (!cn) {
		return EINVAL;
	}

	node = *rnode;
	node.sysctl_data = &arg;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return (error);

	if (!crashme_enable)
		return EACCES;

	DPRINTF("invoking \"%s\" (%s)", cn->cn_name, cn->cn_longname);
	if ((*cn->cn_fn)(arg) != 0)
		panic("crashme on %s failed!\n", cn->cn_name);
	return 0;
}

/*
 * register the various nodes with sysctl.
 */
SYSCTL_SETUP(selfdebug_crashme, "sysctl crashme setup")
{
	int rv;
	size_t n;

	mutex_init(&crashme_lock, MUTEX_DEFAULT, IPL_NONE);

	rv = sysctl_createv(NULL, 0, NULL, &crashme_root,
	    CTLFLAG_PERMANENT,
	    CTLTYPE_NODE, "crashme",
	    SYSCTL_DESCR("Crashme options"),
	    NULL, 0, NULL, 0,
	    CTL_DEBUG, CTL_CREATE, CTL_EOL);

	if (rv != 0 || crashme_root == NULL)
		return;

	rv = sysctl_createv(NULL, 0, NULL, NULL,
	    CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
	    CTLTYPE_BOOL, "crashme_enable",
	    SYSCTL_DESCR("Enable crashme"),
	    NULL, 0, &crashme_enable, 0,
	    CTL_DEBUG, CTL_CREATE, CTL_EOL);

	for (n = 0; n < __arraycount(nodes); n++) {
		crashme_node *cn = &nodes[n];

		rv = crashme_add(cn);

		/* don't insert */
		if (rv != 0)
			continue;
	}
}

/*
 * actual panic functions below
 */
static int
crashme_panic(int flags)
{

	panic("crashme plain old panic");
	return -1;
}

static int
crashme_null_deref(int flags)
{

	*(volatile char *)0 = 0;
	return -1;
}
