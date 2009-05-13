#	$NetBSD: Makefile.inc,v 1.19.6.1 2009/05/13 19:20:31 jym Exp $

.if !defined(NTP_MAKEFILE_INC)

WARNS?=	1	# XXX -Wcast-qual -Wshadow -Wsign-compare

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
CLEANFILES+=version.c version.o
version.c: ${LIBNTP}/libntp.a ${.CURDIR}/../scripts/mkver
	${_MKTARGET_CREATE}
	${HOST_SH} ${.CURDIR}/../scripts/mkver ${PROG} > /dev/null
.endif # defined(PROG)

NTPSRCDIR?= ${PROG}
.PATH:		${DIST}/${NTPSRCDIR}

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif

.endif # !defined(NTP_MAKEFILE_INC)
