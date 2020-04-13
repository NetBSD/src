# $NetBSD: Makefile.inc,v 1.4.42.2 2020/04/13 07:56:29 martin Exp $

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
.if defined(HAVE_GCC) && ${HAVE_GCC} >= 7 && ${ACTIVE_CC} == "gcc"
COPTS.main.c+=		-Wno-error=implicit-fallthrough
COPTS.pkg_delete.c+=	-Wno-error=implicit-fallthrough
.endif
