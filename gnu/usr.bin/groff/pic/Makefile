#	$NetBSD: Makefile,v 1.11 1997/10/25 10:12:19 lukem Exp $

PROG=		pic
SRCS=		lex.cc main.cc object.cc common.cc troff.cc tex.cc pic.cc
CPPFLAGS+=	-I. -I$(.CURDIR)/../include
LDADD+=		$(LIBGROFF) -lm
DPADD+=		$(LIBGROFF) $(LIBM)

CLEANFILES+=	pic.cc pic.tab.h

lex.o:	pic.cc

.include "../../Makefile.inc"
.include "../Makefile.cfg"
.include <bsd.prog.mk>
