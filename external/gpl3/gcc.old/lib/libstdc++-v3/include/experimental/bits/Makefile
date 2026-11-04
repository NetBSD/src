#	$NetBSD: Makefile,v 1.2.2.1 2026/11/04 23:00:52 perseant Exp $

.include <bsd.init.mk>
.include "../../Makefile.inc"

.include "${.CURDIR}/../../../arch/${GCC_MACHINE_ARCH}/defs.mk"

.cc: # disable .cc->NULL transform

INCS=		${G_experimental_bits_headers}
INCSDIR=	/usr/include/g++/experimental/bits

.include "${.CURDIR}/../../Makefile.includes"
