# $NetBSD: Makefile,v 1.1 2010/06/04 08:33:42 jmmv Exp $

.include <bsd.own.mk>

SRCDIR=		${NETBSDSRCDIR}/external/bsd/atf/dist
.PATH:		${SRCDIR}/atf-sh

BINDIR=		/usr/bin
PROG_CXX=	atf-sh
SRCS=		atf-sh.cpp
MAN=		atf-sh.1 atf-sh-api.3

LDADD+=		-latf-c++ -latf-c

WARNS?=		2

.if ${MKSHARE} != "no"
FILESDIR=	/usr/share/atf
FILESMODE=	444
FILES+=		libatf-sh.subr
.endif

.include <bsd.prog.mk>
