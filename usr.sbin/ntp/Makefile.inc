#	$NetBSD: Makefile.inc,v 1.6 2001/01/11 13:15:06 enami Exp $

.if !defined(NTP_MAKEFILE_INC)
NTP_MAKEFILE_INC=1

.include <bsd.own.mk>

NTPDOCDIR?=	${HTMLDOCDIR}/ntp

.if exists(${.CURDIR}/../../../dist/ntp)
DIST=		${.CURDIR}/../../../dist/ntp
.elif exists(${.CURDIR}/../../../../dist/ntp)
DIST=		${.CURDIR}/../../../../dist/ntp
.else
# error here?
.endif

CPPFLAGS+=	-I${.CURDIR}/../include -I${DIST}/include -DHAVE_CONFIG_H
.if (${MACHINE_ARCH} == "sparc64")
CFLAGS+=        -msoft-quad-float
.endif

.if defined(PROG)
LIBNTP!=	cd ${.CURDIR}/../libntp && ${PRINTOBJDIR}
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
