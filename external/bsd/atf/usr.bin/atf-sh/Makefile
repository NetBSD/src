# $NetBSD: Makefile,v 1.2 2010/07/03 08:14:13 jmmv Exp $

.include <bsd.own.mk>

SRCDIR=		${NETBSDSRCDIR}/external/bsd/atf/dist
.PATH:		${SRCDIR}/atf-sh

BINDIR=		/usr/bin
PROG_CXX=	atf-sh
SRCS=		atf-sh.cpp
MAN=		atf-sh.1 atf-sh-api.3

LDADD+=		-latf-c++ -latf-c

CPPFLAGS+=	-DHAVE_CONFIG_H

WARNS?=		2

.if ${MKSHARE} != "no"
FILESDIR=	/usr/share/atf
FILESMODE=	444
FILES+=		libatf-sh.subr
.endif

.include <bsd.prog.mk>
