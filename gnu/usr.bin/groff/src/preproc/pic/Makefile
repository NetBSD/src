# $NetBSD: Makefile,v 1.4 2003/07/01 08:16:54 mrg Exp $

# $FreeBSD: src/gnu/usr.bin/groff/src/preproc/pic/Makefile,v 1.4 2003/05/01 13:22:20 ru Exp $

DIST_SUBDIR=	src/preproc/pic

PROG_CXX=	pic
SRCS=		lex.cpp main.cpp object.cpp pic.cpp \
		common.cpp troff.cpp tex.cpp
CFLAGS+=	-I${DIST_DIR} -I.
DPADD+=		${LIBGROFF} ${LIBM}
LDADD+=		${LIBGROFF} -lm
YFLAGS=		-d
CLEANFILES=	${MAN} pic.cpp pic_tab.h y.tab.c y.tab.h

pic_tab.h: pic.cpp

.include <bsd.prog.mk>
