#	$NetBSD: Makefile.inc,v 1.12 2004/01/04 00:04:52 lukem Exp $

.if !defined(NTP_MAKEFILE_INC)
NTP_MAKEFILE_INC=1

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/dist/ntp

CPPFLAGS+=	-I${.CURDIR}/../include -I${DIST}/include -DHAVE_CONFIG_H

.if (${MKCRYPTO} != "no")
CPPFLAGS+=	-DOPENSSL
.endif

.if defined(PROG)
LIBNTP!=	cd ${.CURDIR}/../libntp && ${PRINTOBJDIR}
LIBISC!=	cd ${.CURDIR}/../libisc && ${PRINTOBJDIR}
LDADD+=		-L${LIBNTP} -lntp
DPADD+=		${LIBNTP}/libntp.a

.if (${MKCRYPTO} != "no")
LDADD+=		-lcrypto
DPADD+=		${LIBCRYPTO}
.endif

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
