/*	$NetBSD: consinit.c,v 1.1 2006/12/02 22:18:47 freza Exp $ */

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

#include "opt_cons.h"
#include "xlcom.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: consinit.c,v 1.1 2006/12/02 22:18:47 freza Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>

#include <evbppc/virtex/virtex.h>

#include <dev/cons.h>


#if NXLCOM > 0
extern struct consdev 	consdev_xlcom;
void    		xlcom_cninit(struct consdev *, bus_addr_t);
#endif

struct consdev 		*cn_tab = NULL;
bus_space_tag_t 	consdev_iot;
bus_space_handle_t 	consdev_ioh;


/*
 * Initialize the system console (hmm, as if anyone can see those panics).
 */
void
consinit(void)
{
	static int 	initted = 0;

	if (initted)
		return;

	/* Pick MD knowledge about console. */
	if (virtex_console_tag(CONS_NAME, &consdev_iot))
		panic("No bus space for %s console", CONS_NAME);

#if NXLCOM > 0
	if (strncmp("xlcom", CONS_NAME, 5) == 0) {
		cn_tab = &consdev_xlcom;
		xlcom_cninit(cn_tab, CONS_ADDR);

		goto done;
	}
#endif

	panic("No console"); 	/* XXX really panic? */
 done:
	initted = 1;
}
