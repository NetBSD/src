# $NetBSD: Makefile.inc,v 1.1.1.1 2024/08/25 06:38:34 wiz Exp $

DIST=	${NETBSDSRCDIR}/external/bsd/pkg_install/dist

USE_FORT?=yes	# network client

CPPFLAGS+=-I${DIST}/lib
CPPFLAGS+=-I${NETBSDSRCDIR}/external/bsd/pkg_install/lib
CPPFLAGS+=-DHAVE_CONFIG_H -DNETBSD -DHAVE_SSL
CPPFLAGS+=-DSYSCONFDIR='"/etc"'

WARNS=	4
CWARNFLAGS+=	-Wno-missing-noreturn

.include <bsd.own.mk>

# show_version() does not return
COPTS.main.c+=		${CC_WNO_IMPLICIT_FALLTHROUGH}
COPTS.pkg_delete.c+=	${CC_WNO_IMPLICIT_FALLTHROUGH}
