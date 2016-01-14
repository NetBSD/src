#	$NetBSD: Makefile.inc,v 1.1 2016/01/14 00:34:52 christos Exp $

DIST=		${NETBSDSRCDIR}/external/gpl2/texinfo
IDIST=		${DIST}/dist

CPPFLAGS+=	-I${DIST}/include -I${IDIST}/lib \
		-DLOCALEDIR=\"/usr/share/locale\" -DHAVE_CONFIG_H

CWARNFLAGS.clang+=	-Wno-format-security

.if !defined(MAKEDOC)
PROGDPLIBS+=	common ${DIST}/lib

DPADD+=		${LIBINTL}
LDADD+=		-lintl
.endif
