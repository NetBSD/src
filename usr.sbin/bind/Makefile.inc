#	$NetBSD: Makefile.inc,v 1.16 2004/05/18 00:10:39 christos Exp $

.if !defined(BIND9_MAKEFILE_INC)
BIND9_MAKEFILE_INC=yes

WARNS?=	1
WFORMAT?=2
NAMED_USE_PTHREADS?=yes

.include <bsd.own.mk>

IDIST=	${NETBSDSRCDIR}/dist/bind
.include "${IDIST}/version"

VERSION=${MAJORVER}.${MINORVER}.${PATCHVER}${RELEASETYPE}${RELEASEVER}
SYSCONFDIR=/etc
LOCALSTATEDIR=/var

CPPFLAGS+=-I${.CURDIR}/../include \
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

.if !defined(LIB) || empty(LIB)
LIBBIND9OBJDIR!=cd ${.CURDIR}/../libbind9 && ${PRINTOBJDIR}
LDADD+=	-L${LIBBIND9OBJDIR} -lbind9
DPADD+=	${LIBBIND9OBJDIR}/libbind9.a
LIBDNSOBJDIR!=cd ${.CURDIR}/../libdns && ${PRINTOBJDIR}
LDADD+=	-L${LIBDNSOBJDIR} -ldns
DPADD+=	${LIBDNSOBJDIR}/libdns.a
LIBLWRESOBJDIR!=cd ${.CURDIR}/../liblwres && ${PRINTOBJDIR}
LDADD+=	-L${LIBLWRESOBJDIR} -llwres
DPADD+=	${LIBLWRESOBJDIR}/liblwres.a
LIBISCCFGOBJDIR!=cd ${.CURDIR}/../libisccfg && ${PRINTOBJDIR}
LDADD+=	-L${LIBISCCFGOBJDIR} -lisccfg
DPADD+=	${LIBISCCFGOBJDIR}/libisccfg.a
LIBISCCCOBJDIR!=cd ${.CURDIR}/../libisccc && ${PRINTOBJDIR}
LDADD+=	-L${LIBISCCCOBJDIR} -lisccc
DPADD+=	${LIBISCCCOBJDIR}/libisccc.a
LIBISCOBJDIR!=cd ${.CURDIR}/../libisc && ${PRINTOBJDIR}
LDADD+=	-L${LIBISCOBJDIR} -lisc
DPADD+=	${LIBISCOBJDIR}/libisc.a
.if ${NAMED_USE_PTHREADS} == "yes"
LDADD+= -lpthread
DPADD+= ${LIBPTHREAD}
.endif
.else
MKPIC=no
MKPROFILE=no
MKLINT=no
MKLINKLIB=no
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
