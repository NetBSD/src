/* 	$NetBSD: virtex.h,v 1.1 2006/12/02 22:18:47 freza Exp $ */

/*
 * Copyright (c) 2006 Jachym Holecek
 * All rights reserved.
 *
 * Written for DFC Design, s.r.o.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _VIRTEX_H_
#define	_VIRTEX_H_

#ifdef _KERNEL_OPT
#include "opt_cons.h"
#endif

#ifndef CONS_NAME
#define CONS_NAME 	"xlcom0"
#endif

#ifndef CONS_ADDR
#define CONS_ADDR 	0x00
#endif

struct mem_region;

/* Setup console bus space. */
int 	virtex_console_tag(const char *, bus_space_tag_t *);

/* Called after RAM is linear mapped. Translation & console still off. */
void 	virtex_machdep_init(vaddr_t, vsize_t, struct mem_region *,
	    struct mem_region *);

/* For use by console. Tag is initialized before <foo>_cninit. */
extern bus_space_tag_t 		consdev_iot;
extern bus_space_handle_t 	consdev_ioh;

#endif /*_VIRTEX_H_*/
