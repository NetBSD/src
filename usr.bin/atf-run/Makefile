# $NetBSD: Makefile,v 1.2 2007/11/28 08:20:29 jnemeth Exp $

.include <bsd.own.mk>

SRCDIR=		${NETBSDSRCDIR}/dist/atf
.PATH:		${SRCDIR}/tools

PROG_CXX=	atf-run
SRCS=		atf-run.cpp
MAN=		atf-run.1

CPPFLAGS+=	-DHAVE_CONFIG_H
CPPFLAGS+=	-I${SRCDIR}/libs
CPPFLAGS+=	-I${.CURDIR}/../../lib/libatf
LDADD+=		-latf

COPTS.atf-run.cpp+=	-Wno-stack-protector

WARNS?=		2

FILESDIR=	/usr/share/atf
FILES=		atf-run.hooks

.include <bsd.prog.mk>
