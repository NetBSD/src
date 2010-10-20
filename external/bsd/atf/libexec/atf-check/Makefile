# $NetBSD: Makefile,v 1.1 2010/10/20 09:20:10 jmmv Exp $

.include <bsd.own.mk>

SRCDIR=		${NETBSDSRCDIR}/external/bsd/atf/dist
.PATH:		${SRCDIR}/atf-sh

BINDIR=		/usr/libexec
PROG_CXX=	atf-check
SRCS=		atf-check.cpp
MAN=		atf-check.1

LDADD+=		-latf-c++ -latf-c

CPPFLAGS+=	-DHAVE_CONFIG_H
CPPFLAGS+=	-I${SRCDIR}

WARNS?=		2

.include <bsd.prog.mk>
