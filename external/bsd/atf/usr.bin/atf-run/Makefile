# $NetBSD: Makefile,v 1.8.2.1 2011/03/05 15:08:45 bouyer Exp $

.include <bsd.own.mk>

SRCDIR=		${NETBSDSRCDIR}/external/bsd/atf/dist
.PATH:		${SRCDIR}/atf-run
.PATH:		${SRCDIR}/atf-run/share
.PATH:		${.CURDIR}/../../lib/libatf-c

BINDIR=		/usr/bin
PROG_CXX=	atf-run
SRCS=		atf-run.cpp atffile.cpp config.cpp fs.cpp io.cpp requirements.cpp \
		signals.cpp test-program.cpp timer.cpp user.cpp
MAN=		atf-run.1

CPPFLAGS+=	-DHAVE_CONFIG_H
CPPFLAGS+=	-I${SRCDIR}
CPPFLAGS+=	-I${.CURDIR}/../../lib/libatf-c
LDADD+=		-latf-c++ -latf-c
DPADD+=		${LIBATF_CXX} ${LIBATF_C}

COPTS.atf-run.cpp+=	-Wno-stack-protector

WARNS?=		2

.if ${MKSHARE} != "no"
FILESDIR=	/usr/share/atf
FILESMODE=	444
FILES=		atf-run.hooks
.endif

.include <bsd.prog.mk>
