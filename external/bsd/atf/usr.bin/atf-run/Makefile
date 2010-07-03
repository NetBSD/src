# $NetBSD: Makefile,v 1.7 2010/07/03 08:14:13 jmmv Exp $

.include <bsd.own.mk>

SRCDIR=		${NETBSDSRCDIR}/external/bsd/atf/dist
.PATH:		${SRCDIR}/atf-run
.PATH:		${SRCDIR}/atf-run/share
.PATH:		${.CURDIR}/../../lib/libatf-c

BINDIR=		/usr/bin
PROG_CXX=	atf-run
SRCS=		atf-run.cpp atffile.cpp config.cpp fs.cpp requirements.cpp \
		test-program.cpp timer.cpp
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
