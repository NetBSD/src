#	$NetBSD: Makefile.inc,v 1.18 2004/05/23 03:16:40 lukem Exp $

.if !defined(BIND9_MAKEFILE_INC)
BIND9_MAKEFILE_INC=yes

WARNS?=	1
WFORMAT?=2
NAMED_USE_PTHREADS?=yes

.include <bsd.own.mk>

IDIST=		${NETBSDSRCDIR}/dist/bind
BIND_SRCDIR=	${NETBSDSRCDIR}/usr.sbin/bind
.include "${IDIST}/version"

VERSION=${MAJORVER}.${MINORVER}.${PATCHVER}${RELEASETYPE}${RELEASEVER}
SYSCONFDIR=/etc
LOCALSTATEDIR=/var

CPPFLAGS+=-I${BIND_SRCDIR}/include \
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

.if ${NAMED_USE_PTHREADS} == "yes"
CPPFLAGS+=-DISC_PLATFORM_USETHREADS -I${IDIST}/lib/isc/pthreads/include
.else
CPPFLAGS+=-I${IDIST}/lib/isc/nothreads/include
.endif

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
.endif
