#	$NetBSD: Makefile,v 1.4 2020/04/04 00:46:55 christos Exp $

PROG=		readelf

PROGDPLIBS+=	ctf 	${TOP}/${BFDSUBDIR}/libctf
PROGDPLIBS+=	bfd 	${TOP}/${BFDSUBDIR}/libbfd

.include "${.CURDIR}/../common/Makefile.prog"
