# $NetBSD: Makefile,v 1.9.6.1 2012/10/30 18:58:30 yamt Exp $

# $FreeBSD: src/gnu/usr.bin/groff/src/preproc/eqn/Makefile,v 1.4 2003/05/01 13:22:19 ru Exp $

DIST_SUBDIR=	src/preproc/eqn

PROG_CXX=	eqn
SRCS=		main.cpp lex.cpp \
		box.cpp limit.cpp list.cpp over.cpp text.cpp \
		script.cpp mark.cpp other.cpp delim.cpp sqrt.cpp pile.cpp special.cpp
GROFFYSRCS=	eqn.y
CFLAGS+=	-I${DIST_DIR} -I.
SCRIPTS=	neqn
MAN=		eqn.1 neqn.1
CLEANFILES=	${SCRIPTS} ${MAN}

.include <bsd.prog.mk>
