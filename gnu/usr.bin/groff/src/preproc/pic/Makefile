# $NetBSD: Makefile,v 1.10 2010/12/16 22:54:20 joerg Exp $

# $FreeBSD: src/gnu/usr.bin/groff/src/preproc/pic/Makefile,v 1.4 2003/05/01 13:22:20 ru Exp $

DIST_SUBDIR=	src/preproc/pic

PROG=		pic
SRCS=		lex.cpp main.cpp object.cpp \
		common.cpp troff.cpp tex.cpp
GROFFYSRCS=	pic.y
CFLAGS+=	-I${DIST_DIR} -I.
CLEANFILES=	${MAN}

.include <bsd.prog.mk>
