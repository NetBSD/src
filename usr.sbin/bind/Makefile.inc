#	$NetBSD: Makefile.inc,v 1.25.4.1 2007/06/03 17:26:11 wrstuden Exp $

.if !defined(BIND9_MAKEFILE_INC)
BIND9_MAKEFILE_INC=yes

WARNS?=	1

.include <bsd.own.mk>

.if ${MKCRYPTO} == "no"
NAMED_USE_OPENSSL?=no
.endif

.if (${MACHINE_ARCH} == "sparc" || ${MACHINE_ARCH} == "sparc64")
NAMED_USE_PTHREADS?=no
.else
NAMED_USE_PTHREADS?=yes
.endif

NAMED_USE_OPENSSL?=yes

IDIST=		${NETBSDSRCDIR}/dist/bind
BIND_SRCDIR=	${NETBSDSRCDIR}/usr.sbin/bind
BIND_HTMLDIR=	/usr/share/doc/html/bind9

.include "${IDIST}/version"

VERSION=${MAJORVER}.${MINORVER}.${PATCHVER}${RELEASETYPE}${RELEASEVER}
SYSCONFDIR=/etc
LOCALSTATEDIR=/var

CPPFLAGS+=-I${BIND_SRCDIR}/include \
	-I${IDIST} \
	-I${IDIST}/lib/dns/include -I${IDIST}/lib/dns/unix/include \
	-I${IDIST}/lib/isc/include -I${IDIST}/lib/isc/unix/include \
	-I${IDIST}/lib/bind9/include \
	-I${IDIST}/lib/isccfg/include \
	-I${IDIST}/lib/isccc/include \
	-I${IDIST}/lib/lwres/include -I${IDIST}/lib/lwres/unix/include \
	-I${IDIST}/lib/dns/sec/dst/include \
	-DNS_LOCALSTATEDIR=\"${LOCALSTATEDIR}\" \
	-DNS_SYSCONFDIR=\"${SYSCONFDIR}\" \
	-DVERSION=\"${VERSION}\"

.if (${USE_INET6} != "no")
CPPFLAGS+=	-DWANT_IPV6
.endif

.if ${HAVE_GCC} == 4
COPTS+=	-Wno-pointer-sign
.endif

.if !defined(LIB) || empty(LIB)						# {
# NOTE: the order of these libraries is important...
.for _LL in	bind9 dns lwres isccfg isccc isc
.if !defined(BOBJDIR.${_LL})
BOBJDIR.${_LL}!=cd ${BIND_SRCDIR}/lib${_LL} && ${PRINTOBJDIR}
.MAKEOVERRIDES+=BOBJDIR.${_LL}
.endif
LDADD+=		-L${BOBJDIR.${_LL}} -l${_LL}
DPADD+=		${BOBJDIR.${_LL}}/lib${_LL}.a
.endfor

.if ${NAMED_USE_PTHREADS} == "yes"
LDADD+= -lpthread
DPADD+= ${LIBPTHREAD}
.endif
.else									# }
MKPRIVATELIB=yes
CPPFLAGS+= -DLIBINTERFACE=${LIBINTERFACE} \
	-DLIBREVISION=${LIBREVISION} -DLIBAGE=${LIBAGE}
.endif

.if ${NAMED_USE_OPENSSL} == "yes"
CPPFLAGS+=-DOPENSSL
LDADD+= -lcrypto
DPADD+= ${LIBCRYPTO}
.endif

.if ${NAMED_USE_PTHREADS} == "yes"
CPPFLAGS+=-DISC_PLATFORM_USETHREADS -I${IDIST}/lib/isc/pthreads/include
.else
CPPFLAGS+=-I${IDIST}/lib/isc/nothreads/include
.endif

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
.endif
