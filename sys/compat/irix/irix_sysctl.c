/*	$NetBSD: irix_sysctl.c,v 1.3 2004/03/24 15:34:52 atatat Exp $ */

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
__KERNEL_RCSID(0, "$NetBSD: irix_sysctl.c,v 1.3 2004/03/24 15:34:52 atatat Exp $");

#include <sys/param.h>
#include <sys/signal.h>
#include <sys/sysctl.h>

#include <compat/irix/irix_sysctl.h>

SYSCTL_SETUP(sysctl_irix_setup, "sysctl emul.irix subtree setup")
{

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "emul", NULL,
		       NULL, 0, NULL, 0,
		       CTL_EMUL, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "irix", NULL,
		       NULL, 0, NULL, 0,
		       CTL_EMUL, EMUL_IRIX, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "kern", NULL,
		       NULL, 0, NULL, 0,
		       CTL_EMUL, EMUL_IRIX, EMUL_IRIX_KERN, CTL_EOL);

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_STRING, "vendor", NULL,
		       NULL, 0, irix_si_vendor, 128,
		       CTL_EMUL, EMUL_IRIX, EMUL_IRIX_KERN,
		       EMUL_IRIX_KERN_VENDOR, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_STRING, "osprovider", NULL,
		       NULL, 0, irix_si_os_provider, 128,
		       CTL_EMUL, EMUL_IRIX, EMUL_IRIX_KERN,
		       EMUL_IRIX_KERN_OSPROVIDER, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_STRING, "osname", NULL,
		       NULL, 0, irix_si_os_name, 128,
		       CTL_EMUL, EMUL_IRIX, EMUL_IRIX_KERN,
		       EMUL_IRIX_KERN_OSNAME, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_STRING, "hwname", NULL,
		       NULL, 0, irix_si_hw_name, 128,
		       CTL_EMUL, EMUL_IRIX, EMUL_IRIX_KERN,
		       EMUL_IRIX_KERN_HWNAME, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_STRING, "osrelmaj", NULL,
		       NULL, 0, irix_si_osrel_maj, 128,
		       CTL_EMUL, EMUL_IRIX, EMUL_IRIX_KERN,
		       EMUL_IRIX_KERN_OSRELMAJ, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_STRING, "osrelmin", NULL,
		       NULL, 0, irix_si_osrel_min, 128,
		       CTL_EMUL, EMUL_IRIX, EMUL_IRIX_KERN,
		       EMUL_IRIX_KERN_OSRELMIN, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_STRING, "osrelpatch", NULL,
		       NULL, 0, irix_si_osrel_patch, 128,
		       CTL_EMUL, EMUL_IRIX, EMUL_IRIX_KERN,
		       EMUL_IRIX_KERN_OSRELPATCH, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_STRING, "processor", NULL,
		       NULL, 0, irix_si_processors, 128,
		       CTL_EMUL, EMUL_IRIX, EMUL_IRIX_KERN,
		       EMUL_IRIX_KERN_PROCESSOR, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_STRING, "version", NULL,
		       NULL, 0, irix_si_version, 128,
		       CTL_EMUL, EMUL_IRIX, EMUL_IRIX_KERN,
		       EMUL_IRIX_KERN_VERSION, CTL_EOL);
}
