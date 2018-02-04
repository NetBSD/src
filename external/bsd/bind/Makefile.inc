#	$NetBSD: Makefile.inc,v 1.28 2018/02/04 03:19:52 christos Exp $

.if !defined(BIND9_MAKEFILE_INC)
BIND9_MAKEFILE_INC=yes

#NAMED_DEBUG=1

USE_FORT?= yes	# network client/server

WARNS?=	1

CWARNFLAGS.clang+=	-Wno-unused-value -Wno-parentheses

.include <bsd.own.mk>

NAMED_USE_OPENSSL?=yes

.if exists(${NETBSDSRCDIR}/sys/sys/atomic.h)
NAMED_USE_PTHREADS?=yes
.else
NAMED_USE_PTHREADS?=no
.endif

IDIST=		${NETBSDSRCDIR}/external/bsd/bind/dist
BIND_SRCDIR=	${NETBSDSRCDIR}/external/bsd/bind
BIND_HTMLDIR=	/usr/share/doc/reference/ref8/bind9

VERSIONFILE=${IDIST}/version
.include "${VERSIONFILE}"

VERSION=${MAJORVER}.${MINORVER}.${PATCHVER}${RELEASETYPE}${RELEASEVER}
SYSCONFDIR=/etc
LOCALSTATEDIR=/var

CPPFLAGS+=-I${BIND_SRCDIR}/include \
	-I${IDIST} \
	-I${IDIST}/lib/dns/include \
	-I${IDIST}/lib/isc/include -I${IDIST}/lib/isc/unix/include \
	-I${IDIST}/lib/bind9/include \
	-I${IDIST}/lib/isccfg/include \
	-I${IDIST}/lib/isccc/include \
	-I${IDIST}/lib/lwres/include -I${IDIST}/lib/lwres/unix/include \
	-DNS_LOCALSTATEDIR=\"${LOCALSTATEDIR}\" \
	-DNS_SYSCONFDIR=\"${SYSCONFDIR}\" \
	-DSESSION_KEYFILE=\"${LOCALSTATEDIR}/run/named/session.key\" \
	-DVERSION=\"${VERSION}\" -DBIND9

.if (${USE_INET6} != "no")
CPPFLAGS+=	-DWANT_IPV6
CPPFLAGS+=	-DALLOW_FILTER_AAAA
.endif

.if defined(HAVE_GCC)
COPTS+=	-Wno-pointer-sign
.endif

.if defined(NAMED_DEBUG)
DBG=-g3 -gstabs
.endif

.if !defined(LIB) || empty(LIB)
# NOTE: the order of these libraries is important...
.if defined(NAMED_DEBUG)
LDADD+=		-lbind9_g -llwres_g -lisccfg_g -ldns_g -lisccc_g -lisc_g
.else
LDADD+=		-lbind9 -llwres -lisccfg -ldns -lisccc -lisc
DPADD+=		${LIBBIND9} ${LIBDNS} ${LIBLWRES}
DPADD+=		${LIBISCCFG} ${LIBISCCC} ${LIBISC}
.endif
.else
CPPFLAGS+= -DLIBINTERFACE=${LIBINTERFACE} \
	   -DLIBREVISION=${LIBREVISION} -DLIBAGE=${LIBAGE}
.endif
#CPPFLAGS+= -DUSE_MEMIMPREGISTER -DUSE_APPIMPREGISTER -DUSE_SOCKETIMPREGISTER \
#    -DUSE_TIMERIMPREGISTER

.if ${NAMED_USE_PTHREADS} == "yes"
# XXX: Not ready yet
# CPPFLAGS+=	-DISC_PLATFORM_USE_NATIVE_RWLOCKS
CPPFLAGS+=	-DISC_PLATFORM_USETHREADS
.if !defined (LIB) || empty(LIB)
LDADD+= -lpthread
DPADD+= ${LIBPTHREAD}
.else
LIBDPLIBS+=      pthread  ${NETBSDSRCDIR}/lib/libpthread
.endif
.endif

.if ${NAMED_USE_OPENSSL} == "yes"
CPPFLAGS+=-DOPENSSL -DUSE_ISC_SPNEGO -DHAVE_OPENSSL_GOST
.if ${MKKERBEROS} != "no"
CPPFLAGS+=-DGSSAPI
.endif
.if ${MKKERBEROS} != "no"
.if !defined (LIB) || empty(LIB)
LDADD+= -lgssapi -lheimntlm -lkrb5 -lhx509 -lheimbase \
	-lcom_err  -lroken -lasn1 -lwind -lsqlite3
DPADD+= ${LIBGSSAPI} ${LIBKRB5} ${LIBHX509} ${LIBHEIMNTLM} ${LIBHEIMBASE} \
	${LIBCOM_ERR} ${LIBROKEN} ${LIBASN1} ${LIBWIND} ${LIBSQLITE3}
.else
.for L in gssapi krb5 hx509 heimntlm heimbase com_err roken asn1 wind
LIBDPLIBS+=	$L ${NETBSDSRCDIR}/crypto/external/bsd/heimdal/lib/lib$L
.endfor
.endif
.endif
.if !defined (LIB) || empty(LIB)
LDADD+= -lcrypto -lcrypt
DPADD+= ${LIBCRYPTO} ${LIBCRYPT}
.else
LIBDPLIBS+=	crypto ${NETBSDSRCDIR}/crypto/external/bsd/${EXTERNAL_OPENSSL_SUBDIR}/lib/libcrypto
.endif
.endif

.if ${NAMED_USE_PTHREADS} == "yes"
CPPFLAGS+=-DISC_PLATFORM_USETHREADS -I${IDIST}/lib/isc/pthreads/include
.else
CPPFLAGS+=-I${IDIST}/lib/isc/nothreads/include
.endif

.if exists(${.PARSEDIR}/../Makefile.inc)
.include "${.PARSEDIR}/../Makefile.inc"
.endif
.endif
