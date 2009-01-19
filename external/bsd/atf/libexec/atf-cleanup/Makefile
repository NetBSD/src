# $NetBSD: Makefile,v 1.1 2009/01/19 07:13:04 jmmv Exp $

.include <bsd.own.mk>

SRCDIR=		${NETBSDSRCDIR}/external/bsd/atf/dist
.PATH:		${SRCDIR}/tools

BINDIR=		/usr/libexec
PROG_CXX=	atf-cleanup
SRCS=		atf-cleanup.cpp
MAN=		atf-cleanup.1

LDADD+=		-latf-c++ -latf-c

WARNS?=		2

.include <bsd.prog.mk>
