/*	$NetBSD: internal.c,v 1.1 2013/04/28 12:11:26 kiyohara Exp $	*/
/*
 * Copyright (c) 2012 KIYOHARA Takashi
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: internal.c,v 1.1 2013/04/28 12:11:26 kiyohara Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/errno.h>

static int internal_match(device_t, cfdata_t, void *);
static void internal_attach(device_t, device_t, void *);

static int internal_search(device_t, cfdata_t, const int *, void *);

CFATTACH_DECL_NEW(internal, 0, internal_match, internal_attach, NULL, NULL);

/* ARGSUSED */
static int
internal_match(device_t parent, cfdata_t match, void *aux)
{
	/* always attach */
	return 1;
}

/* ARGSUSED */
static void
internal_attach(device_t parent, device_t self, void *aux)
{

	aprint_naive("\n");
	aprint_normal("\n");

	config_search_ia(internal_search, self, NULL, aux);
}

static int
internal_search(device_t self, cfdata_t cf, const int *ldesc, void *aux)
{
	extern char epoc32_model[];

	if (strcmp(epoc32_model, "SERIES5 R1") == 0) {
		if (strcmp(cf->cf_name, "clpssoc") != 0)
			return 0;
	} else if (strcmp(epoc32_model, "SERIES5mx") == 0) {
		if (strcmp(cf->cf_name, "windermere") != 0)
			return 0;
	} else if (strcmp(epoc32_model, "SERIES 7") == 0) {
		if (strcmp(cf->cf_name, "saip") != 0)
			return 0;
	} else
		return 0;

	if (config_match(self, cf, aux) > 0)
		config_attach(self, cf, aux, NULL);

	return 0;
}
