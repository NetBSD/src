/*	$NetBSD: zsspvar.h,v 1.1.92.1 2012/02/18 07:33:51 mrg Exp $	*/
/*	$OpenBSD: zaurus_sspvar.h,v 1.3 2005/04/08 21:58:49 uwe Exp $	*/

/*
 * Copyright (c) 2005 Uwe Stuehler <uwe@bsdx.de>
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

#ifndef	_ZAURUS_DEV_ZSSPVAR_H_
#define	_ZAURUS_DEV_ZSSPVAR_H_

struct zssp_attach_args {
	const char *zaa_name;
};

#define ZSSP_IC_MAX1111		1	/* temperature/voltage ADC */
#define ZSSP_IC_LZ9JG18		2	/* LCD backlight controller */
#define ZSSP_IC_ADS7846		3	/* touch screen controller */

void		zssp_ic_start(int, uint32_t);
uint32_t	zssp_ic_send(int, uint32_t);
uint32_t	zssp_ic_stop(int);

int		zssp_read_max1111(uint32_t);
uint32_t	zssp_read_ads7846(u_int32_t);
void		zssp_write_lz9jg18(uint32_t);

#endif	/* _ZAURUS_DEV_ZSSPVAR_H_ */
