#	$NetBSD: Makefile.inc,v 1.11.6.1 2014/08/19 23:51:32 tls Exp $

.if !defined(NTP_MAKEFILE_INC)
NTP_MAKEFILE_INC=yes

USE_FORT?= yes	# network client/server

CWARNFLAGS+=	-Wno-missing-noreturn
CWARNFLAGS.clang+=	-Wno-unneeded-internal-declaration \
			-Wno-parentheses -Wno-constant-logical-operand \
			-Wno-error=unused-const-variable

WARNS?=	4


.include <bsd.own.mk>

IDIST=		${NETBSDSRCDIR}/external/bsd/ntp/dist
NTP_SRCDIR=	${NETBSDSRCDIR}/external/bsd/ntp
NTP_HTMLDIR=	/usr/share/doc/reference/ref8

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

.if ("${MKMDNS}" != "no")
CPPFLAGS+=-DHAVE_DNSREGISTRATION=1
LDADD+=-ldns_sd
.endif

SRCS+=	version.c

CLEANFILES+=version.c version.o
version.c: ${LIBNTP}/libntp.a ${.CURDIR}/../../scripts/mkver ${.CURDIR}/../../importdate
	${_MKTARGET_CREATE}
	${HOST_SH} ${.CURDIR}/../../scripts/mkver ${PROG} > /dev/null

.endif # defined(PROG)

.if (${MKCRYPTO} != "no")
CPPFLAGS+=-DOPENSSL
.endif

.if ("${MKINET6}" != "no")
CPPFLAGS+=-DWANT_IPV6
.else
CPPFLAGS+=-DISC_PLATFORM_NEEDIN6ADDRANY
.endif

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif

.endif
