#	$NetBSD: Makefile,v 1.1.1.3 2022/12/23 17:02:16 christos Exp $

PROG=		objdump

CPPFLAGS+=-DOBJDUMP_PRIVATE_VECTORS=""

PROGDPLIBS+=	gnuctf 	${TOP}/${BFDSUBDIR}/libgnuctf

.include "${.CURDIR}/../common/Makefile.prog"
