#	$NetBSD: Makefile.inc,v 1.1 2000/03/29 13:13:42 simonb Exp $

.if !defined(NTP_MAKEFILE_INC)
NTP_MAKEFILE_INC=1

DIST=	${.CURDIR}/../../../dist/ntp

CPPFLAGS+= -I${.CURDIR}/../include -I${DIST}/include -DHAVE_CONFIG_H

.if !defined(LIB) || empty(LIB)
LIBNTP!=cd ${.CURDIR}/../libntp; make print-objdir
LDADD+=	-L${LIBNTP} -lntp
DPADD+=	${LIBNTP}/libntp.a
.endif

NTPSRCDIR?= ${PROG}
.PATH: ${DIST}/${NTPSRCDIR}

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif

.endif # !defined(NTP_MAKEFILE_INC)
