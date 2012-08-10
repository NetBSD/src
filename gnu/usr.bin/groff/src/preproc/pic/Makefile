# $NetBSD: Makefile,v 1.11 2012/08/10 12:12:20 joerg Exp $

# $FreeBSD: src/gnu/usr.bin/groff/src/preproc/pic/Makefile,v 1.4 2003/05/01 13:22:20 ru Exp $

DIST_SUBDIR=	src/preproc/pic

PROG_CXX=	pic
SRCS=		lex.cpp main.cpp object.cpp \
		common.cpp troff.cpp tex.cpp
GROFFYSRCS=	pic.y
CFLAGS+=	-I${DIST_DIR} -I.
CLEANFILES=	${MAN}

.include <bsd.prog.mk>
