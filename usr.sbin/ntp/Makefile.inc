#	$NetBSD: Makefile.inc,v 1.2 2000/05/02 12:01:10 simonb Exp $

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
LIBNTP!=	cd ${.CURDIR}/../libntp; make print-objdir
LDADD+=		-L${LIBNTP} -lntp
DPADD+=		${LIBNTP}/libntp.a
.endif

NTPSRCDIR?= ${PROG}
.PATH:		${DIST}/${NTPSRCDIR}

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif

.endif # !defined(NTP_MAKEFILE_INC)
