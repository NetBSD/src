#	$NetBSD: Makefile,v 1.12 2001/04/19 14:41:34 wiz Exp $

.include "../Makefile.lib"

DIST=		${IDIST}/src/preproc/pic
.PATH:		${DIST}

PROG=		pic
SRCS=		lex.cc main.cc object.cc common.cc troff.cc tex.cc pic.cc
CPPFLAGS+=	-I${DIST}
LDADD+=		$(LIBGROFF) -lm
DPADD+=		$(LIBGROFF) $(LIBM)

CLEANFILES+=	pic.cc pic.tab.h

lex.o:	pic.cc

.include <bsd.prog.mk>
