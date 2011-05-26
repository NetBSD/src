# $NetBSD: Makefile,v 1.7 2011/05/26 12:56:29 joerg Exp $

# $FreeBSD: src/gnu/usr.bin/groff/src/utils/hpftodit/Makefile,v 1.4 2003/05/01 13:22:20 ru Exp $

DIST_SUBDIR=	src/utils/hpftodit

PROG=		hpftodit
SRCS=		hpftodit.cpp hpuni.cpp
CLEANFILES=	${MAN}

CWARNFLAGS.clang+=	-Wno-format-security

.include <bsd.prog.mk>
