#	$NetBSD: Makefile,v 1.9.2.1 2026/11/04 22:46:35 perseant Exp $

PROG=		readelf

PROGDPLIBS+=	gnuctf 	${TOP}/${BFDSUBDIR}/libgnuctf
PROGDPLIBS+=	bfd 	${TOP}/${BFDSUBDIR}/libbfd
PROGDPLIBS+=	sframe 	${TOP}/${BFDSUBDIR}/libsframe

.include "${.CURDIR}/../common/Makefile.prog"
