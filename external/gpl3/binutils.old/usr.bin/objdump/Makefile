#	$NetBSD: Makefile,v 1.8 2022/12/23 17:09:55 christos Exp $

PROG=		objdump

CPPFLAGS+=-DOBJDUMP_PRIVATE_VECTORS=""

PROGDPLIBS+=	gnuctf 	${TOP}/${BFDSUBDIR}/libgnuctf

.include "${.CURDIR}/../common/Makefile.prog"
