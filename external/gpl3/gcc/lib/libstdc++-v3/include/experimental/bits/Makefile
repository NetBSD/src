#	$NetBSD: Makefile,v 1.1 2021/08/16 06:56:27 mrg Exp $

.include <bsd.init.mk>
.include "../../Makefile.inc"

.include "${.CURDIR}/../../../arch/${GCC_MACHINE_ARCH}/defs.mk"

.cc: # disable .cc->NULL transform

INCS=		${G_experimental_bits_headers}
INCSDIR=	/usr/include/g++/experimental/bits

.include "${.CURDIR}/../../Makefile.includes"
