# $NetBSD: Makefile,v 1.4 2003/07/01 08:26:30 wiz Exp $

# $FreeBSD: src/gnu/usr.bin/groff/src/preproc/eqn/Makefile,v 1.4 2003/05/01 13:22:19 ru Exp $

DIST_SUBDIR=	src/preproc/eqn

PROG_CXX=	eqn
SRCS=		eqn.cpp eqn_tab.h main.cpp lex.cpp \
		box.cpp limit.cpp list.cpp over.cpp text.cpp \
		script.cpp mark.cpp other.cpp delim.cpp sqrt.cpp pile.cpp special.cpp
CFLAGS+=	-I${DIST_DIR} -I.
DPADD+=		${LIBGROFF}
LDADD+=		${LIBGROFF}
SCRIPTS=	neqn
MAN=		eqn.1 neqn.1
# YHEADER is useless here; eqn.y -> eqn_tab.h and eqn.cpp.
YFLAGS=		-d
CLEANFILES=	eqn.cpp eqn_tab.h ${SCRIPTS} ${MAN} y.tab.c y.tab.h

eqn_tab.h: eqn.cpp

.include <bsd.prog.mk>
