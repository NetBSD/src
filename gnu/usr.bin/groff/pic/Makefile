#	$Id: Makefile,v 1.8 1993/08/02 17:44:44 mycroft Exp $

PROG=		pic
SRCS=		lex.cc main.cc object.cc common.cc troff.cc tex.cc pic.cc
CFLAGS+=	-I. -I$(.CURDIR)/../include
LDADD+=		$(LIBGROFF) -lm
DPADD+=		$(LIBGROFF) $(LIBMATH)

CLEANFILES+=	pic.cc pic.tab.h

lex.o:	pic.cc

.include "../../Makefile.inc"
.include "../Makefile.cfg"
.include <bsd.prog.mk>
