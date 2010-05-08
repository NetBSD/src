# $NetBSD: Makefile,v 1.2 2010/05/08 08:12:33 jmmv Exp $

.include <bsd.own.mk>

SRCDIR=		${NETBSDSRCDIR}/external/bsd/atf/dist
.PATH:		${SRCDIR}/atf-cleanup

BINDIR=		/usr/libexec
PROG_CXX=	atf-cleanup
SRCS=		atf-cleanup.cpp
MAN=		atf-cleanup.1

LDADD+=		-latf-c++ -latf-c

WARNS?=		2

.include <bsd.prog.mk>
