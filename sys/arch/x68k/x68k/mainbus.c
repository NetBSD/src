/* $NetBSD: mainbus.c,v 1.3 2008/12/31 08:33:20 isaki Exp $ */

/*
 * Copyright (c) 2008 Tetsuya Isaki. All rights reserved.
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
 *    derived from this software without specific prior written permission
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
 * mainbus driver
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mainbus.c,v 1.3 2008/12/31 08:33:20 isaki Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

static int  mainbus_match(device_t, cfdata_t, void *);
static void mainbus_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(mainbus, 0,
	mainbus_match, mainbus_attach, NULL, NULL);

static int mainbus_attached;

static int
mainbus_match(device_t parent, cfdata_t cf, void *aux)
{

	if (mainbus_attached)
		return 0;

	return 1;
}

/*
 * "find" all the things that should be there.
 */
static void
mainbus_attach(device_t parent, device_t self, void *aux)
{

	mainbus_attached = 1;

	aprint_normal("\n");

	config_found(self, __UNCONST("intio")  , NULL);
	config_found(self, __UNCONST("grfbus") , NULL);
	config_found(self, __UNCONST("com")    , NULL);
	config_found(self, __UNCONST("com")    , NULL);
}
