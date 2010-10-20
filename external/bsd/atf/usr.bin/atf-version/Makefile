# $NetBSD: Makefile,v 1.6 2010/10/20 09:20:13 jmmv Exp $

.include <bsd.own.mk>

SRCDIR=		${NETBSDSRCDIR}/external/bsd/atf/dist
.PATH:		${SRCDIR}/admin ${SRCDIR}/atf-version

BINDIR=		/usr/bin
PROG_CXX=	atf-version
SRCS=		atf-version.cpp
MAN=		atf-version.1

CPPFLAGS+=	-DHAVE_CONFIG_H
CPPFLAGS+=	-I${SRCDIR}
CPPFLAGS+=	-I${.CURDIR}/../../lib/libatf-c
CPPFLAGS+=	-I${.CURDIR}
LDADD+=		-latf-c++ -latf-c

WARNS?=		2

.include <bsd.prog.mk>
