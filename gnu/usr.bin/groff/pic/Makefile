#	$NetBSD: Makefile,v 1.13 2001/11/12 17:09:46 tv Exp $

.include "../Makefile.lib"

DIST=		${IDIST}/src/preproc/pic
.PATH:		${DIST}

PROG=		pic
SRCS=		lex.cc main.cc object.cc common.cc troff.cc tex.cc pic.cc
CPPFLAGS+=	-I${DIST}
LDADD+=		$(LIBGROFF) -lm
DPADD+=		$(LIBGROFF) $(LIBM)

CLEANFILES+=	pic.cc pic_tab.h

lex.o:	pic.cc

.include <bsd.prog.mk>
