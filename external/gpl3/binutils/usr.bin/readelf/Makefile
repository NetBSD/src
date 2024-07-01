#	$NetBSD: Makefile,v 1.5.8.1 2024/07/01 01:00:42 perseant Exp $

PROG=		readelf

PROGDPLIBS+=	gnuctf 	${TOP}/${BFDSUBDIR}/libgnuctf
PROGDPLIBS+=	bfd 	${TOP}/${BFDSUBDIR}/libbfd
PROGDPLIBS+=	sframe 	${TOP}/${BFDSUBDIR}/libsframe

.include "${.CURDIR}/../common/Makefile.prog"
