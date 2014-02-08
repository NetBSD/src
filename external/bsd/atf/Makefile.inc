# $NetBSD: Makefile.inc,v 1.2 2014/02/08 19:15:33 jmmv Exp $

TOPDIR=		${NETBSDSRCDIR}/external/bsd/atf
SRCDIR=		${TOPDIR}/dist

ATF_VERSION!=	grep 'define VERSION' ${TOPDIR}/lib/libatf-c/bconfig.h \
		| cut -d '"' -f 2

.if defined(USE_ATF_LIBTOOLS)
PRIVATELIBDIR!= cd ${TOPDIR}/lib; ${PRINTOBJDIR}
CPPFLAGS+=	-I${PRIVATELIBDIR}/tools
LDADD+=		${PRIVATELIBDIR}/tools/libtools.a
DPADD+=		${PRIVATELIBDIR}/tools/libtools.a
.endif
