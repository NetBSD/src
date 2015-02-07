# $FreeBSD: src/cddl/Makefile.inc,v 1.6.2.1 2009/08/03 08:13:06 kensmith Exp $

WARNS?=5
OSNETDIR=	${NETBSDSRCDIR}/external/cddl/osnet

OPENSOLARIS_USR_DISTDIR=${OSNETDIR}/dist
OPENSOLARIS_SYS_DISTDIR=${OSNETDIR}/dist
CPPFLAGS+=-Wno-unknown-pragmas -Wno-sign-compare
