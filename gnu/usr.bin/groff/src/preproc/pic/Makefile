# $NetBSD: Makefile,v 1.9 2006/10/09 08:32:54 mrg Exp $

# $FreeBSD: src/gnu/usr.bin/groff/src/preproc/pic/Makefile,v 1.4 2003/05/01 13:22:20 ru Exp $

DIST_SUBDIR=	src/preproc/pic

PROG=		pic
SRCS=		lex.cpp main.cpp object.cpp \
		common.cpp troff.cpp tex.cpp
GROFFYSRCS=	pic.y
CFLAGS+=	-I${DIST_DIR} -I.
DPADD+=		${LIBGROFF} ${LIBM} ${LIBSUPCXX}
LDADD+=		${LIBGROFF} -lm -lsupc++
CLEANFILES=	${MAN}

.include <bsd.prog.mk>
