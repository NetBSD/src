# $NetBSD: Makefile,v 1.2 2010/05/08 08:12:35 jmmv Exp $

.include <bsd.own.mk>

SRCDIR=		${NETBSDSRCDIR}/external/bsd/atf/dist
.PATH:		${SRCDIR}/atf-check

BINDIR=		/usr/bin
PROG_CXX=	atf-check
SRCS=		atf-check.cpp
MAN=		atf-check.1

LDADD+=		-latf-c++ -latf-c

WARNS?=		2

.include <bsd.prog.mk>
