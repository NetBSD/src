# $NetBSD: Makefile,v 1.2.14.1 2018/03/15 09:11:55 pgoyette Exp $

# $FreeBSD: src/gnu/usr.bin/groff/src/preproc/pic/Makefile,v 1.4 2003/05/01 13:22:20 ru Exp $

GROFFYSRCS=	pic.y
DIST_SUBDIR=	src/preproc/pic
NOLIBDRIVER=

.include <bsd.init.mk>

PROG_CXX=	pic
SRCS=		lex.cpp main.cpp object.cpp pic.cpp \
		common.cpp troff.cpp tex.cpp
CPPFLAGS+=	-I${DIST_DIR} -I.
CLEANFILES+=	${MAN}

.include <bsd.prog.mk>
