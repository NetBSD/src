# $NetBSD: Makefile,v 1.2.8.1 2008/05/18 12:36:04 yamt Exp $

.include <bsd.own.mk>

SRCDIR=		${NETBSDSRCDIR}/dist/atf
.PATH:		${SRCDIR}/tools

PROG_CXX=	atf-run
SRCS=		atf-run.cpp
MAN=		atf-run.1

CPPFLAGS+=	-DHAVE_CONFIG_H
CPPFLAGS+=	-I${.CURDIR}/../../lib/libatf-c
LDADD+=		-latf-c++ -latf-c

COPTS.atf-run.cpp+=	-Wno-stack-protector

WARNS?=		2

FILESDIR=	/usr/share/atf
FILES=		atf-run.hooks

.include <bsd.prog.mk>
