/*	$NetBSD: sysctl.c,v 1.1.2.2 2018/04/16 02:00:08 pgoyette Exp $	*/

/*-
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: sysctl.c,v 1.1.2.2 2018/04/16 02:00:08 pgoyette Exp $");

#include <sys/param.h>
#include <sys/module.h>
#include <sys/sysctl.h>

/*
 * Check if sysctl -A contains an entry
 *    example_subroot1.sysctl_example=0
 * to test this module
 *
 */

MODULE(MODULE_CLASS_MISC, sysctl, NULL);

static int sysctl_example;

static struct sysctllog *example_sysctl_log;

static void sysctl_example_setup(struct sysctllog **);


/*
 * sysctl_example_setup :
 *    It first creates a subtree by adding a node to the tree.
 *    This node is named as example_subroot1.
 *
 *    It then creates a node in subtree for the example variable which
 *    is an integer and is defined in this file itself.
 *
 *                      ROOT
 *                        |
 *                        -------
 *                              |
 *                       examples_subroot1
 *                              |
 *                              |
 *                        sysctl_example (INT)
 *
 */

static void
sysctl_example_setup(struct sysctllog **clog)
{
	const struct sysctlnode *rnode;

	sysctl_createv(clog, 0, NULL, &rnode,
			CTLFLAG_PERMANENT,
			CTLTYPE_NODE, "example_subroot1",
			NULL,
			NULL, 0,
			NULL, 0,
			CTL_CREATE, CTL_EOL);

	sysctl_createv(clog, 0, &rnode, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "sysctl_example",
		       SYSCTL_DESCR("An example for sysctl_example"),
		       NULL, 0,
		       &sysctl_example, 0,
		       CTL_CREATE, CTL_EOL);
}

/*
 *  The sysctl_example modcmd has two functions.
 *  1. Call the sysctl_example_setup function to create a sysctl
 *     handle when the module is loaded in the kernel.
 *  2. Remove the sysctl entry from the kernel once the module
 *     is unloaded.
 */


static int
sysctl_modcmd(modcmd_t cmd, void *arg)
{
	switch(cmd) {
	case MODULE_CMD_INIT:
		printf("sysctl module inserted");
		sysctl_example_setup(&example_sysctl_log);
		break;
	case MODULE_CMD_FINI:
		printf("sysctl module unloaded");
		sysctl_teardown(&example_sysctl_log);
		break;
	default:
		return ENOTTY;
	}
	return 0;
}
