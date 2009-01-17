/*	$NetBSD: irix_sysctl.c,v 1.5.70.2 2009/01/17 13:28:42 mjf Exp $ */

/*-
 * Copyright (c) 2003, 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Brown.
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
__KERNEL_RCSID(0, "$NetBSD: irix_sysctl.c,v 1.5.70.2 2009/01/17 13:28:42 mjf Exp $");

#include <sys/param.h>
#include <sys/signal.h>
#include <sys/sysctl.h>

#include <compat/irix/irix_sysctl.h>

static struct sysctllog *irix_clog;

void
irix_sysctl_fini(void)
{

	sysctl_teardown(&irix_clog);
}

void
irix_sysctl_init(void)
{

	sysctl_createv(&irix_clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "emul", NULL,
		       NULL, 0, NULL, 0,
		       CTL_EMUL, CTL_EOL);
	sysctl_createv(&irix_clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "irix",
		       SYSCTL_DESCR("IRIX emulation settings"),
		       NULL, 0, NULL, 0,
		       CTL_EMUL, EMUL_IRIX, CTL_EOL);
	sysctl_createv(&irix_clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "kern",
		       SYSCTL_DESCR("IRIX kernel emulation settings"),
		       NULL, 0, NULL, 0,
		       CTL_EMUL, EMUL_IRIX, EMUL_IRIX_KERN, CTL_EOL);

	sysctl_createv(&irix_clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_STRING, "vendor",
		       SYSCTL_DESCR("Emulated IRIX vendor name"),
		       NULL, 0, irix_si_vendor, 128,
		       CTL_EMUL, EMUL_IRIX, EMUL_IRIX_KERN,
		       EMUL_IRIX_KERN_VENDOR, CTL_EOL);
	sysctl_createv(&irix_clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_STRING, "osprovider",
		       SYSCTL_DESCR("Emulated IRIX system manufacturer"),
		       NULL, 0, irix_si_os_provider, 128,
		       CTL_EMUL, EMUL_IRIX, EMUL_IRIX_KERN,
		       EMUL_IRIX_KERN_OSPROVIDER, CTL_EOL);
	sysctl_createv(&irix_clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_STRING, "osname",
		       SYSCTL_DESCR("Emulated IRIX operating system name"),
		       NULL, 0, irix_si_os_name, 128,
		       CTL_EMUL, EMUL_IRIX, EMUL_IRIX_KERN,
		       EMUL_IRIX_KERN_OSNAME, CTL_EOL);
	sysctl_createv(&irix_clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_STRING, "hwname",
		       SYSCTL_DESCR("Emulated IRIX system type"),
		       NULL, 0, irix_si_hw_name, 128,
		       CTL_EMUL, EMUL_IRIX, EMUL_IRIX_KERN,
		       EMUL_IRIX_KERN_HWNAME, CTL_EOL);
	sysctl_createv(&irix_clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_STRING, "osrelmaj",
		       SYSCTL_DESCR("Emulated IRIX major release number"),
		       NULL, 0, irix_si_osrel_maj, 128,
		       CTL_EMUL, EMUL_IRIX, EMUL_IRIX_KERN,
		       EMUL_IRIX_KERN_OSRELMAJ, CTL_EOL);
	sysctl_createv(&irix_clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_STRING, "osrelmin",
		       SYSCTL_DESCR("Emulated IRIX minor release number"),
		       NULL, 0, irix_si_osrel_min, 128,
		       CTL_EMUL, EMUL_IRIX, EMUL_IRIX_KERN,
		       EMUL_IRIX_KERN_OSRELMIN, CTL_EOL);
	sysctl_createv(&irix_clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_STRING, "osrelpatch",
		       SYSCTL_DESCR("Emulated IRIX patch level"),
		       NULL, 0, irix_si_osrel_patch, 128,
		       CTL_EMUL, EMUL_IRIX, EMUL_IRIX_KERN,
		       EMUL_IRIX_KERN_OSRELPATCH, CTL_EOL);
	sysctl_createv(&irix_clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_STRING, "processor",
		       SYSCTL_DESCR("Emulated IRIX processor type"),
		       NULL, 0, irix_si_processors, 128,
		       CTL_EMUL, EMUL_IRIX, EMUL_IRIX_KERN,
		       EMUL_IRIX_KERN_PROCESSOR, CTL_EOL);
	sysctl_createv(&irix_clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_STRING, "version",
		       SYSCTL_DESCR("Emulated IRIX version number"),
		       NULL, 0, irix_si_version, 128,
		       CTL_EMUL, EMUL_IRIX, EMUL_IRIX_KERN,
		       EMUL_IRIX_KERN_VERSION, CTL_EOL);
}
