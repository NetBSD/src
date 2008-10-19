# $NetBSD: Makefile.inc,v 1.1.4.2 2008/10/19 22:40:48 haad Exp $

DIST=	${NETBSDSRCDIR}/external/bsd/pkg_install/dist

USE_FORT?=yes	# network client

CPPFLAGS+=-I${DIST}/lib
CPPFLAGS+=-I${NETBSDSRCDIR}/external/bsd/pkg_install/lib
CPPFLAGS+=-DHAVE_CONFIG_H -DNETBSD
CPPFLAGS+=-DSYSCONFDIR='"/etc"'

WARNS=	2
