# $NetBSD: Makefile,v 1.6 2010/06/04 08:33:42 jmmv Exp $

.include <bsd.own.mk>

SRCDIR=		${NETBSDSRCDIR}/external/bsd/atf/dist
.PATH:		${SRCDIR}/atf-run
.PATH:		${SRCDIR}/atf-run/share

BINDIR=		/usr/bin
PROG_CXX=	atf-run
SRCS=		atf-run.cpp config.cpp requirements.cpp test-program.cpp \
		timer.cpp
MAN=		atf-run.1

CPPFLAGS+=	-DHAVE_CONFIG_H
CPPFLAGS+=	-I${.CURDIR}/../../lib/libatf-c
LDADD+=		-latf-c++ -latf-c

COPTS.atf-run.cpp+=	-Wno-stack-protector

WARNS?=		2

.if ${MKSHARE} != "no"
FILESDIR=	/usr/share/atf
FILESMODE=	444
FILES=		atf-run.hooks
.endif

.include <bsd.prog.mk>
