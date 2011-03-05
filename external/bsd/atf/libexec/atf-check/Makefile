# $NetBSD: Makefile,v 1.1.2.1 2011/03/05 15:08:45 bouyer Exp $

.include <bsd.own.mk>

SRCDIR=		${NETBSDSRCDIR}/external/bsd/atf/dist
.PATH:		${SRCDIR}/atf-sh

BINDIR=		/usr/libexec
PROG_CXX=	atf-check
SRCS=		atf-check.cpp
MAN=		atf-check.1

LDADD+=		-latf-c++ -latf-c
DPADD+=		${LIBATF_CXX} ${LIBATF_C}

CPPFLAGS+=	-DHAVE_CONFIG_H
CPPFLAGS+=	-I${SRCDIR}

WARNS?=		2

.include <bsd.prog.mk>
