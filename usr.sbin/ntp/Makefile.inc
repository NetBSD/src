#	$NetBSD: Makefile.inc,v 1.15 2005/03/09 03:11:23 christos Exp $

.if !defined(NTP_MAKEFILE_INC)
NTP_MAKEFILE_INC=1

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/dist/ntp

CPPFLAGS+=	-I${.CURDIR}/../include -I${DIST}/include -DHAVE_CONFIG_H

.if (${MKCRYPTO} != "no")
CPPFLAGS+=	-DOPENSSL
.endif

.if (${USE_INET6} != "no")
CPPFLAGS+=	-DWANT_IPV6
.else
CPPFLAGS+=	-DISC_PLATFORM_NEEDIN6ADDRANY
.endif

.if defined(PROG)
LIBNTP!=	cd ${.CURDIR}/../libntp && ${PRINTOBJDIR}
LIBISC!=	cd ${.CURDIR}/../libisc && ${PRINTOBJDIR}
LDADD+=		-L${LIBNTP} -lntp
DPADD+=		${LIBNTP}/libntp.a

.if (${MKCRYPTO} != "no")
LDADD+=		-lcrypto -lcrypt
DPADD+=		${LIBCRYPTO} ${LIBCRYPT}
.endif

SRCS+=	version.c
CLEANFILES+=version.c version.o .version
.version: ${.CURDIR}/../scripts/mkver
	${_MKTARGET_CREATE}
	${HOST_SH} ${.CURDIR}/../scripts/mkver ${PROG} > /dev/null
version.c: .version ${LIBNTP}/libntp.a
.endif # defined(PROG)

NTPSRCDIR?= ${PROG}
.PATH:		${DIST}/${NTPSRCDIR}

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif

.endif # !defined(NTP_MAKEFILE_INC)
