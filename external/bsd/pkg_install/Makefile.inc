# $NetBSD: Makefile.inc,v 1.1 2008/09/30 19:19:56 joerg Exp $

DIST=	${NETBSDSRCDIR}/external/bsd/pkg_install/dist

USE_FORT?=yes	# network client

CPPFLAGS+=-I${DIST}/lib
CPPFLAGS+=-I${NETBSDSRCDIR}/external/bsd/pkg_install/lib
CPPFLAGS+=-DHAVE_CONFIG_H -DNETBSD
CPPFLAGS+=-DSYSCONFDIR='"/etc"'

WARNS=	2
