#	$NetBSD: Makefile,v 1.10 2025/08/24 17:57:31 christos Exp $

PROG=		readelf

PROGDPLIBS+=	gnuctf 	${TOP}/${BFDSUBDIR}/libgnuctf
PROGDPLIBS+=	bfd 	${TOP}/${BFDSUBDIR}/libbfd
PROGDPLIBS+=	sframe 	${TOP}/${BFDSUBDIR}/libsframe

.include "${.CURDIR}/../common/Makefile.prog"
