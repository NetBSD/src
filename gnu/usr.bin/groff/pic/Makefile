#	$NetBSD: Makefile,v 1.10 1997/10/18 05:59:24 lukem Exp $

PROG=		pic
SRCS=		lex.cc main.cc object.cc common.cc troff.cc tex.cc pic.cc
CFLAGS+=	-I. -I$(.CURDIR)/../include
LDADD+=		$(LIBGROFF) -lm
DPADD+=		$(LIBGROFF) $(LIBM)

CLEANFILES+=	pic.cc pic.tab.h

lex.o:	pic.cc

.include "../../Makefile.inc"
.include "../Makefile.cfg"
.include <bsd.prog.mk>
