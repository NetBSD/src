# Makefile for pic

PROG=		pic
SRCS=		lex.cc main.cc object.cc common.cc troff.cc tex.cc pic.cc
CFLAGS+=	-I. -I$(.CURDIR)/../include
LDADD+=		$(LIBGROFF) -lm
DPADD+=		$(LIBGROFF) $(LIBMATH)

CLEANFILES+=	pic.cc pic.tab.h

.include <bsd.prog.mk>
.include "../../Makefile.inc"
.include "../Makefile.cfg"
