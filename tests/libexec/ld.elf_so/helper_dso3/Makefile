# $NetBSD: Makefile,v 1.1.4.2 2017/08/29 09:43:19 bouyer Exp $

.include <bsd.own.mk>

LIB=			h_helper_dso3
LIBISCXX=	yes
SRCS=			h_helper_dso3.cpp

LIBDIR=		${TESTSBASE}/libexec/ld.elf_so
SHLIBDIR=	${TESTSBASE}/libexec/ld.elf_so
SHLIB_MAJOR=	1

MKSTATICLIB=	no
MKPROFILE=	no
MKPICINSTALL=	no
MKLINT=		no

NOMAN=		# defined

CXXFLAGS+=	-std=c++11

.include <bsd.lib.mk>
