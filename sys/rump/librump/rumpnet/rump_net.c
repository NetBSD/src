/*	$NetBSD: rump_net.c,v 1.20.6.1 2018/01/16 13:04:34 martin Exp $	*/

/*
 * Copyright (c) 2008 Antti Kantee.  All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: rump_net.c,v 1.20.6.1 2018/01/16 13:04:34 martin Exp $");

#include <sys/param.h>

#include <sys/domain.h>
#include <sys/mbuf.h>
#include <sys/socketvar.h>

#include <net/bpf.h>
#include <net/pfil.h>
#include <net/route.h>

#include <rump-sys/kern.h>
#include <rump-sys/net.h>

RUMP_COMPONENT(RUMP__FACTION_NET)
{

	bpf_setops();

	mbinit();
	soinit();
	pfil_init();

	domaininit(false);

	rump_component_init(RUMP_COMPONENT_NET);
	rump_component_init(RUMP_COMPONENT_NET_ROUTE);
	rump_component_init(RUMP_COMPONENT_NET_IF);
	rump_component_init(RUMP_COMPONENT_NET_IFCFG);

	domaininit_post();
}
