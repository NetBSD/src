#	$NetBSD: Makefile.inc,v 1.2.2.1 2000/06/22 18:01:05 minoura Exp $

.if !defined(NTP_MAKEFILE_INC)
NTP_MAKEFILE_INC=1

NTPDOCDIR?=	${HTMLDOCDIR}/ntp

.if exists(${.CURDIR}/../../../dist/ntp)
DIST=		${.CURDIR}/../../../dist/ntp
.elif exists(${.CURDIR}/../../../../dist/ntp)
DIST=		${.CURDIR}/../../../../dist/ntp
.else
# error here?
.endif

CPPFLAGS+=	-I${.CURDIR}/../include -I${DIST}/include -DHAVE_CONFIG_H

.if defined(PROG)
LIBNTP!=	cd ${.CURDIR}/../libntp; ${MAKE} print-objdir
LDADD+=		-L${LIBNTP} -lntp
DPADD+=		${LIBNTP}/libntp.a

SRCS+=	version.c
CLEANFILES+=version.c version.o .version
.version:
	sh ${.CURDIR}/../scripts/mkver ${PROG} > /dev/null
version.c: .version ${LIBNTP}/libntp.a
.endif

NTPSRCDIR?= ${PROG}
.PATH:		${DIST}/${NTPSRCDIR}

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif

.endif # !defined(NTP_MAKEFILE_INC)
