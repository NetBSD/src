# $NetBSD: Makefile,v 1.8 2003/10/20 00:54:37 lukem Exp $

# $FreeBSD: src/gnu/usr.bin/groff/src/preproc/refer/Makefile,v 1.4 2003/05/01 13:22:20 ru Exp $

DIST_SUBDIR=	src/preproc/refer

PROG_CXX=	refer
SRCS=		command.cpp ref.cpp refer.cpp token.cpp
GROFFYSRCS=	label.y
CFLAGS+=	-I${DIST_DIR}
DPADD+=		${LIBBIB} ${LIBGROFF} ${LIBM}
LDADD+=		${LIBBIB} ${LIBGROFF} -lm
CLEANFILES=	${MAN}

.include <bsd.prog.mk>
