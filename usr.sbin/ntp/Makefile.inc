#	$NetBSD: Makefile.inc,v 1.11 2003/12/04 16:41:56 drochner Exp $

.if !defined(NTP_MAKEFILE_INC)
NTP_MAKEFILE_INC=1

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/dist/ntp

CPPFLAGS+=	-I${.CURDIR}/../include -I${DIST}/include -DHAVE_CONFIG_H

.if defined(PROG)
LIBNTP!=	cd ${.CURDIR}/../libntp && ${PRINTOBJDIR}
LIBISC!=	cd ${.CURDIR}/../libisc && ${PRINTOBJDIR}
LDADD+=		-L${LIBNTP} -lntp -lcrypto
DPADD+=		${LIBNTP}/libntp.a

SRCS+=	version.c
CLEANFILES+=version.c version.o .version
.version:
	${_MKTARGET_CREATE}
	${HOST_SH} ${.CURDIR}/../scripts/mkver ${PROG} > /dev/null
version.c: .version ${LIBNTP}/libntp.a
.endif

NTPSRCDIR?= ${PROG}
.PATH:		${DIST}/${NTPSRCDIR}

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif

.endif # !defined(NTP_MAKEFILE_INC)
