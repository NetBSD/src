#	$NetBSD: Makefile,v 1.1.1.3 2022/12/23 17:02:16 christos Exp $

PROG=		readelf

PROGDPLIBS+=	gnuctf 	${TOP}/${BFDSUBDIR}/libgnuctf
PROGDPLIBS+=	bfd 	${TOP}/${BFDSUBDIR}/libbfd

.include "${.CURDIR}/../common/Makefile.prog"
