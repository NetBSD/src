#	$NetBSD: Makefile,v 1.3.16.1 2020/04/08 14:05:30 martin Exp $

PROG=		objdump

CPPFLAGS+=-DOBJDUMP_PRIVATE_VECTORS=""

PROGDPLIBS+=	gnuctf 	${TOP}/${BFDSUBDIR}/libgnuctf

.include "${.CURDIR}/../common/Makefile.prog"
