#	$Id: Makefile,v 1.9 1995/10/26 00:59:09 jtc Exp $

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
