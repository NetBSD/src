/* $NetBSD: gpiolib.c,v 1.1 2024/12/17 12:44:33 martin Exp $ */
/*	$OpenBSD: gpio.c,v 1.6 2006/01/14 12:33:49 grange Exp $	*/

/*
 * Copyright (c) 2008, 2009, 2010, 2011 Marc Balmer <marc@msys.ch>
 * Copyright (c) 2004, 2006 Alexander Yurchenko <grange@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */


#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: gpiolib.c,v 1.1 2024/12/17 12:44:33 martin Exp $");

/*
 * General Purpose Input/Output framework.
 */

#include <sys/systm.h>
#include <sys/gpio.h>
#include <dev/gpio/gpiovar.h>

int
gpiobus_print(void *aux, const char *pnp)
{
#if 0
	struct gpiobus_attach_args *gba = aux;
#endif
	if (pnp != NULL)
		aprint_normal("gpiobus at %s", pnp);

	return UNCONF;
}

