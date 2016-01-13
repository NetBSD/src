# $NetBSD: Makefile,v 1.1 2016/01/13 19:01:56 christos Exp $

# $FreeBSD: src/gnu/usr.bin/groff/src/utils/hpftodit/Makefile,v 1.4 2003/05/01 13:22:20 ru Exp $

DIST_SUBDIR=	src/utils/hpftodit

.include <bsd.init.mk>

PROG_CXX=	hpftodit
SRCS=		hpftodit.cpp hpuni.cpp
CLEANFILES=	${MAN}

CWARNFLAGS.clang+=	-Wno-format-security

.include <bsd.prog.mk>
