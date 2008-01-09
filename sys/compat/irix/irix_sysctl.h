/*	$NetBSD: irix_sysctl.h,v 1.4.46.1 2008/01/09 01:50:52 matt Exp $ */

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Emmanuel Dreyfus.
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

#ifndef _IRIX_SYSCTL_H_
#define _IRIX_SYSCTL_H_

extern char irix_si_vendor[128];
extern char irix_si_os_provider[128];
extern char irix_si_os_name[128];
extern char irix_si_hw_name[128];
extern char irix_si_osrel_maj[128];
extern char irix_si_osrel_min[128];
extern char irix_si_osrel_patch[128];
extern char irix_si_processors[128];
extern char irix_si_version[128];

#define EMUL_IRIX_KERN			1
#define EMUL_IRIX_MAXID			2

#define EMUL_IRIX_NAMES { \
	{ 0, 0 }, \
	{ "kern", CTLTYPE_NODE }, \
}

#define EMUL_IRIX_KERN_VENDOR		1
#define EMUL_IRIX_KERN_OSPROVIDER	2
#define EMUL_IRIX_KERN_OSNAME		3
#define EMUL_IRIX_KERN_HWNAME		4
#define EMUL_IRIX_KERN_OSRELMAJ		5
#define EMUL_IRIX_KERN_OSRELMIN		6
#define EMUL_IRIX_KERN_OSRELPATCH	7
#define EMUL_IRIX_KERN_PROCESSOR	8
#define EMUL_IRIX_KERN_VERSION		9
#define EMUL_IRIX_KERN_MAXID		10

#define EMUL_IRIX_KERN_NAMES { \
	{ 0, 0 }, \
	{ "vendor", CTLTYPE_STRING }, \
	{ "osprovider", CTLTYPE_STRING }, \
	{ "osname", CTLTYPE_STRING }, \
	{ "hwname", CTLTYPE_STRING }, \
	{ "osrelmaj", CTLTYPE_STRING }, \
	{ "osrelmin", CTLTYPE_STRING }, \
	{ "osrelpatch", CTLTYPE_STRING }, \
	{ "processor", CTLTYPE_STRING }, \
	{ "version", CTLTYPE_STRING }, \
}

int irix_sysctl(int *, u_int, void *, size_t *,
    void *, size_t, struct proc *);

#ifdef SYSCTL_SETUP_PROTO
SYSCTL_SETUP_PROTO(sysctl_emul_irix_setup);
#endif /* SYSCTL_SETUP_PROTO */

#endif /* _IRIX_SYSCTL_H_ */
