#	$NetBSD: Makefile,v 1.14 2001/11/13 17:01:24 tv Exp $

.include "../Makefile.lib"

DIST=		${IDIST}/src/preproc/pic
.PATH:		${DIST}

PROG=		pic
SRCS=		lex.cc main.cc object.cc common.cc troff.cc tex.cc pic.cc
CPPFLAGS+=	-I. -I${DIST}
LDADD+=		$(LIBGROFF) -lm
DPADD+=		$(LIBGROFF) $(LIBM)
DPSRCS+=	pic_tab.h
CLEANFILES+=	pic.cc

lex.o pic_tab.h: pic.cc

.include <bsd.prog.mk>
