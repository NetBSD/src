/*	$NetBSD: rump_dev.c,v 1.19.14.2 2013/02/25 00:30:08 tls Exp $	*/

/*
 * Copyright (c) 2009 Antti Kantee.  All Rights Reserved.
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
__KERNEL_RCSID(0, "$NetBSD: rump_dev.c,v 1.19.14.2 2013/02/25 00:30:08 tls Exp $");

#include <sys/param.h>
#include <sys/device.h>

#include "rump_private.h"
#include "rump_dev_private.h"

int nocomponent(void);
int nocomponent() {return 0;}
__weak_alias(buf_syncwait,nocomponent);

const char *rootspec = "rump0a"; /* usually comes from config */

RUMP_COMPONENT(RUMP__FACTION_DEV)
{
	extern int cold;

	pmf_init();

	KERNEL_LOCK(1, curlwp);

	config_init_mi();

	rump_component_init(RUMP_COMPONENT_DEV);

	rump_pdev_finalize();

	cold = 0;
	if (rump_component_count(RUMP_COMPONENT_DEV) > 0) {
		extern struct cfdriver mainbus_cd;
		extern struct cfattach mainbus_ca;
		extern struct cfdata cfdata[];

		config_cfdata_attach(cfdata, 0);
		config_cfdriver_attach(&mainbus_cd);
		config_cfattach_attach("mainbus", &mainbus_ca);
		if (config_rootfound("mainbus", NULL) == NULL)
			panic("no mainbus");

	}
	config_finalize();

	KERNEL_UNLOCK_LAST(curlwp);
}

void
cpu_rootconf(void)
{

	panic("%s: unimplemented", __func__);
}

void
device_register(device_t dev, void *v)
{

	/* nada */
}

void
device_register_post_config(device_t dev, void *v)
{

	/* nada */
}
