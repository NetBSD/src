/* $NetBSD: autoconf.c,v 1.1 2002/03/06 02:13:50 simonb Exp $ */

/*
 * Copyright 2000, 2001
 * Broadcom Corporation. All rights reserved.
 *
 * This software is furnished under license and may be used and copied only
 * in accordance with the following terms and conditions.  Subject to these
 * conditions, you may download, copy, install, use, modify and distribute
 * modified or unmodified copies of this software in source and/or binary
 * form. No title or ownership is transferred hereby.
 *
 * 1) Any source code used, modified or distributed must reproduce and
 *    retain this copyright notice and list of conditions as they appear in
 *    the source file.
 *
 * 2) No right is granted to use any trade name, trademark, or logo of
 *    Broadcom Corporation. Neither the "Broadcom Corporation" name nor any
 *    trademark or logo of Broadcom Corporation may be used to endorse or
 *    promote products derived from this software without the prior written
 *    permission of Broadcom Corporation.
 *
 * 3) THIS SOFTWARE IS PROVIDED "AS-IS" AND ANY EXPRESS OR IMPLIED
 *    WARRANTIES, INCLUDING BUT NOT LIMITED TO, ANY IMPLIED WARRANTIES OF
 *    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, OR
 *    NON-INFRINGEMENT ARE DISCLAIMED. IN NO EVENT SHALL BROADCOM BE LIABLE
 *    FOR ANY DAMAGES WHATSOEVER, AND IN PARTICULAR, BROADCOM SHALL NOT BE
 *    LIABLE FOR DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *    BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *    WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *    OR OTHERWISE), EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <machine/intr.h>

#ifdef IKOS
#include <sbmips/ikos/ikosvar.h>
#endif

struct device	*booted_device;
int		booted_partition;

void
cpu_configure(void)
{

	(void)splhigh();

#ifdef IKOS
	ikos_icu_init();
	if (system_set_clockfns(NULL, ikos_clock_init))
		panic("couldn't set clock init to ikos_clock_init");
#else
	if (config_rootfound("zbbus", NULL) == NULL)
		panic("no zbbus found");
#endif

	_splnone();
}

void
cpu_rootconf(void)
{

	/* XXXCGD don't know how to find the root device */

	printf("boot device: %s\n",
		booted_device ? booted_device->dv_xname : "<unknown>");

	setroot(booted_device, booted_partition);
}

void
device_register(struct device *dev, void *aux)
{

	/* XXXCGD */
}
