# $NetBSD: Makefile,v 1.8 2012/08/10 12:12:19 joerg Exp $

# $FreeBSD: src/gnu/usr.bin/groff/src/devices/grohtml/Makefile,v 1.5 2003/05/01 13:22:18 ru Exp $

DIST_SUBDIR=	src/devices/grohtml

PROG_CXX=	post-grohtml
MAN=		grohtml.1
SRCS=		post-html.cpp html-table.cpp html-text.cpp output.cpp
CLEANFILES=	${MAN}

CWARNFLAGS.clang+=	-Wno-format-security

.include <bsd.prog.mk>
