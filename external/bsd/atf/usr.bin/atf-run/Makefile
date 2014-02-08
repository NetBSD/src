# $NetBSD: Makefile,v 1.12 2014/02/08 19:15:34 jmmv Exp $

.include <bsd.own.mk>

SRCDIR=		${NETBSDSRCDIR}/external/bsd/atf/dist
.PATH:		${SRCDIR}/tools
.PATH:		${SRCDIR}/tools/share

BINDIR=		/usr/bin
PROG_CXX=	atf-run
SRCS=		atf-run.cpp
MAN=		atf-run.1

CPPFLAGS+=	-DGDB=\"/usr/bin/gdb\"
CPPFLAGS+=	-DHAVE_CONFIG_H
CPPFLAGS+=	-I${SRCDIR}/tools
CPPFLAGS+=	-I${SRCDIR:H}/lib/libatf-c  # For bconfig.h.

USE_ATF_LIBTOOLS=	yes

CWARNFLAGS.clang+=	-Wno-error=unused-private-field

COPTS.atf-run.cpp+=	-Wno-stack-protector

WARNS?=		2

.if ${MKSHARE} != "no"
FILESDIR=	/usr/share/atf
FILESMODE=	444
FILES=		atf-run.hooks
.endif

.include <bsd.prog.mk>
