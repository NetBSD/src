#	$NetBSD: Makefile.inc,v 1.18 2007/05/28 12:06:38 tls Exp $

.if !defined(NTP_MAKEFILE_INC)
NTP_MAKEFILE_INC=1

.include <bsd.own.mk>

USE_FORT?= yes	# network client/server

DIST=		${NETBSDSRCDIR}/dist/ntp
NTP_HTMLDIR=	${DIST}/html

CPPFLAGS+=	-I${.CURDIR}/../include -I${DIST}/include -I${DIST}/libopts -DHAVE_CONFIG_H

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
LIBOPTS!=	cd ${.CURDIR}/../libopts && ${PRINTOBJDIR}
LDADD+=		-L${LIBNTP} -lntp -L${LIBOPTS} -lopts
DPADD+=		${LIBNTP}/libntp.a ${LIBOPTS}/libopts.a

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
