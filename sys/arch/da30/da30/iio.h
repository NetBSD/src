/*	$NetBSD: iio.h,v 1.2 1994/10/26 02:32:48 cgd Exp $	*/

#define IIO_CFLOC_ADDR(cf)	(IIOV(0xE00000 + (cf)->cf_loc[0] * 0x10000))
#define IIO_CFLOC_LEVEL(cf)	((cf)->cf_loc[1])
