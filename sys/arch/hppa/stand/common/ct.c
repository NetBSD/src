/*	$NetBSD: ct.c,v 1.1 2014/02/24 07:23:43 skrll Exp $	*/

/*	$OpenBSD: ct.c,v 1.5 1999/04/20 20:01:01 mickey Exp $	*/

/*
 * Copyright (c) 1998-2004 Michael Shalayeff
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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR HIS RELATIVES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF MIND, USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * Copyright 1996 1995 by Open Software Foundation, Inc.
 *              All Rights Reserved
 *
 * Permission to use, copy, modify, and distribute this software and
 * its documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appears in all copies and
 * that both the copyright notice and this permission notice appear in
 * supporting documentation.
 *
 * OSF DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE.
 *
 * IN NO EVENT SHALL OSF BE LIABLE FOR ANY SPECIAL, INDIRECT, OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT,
 * NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

#include "libsa.h"

#include <sys/param.h>
#include <sys/disklabel.h>
#include <sys/reboot.h>

#include <machine/pdc.h>
#include <machine/iomod.h>

#include "dev_hppa.h"

struct pz_device ctdev;
iodcio_t ctiodc;	/* cartridge tape IODC entry point */
int ctcode[IODC_MAXSIZE/sizeof(int)];

int
ctopen(struct open_file *f, ...)
{
	struct hppa_dev *dp = f->f_devdata;
	int ret;

	if (ctiodc == 0) {

		if ((ret = (*pdc)(PDC_IODC, PDC_IODC_READ, pdcbuf, ctdev.pz_hpa,
				  IODC_IO, ctcode, IODC_MAXSIZE)) < 0) {
			printf("ct: device ENTRY_IO Read ret'd %d\n", ret);
			return (EIO);
		} else
			ctdev.pz_iodc_io = ctiodc = (iodcio_t) ctcode;
	}

	dp->pz_dev = &ctdev;

	if (ctiodc != NULL)
		if ((ret = (*ctiodc)(ctdev.pz_hpa, IODC_IO_READ, ctdev.pz_spa,
				     ctdev.pz_layers, pdcbuf, 0,
				     dp->buf, 0, 0)) < 0)
			printf("ct: device rewind ret'd %d\n", ret);

	return (0);
}

/*ARGSUSED*/
int
ctclose(struct open_file *f)
{
	dealloc(f->f_devdata, sizeof(struct hppa_dev));
	f->f_devdata = NULL;
	return 0;
}
