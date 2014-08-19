# $NetBSD: Makefile,v 1.2.10.1 2014/08/19 23:45:55 tls Exp $

.include <bsd.init.mk>

.PATH:		${SRCDIR}/atf-sh

BINDIR=		/usr/libexec
PROG_CXX=	atf-check
SRCS=		atf-check.cpp
MAN=		atf-check.1

LDADD+=		-latf-c++ -latf-c
DPADD+=		${LIBATF_CXX} ${LIBATF_C}

WARNS?=		2

.include <bsd.prog.mk>
