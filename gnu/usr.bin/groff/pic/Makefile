#	$NetBSD: Makefile,v 1.16 2002/01/01 01:38:29 thorpej Exp $

.include "../Makefile.lib"

DIST=		${IDIST}/src/preproc/pic
.PATH:		${DIST}

PROG_CXX=	pic
SRCS=		lex.cc main.cc object.cc common.cc troff.cc tex.cc pic.cc
CPPFLAGS+=	-I. -I${DIST}
LDADD+=		$(LIBGROFF) -lm
DPADD+=		$(LIBGROFF) $(LIBM)
DPSRCS+=	pic_tab.h
CLEANFILES+=	pic.cc

lex.o pic_tab.h: pic.cc

.include <bsd.prog.mk>
