# $NetBSD: Makefile,v 1.4 2011/02/20 20:18:57 jmmv Exp $

.include <bsd.own.mk>

SRCDIR=		${NETBSDSRCDIR}/external/bsd/atf/dist
.PATH:		${SRCDIR}/atf-sh

BINDIR=		/usr/bin
PROG_CXX=	atf-sh
SRCS=		atf-sh.cpp
MAN=		atf-sh.1 atf-sh-api.3

LDADD+=		-latf-c++ -latf-c
DPADD+=		${LIBATF_CXX} ${LIBATF_C}

CPPFLAGS+=	-DHAVE_CONFIG_H
CPPFLAGS+=	-I${SRCDIR}

WARNS?=		2

.if ${MKSHARE} != "no"
FILESDIR=	/usr/share/atf
FILESMODE=	444
FILES+=		libatf-sh.subr
.endif

.include <bsd.prog.mk>
