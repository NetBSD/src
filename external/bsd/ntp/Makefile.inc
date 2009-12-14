#	$NetBSD: Makefile.inc,v 1.2 2009/12/14 02:09:39 christos Exp $

.if !defined(NTP_MAKEFILE_INC)
NTP_MAKEFILE_INC=yes

USE_FORT?= yes	# network client/server

WARNS?=	0
COPTS+=-Werror

.include <bsd.own.mk>

.if ${MKCRYPTO} == "no"
NTP_USE_OPENSSL?=no
.endif

IDIST=		${NETBSDSRCDIR}/external/bsd/ntp/dist
NTP_SRCDIR=	${NETBSDSRCDIR}/external/bsd/ntp
NTP_HTMLDIR=	/usr/share/doc/html/ntp

SYSCONFDIR=/etc
LOCALSTATEDIR=/var

CPPFLAGS+=-DHAVE_CONFIG_H
CPPFLAGS+=-I${NTP_SRCDIR}/include \
	  -I${IDIST}/include \
	  -I${IDIST}/lib/isc/include \
	  -I${IDIST}/lib/isc/unix/include \
	  -I${IDIST}/lib/isc/nothreads/include

.if defined(PROG)
LIBNTP!=	cd ${.CURDIR}/../../lib/libntp && ${PRINTOBJDIR}
LIBISC!=	cd ${.CURDIR}/../../lib/libiscntp && ${PRINTOBJDIR}
LIBOPTS!=	cd ${.CURDIR}/../../lib/libopts && ${PRINTOBJDIR}
LIBPARSE!=	cd ${.CURDIR}/../../lib/libparse && ${PRINTOBJDIR}

LDADD+=		-L${LIBNTP} -lntp -L${LIBOPTS} -lopts
DPADD+=		${LIBNTP}/libntp.a ${LIBOPTS}/libopts.a

.if (${MKCRYPTO} != "no")
LDADD+=		-lcrypto -lcrypt
DPADD+=		${LIBCRYPTO} ${LIBCRYPT}
.endif

SRCS+=	version.c

CLEANFILES+=version.c version.o
version.c: ${LIBNTP}/libntp.a ${.CURDIR}/../../scripts/mkver ${.CURDIR}/../../importdate
	${_MKTARGET_CREATE}
	${HOST_SH} ${.CURDIR}/../../scripts/mkver ${PROG} > /dev/null

.endif # defined(PROG)

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif

.endif
