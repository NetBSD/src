#	$NetBSD: Makefile,v 1.6 2024/06/30 16:10:22 christos Exp $

PROG=		readelf

PROGDPLIBS+=	gnuctf 	${TOP}/${BFDSUBDIR}/libgnuctf
PROGDPLIBS+=	bfd 	${TOP}/${BFDSUBDIR}/libbfd
PROGDPLIBS+=	sframe 	${TOP}/${BFDSUBDIR}/libsframe

.include "${.CURDIR}/../common/Makefile.prog"
