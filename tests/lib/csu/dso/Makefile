# $NetBSD: Makefile,v 1.1 2010/07/28 13:51:40 joerg Exp $

NOMAN=		# defined

.include <bsd.own.mk>

LIB=		h_initfini3_dso
SRCS=		h_initfini3_dso.cxx

TESTSBASE=	/usr/tests
LIBDIR=		${TESTSBASE}/lib/csu
SHLIBDIR=	${TESTSBASE}/lib/csu
SHLIB_MAJOR=	1

MKSTATICLIB=	no
MKPROFILE=	no
MKPICINSTALL=	no

LIBISCXX=	yes

.include <bsd.lib.mk>
