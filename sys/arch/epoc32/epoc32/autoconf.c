/*	$NetBSD: autoconf.c,v 1.1 2013/04/28 12:11:26 kiyohara Exp $	*/
/*
 * Copyright (c) 2013 KIYOHARA Takashi
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
__KERNEL_RCSID(0, "$NetBSD: autoconf.c,v 1.1 2013/04/28 12:11:26 kiyohara Exp $");

#include <sys/systm.h>
#include <sys/types.h>
#include <sys/conf.h>
#include <sys/device.h>


void
cpu_configure(void)
{

	splhigh();
	splserial();

	if (config_rootfound("mainbus", NULL) == NULL)
		panic("configure: mainbus not configured");

	/* Time to start taking interrupts so lets open the flood gates .... */
	(void)spl0();
}

void
cpu_rootconf(void)
{

	aprint_normal("boot device: %s\n",
	    booted_device ? device_xname(booted_device) : "<unknown>");

	setroot(booted_device, booted_partition);
}

void
device_register(device_t dev, void *aux)
{

	if (device_is_a(dev, "clpslcd") ||
	    device_is_a(dev, "wmlcd")) {
		extern int epoc32_fb_width, epoc32_fb_height, epoc32_fb_addr;
		prop_dictionary_t dict = device_properties(dev);

		prop_dictionary_set_uint32(dict, "width", epoc32_fb_width);
		prop_dictionary_set_uint32(dict, "height", epoc32_fb_height);
		prop_dictionary_set_uint32(dict, "addr", epoc32_fb_addr);
	}
}
