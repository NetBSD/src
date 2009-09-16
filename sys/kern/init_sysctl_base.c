/*	$NetBSD: init_sysctl_base.c,v 1.1 2009/09/16 15:03:56 pooka Exp $ */

/*-
 * Copyright (c) 2003, 2007, 2008, 2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Brown, and by Andrew Doran.
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
__KERNEL_RCSID(0, "$NetBSD: init_sysctl_base.c,v 1.1 2009/09/16 15:03:56 pooka Exp $");

#include <sys/param.h>
#include <sys/sysctl.h>

/*
 * sets up the base nodes...
 */
SYSCTL_SETUP(sysctl_root_setup, "sysctl base setup")
{

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "kern",
		       SYSCTL_DESCR("High kernel"),
		       NULL, 0, NULL, 0,
		       CTL_KERN, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "vm",
		       SYSCTL_DESCR("Virtual memory"),
		       NULL, 0, NULL, 0,
		       CTL_VM, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "vfs",
		       SYSCTL_DESCR("Filesystem"),
		       NULL, 0, NULL, 0,
		       CTL_VFS, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "net",
		       SYSCTL_DESCR("Networking"),
		       NULL, 0, NULL, 0,
		       CTL_NET, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "debug",
		       SYSCTL_DESCR("Debugging"),
		       NULL, 0, NULL, 0,
		       CTL_DEBUG, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "hw",
		       SYSCTL_DESCR("Generic CPU, I/O"),
		       NULL, 0, NULL, 0,
		       CTL_HW, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "machdep",
		       SYSCTL_DESCR("Machine dependent"),
		       NULL, 0, NULL, 0,
		       CTL_MACHDEP, CTL_EOL);
	/*
	 * this node is inserted so that the sysctl nodes in libc can
	 * operate.
	 */
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "user",
		       SYSCTL_DESCR("User-level"),
		       NULL, 0, NULL, 0,
		       CTL_USER, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "ddb",
		       SYSCTL_DESCR("In-kernel debugger"),
		       NULL, 0, NULL, 0,
		       CTL_DDB, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "proc",
		       SYSCTL_DESCR("Per-process"),
		       NULL, 0, NULL, 0,
		       CTL_PROC, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_NODE, "vendor",
		       SYSCTL_DESCR("Vendor specific"),
		       NULL, 0, NULL, 0,
		       CTL_VENDOR, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "emul",
		       SYSCTL_DESCR("Emulation settings"),
		       NULL, 0, NULL, 0,
		       CTL_EMUL, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "security",
		       SYSCTL_DESCR("Security"),
		       NULL, 0, NULL, 0,
		       CTL_SECURITY, CTL_EOL);
}
