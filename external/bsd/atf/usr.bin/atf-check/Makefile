# $NetBSD: Makefile,v 1.3 2010/07/03 08:14:13 jmmv Exp $

.include <bsd.own.mk>

SRCDIR=		${NETBSDSRCDIR}/external/bsd/atf/dist
.PATH:		${SRCDIR}/atf-check

BINDIR=		/usr/bin
PROG_CXX=	atf-check
SRCS=		atf-check.cpp
MAN=		atf-check.1

LDADD+=		-latf-c++ -latf-c

CPPFLAGS+=	-DHAVE_CONFIG_H

WARNS?=		2

.include <bsd.prog.mk>
