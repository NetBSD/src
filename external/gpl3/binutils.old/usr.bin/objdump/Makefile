#	$NetBSD: Makefile,v 1.9 2024/06/29 16:36:17 christos Exp $

PROG=		objdump

CPPFLAGS+=-DOBJDUMP_PRIVATE_VECTORS=""

PROGDPLIBS+=	gnuctf 	${TOP}/${BFDSUBDIR}/libgnuctf

.include "${.CURDIR}/../common/Makefile.prog"
