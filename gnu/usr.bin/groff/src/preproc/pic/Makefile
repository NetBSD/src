# $NetBSD: Makefile,v 1.8 2003/10/20 00:54:37 lukem Exp $

# $FreeBSD: src/gnu/usr.bin/groff/src/preproc/pic/Makefile,v 1.4 2003/05/01 13:22:20 ru Exp $

DIST_SUBDIR=	src/preproc/pic

PROG_CXX=	pic
SRCS=		lex.cpp main.cpp object.cpp \
		common.cpp troff.cpp tex.cpp
GROFFYSRCS=	pic.y
CFLAGS+=	-I${DIST_DIR} -I.
DPADD+=		${LIBGROFF} ${LIBM}
LDADD+=		${LIBGROFF} -lm
CLEANFILES=	${MAN}

.include <bsd.prog.mk>
