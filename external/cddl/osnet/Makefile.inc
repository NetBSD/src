# $FreeBSD: head/cddl/Makefile.inc 270358 2014-08-22 20:04:51Z delphij $

WARNS?=5

OSNETDIR=	${NETBSDSRCDIR}/external/cddl/osnet

OPENSOLARIS_USR_DISTDIR=${OSNETDIR}/dist
OPENSOLARIS_SYS_DISTDIR=${OSNETDIR}/dist

CPPFLAGS+=-Wno-unknown-pragmas -Wno-sign-compare -D_KERNTYPES
