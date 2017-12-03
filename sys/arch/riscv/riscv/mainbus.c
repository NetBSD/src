/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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

#include "locators.h"

#include <sys/cdefs.h>

__RCSID("$NetBSD: mainbus.c,v 1.1.18.2 2017/12/03 11:36:39 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <riscv/locore.h>

static int mainbus_match(device_t, cfdata_t, void *);
static void mainbus_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(mainbus, 0,
    mainbus_match, mainbus_attach, NULL, NULL);

static int
mainbus_print(void *aux, const char *name)
{
	struct mainbus_attach_args * const maa = aux;

	if (maa->maa_instance != MAINBUSCF_INSTANCE_DEFAULT)
		printf(" instance %d", maa->maa_instance);

	return QUIET;
}

int
mainbus_match(device_t parent, cfdata_t cf, void *aux)
{
	return 1;
}

void
mainbus_attach(device_t parent, device_t self, void *aux)
{
	struct mainbus_attach_args maa;

	maa.maa_name = "cpu";
	maa.maa_instance = 0;

	config_found(self, &maa, mainbus_print);
}
