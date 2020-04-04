#	$NetBSD: Makefile,v 1.4 2020/04/04 00:46:55 christos Exp $

PROG=		objdump

CPPFLAGS+=-DOBJDUMP_PRIVATE_VECTORS=""

PROGDPLIBS+=	ctf 	${TOP}/${BFDSUBDIR}/libctf

.include "${.CURDIR}/../common/Makefile.prog"
