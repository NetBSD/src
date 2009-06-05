# $NetBSD: Makefile.inc,v 1.2.4.2 2009/06/05 17:01:57 snj Exp $

DIST=	${NETBSDSRCDIR}/external/bsd/pkg_install/dist

USE_FORT?=yes	# network client

CPPFLAGS+=-I${DIST}/lib
CPPFLAGS+=-I${NETBSDSRCDIR}/external/bsd/pkg_install/lib
CPPFLAGS+=-DHAVE_CONFIG_H -DNETBSD -DHAVE_SSL
CPPFLAGS+=-DSYSCONFDIR='"/etc"'

WARNS=	2
