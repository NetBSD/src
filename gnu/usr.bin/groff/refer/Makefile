# Makefile for refer

PROG=		refer
SRCS=		command.cc ref.cc refer.cc token.cc label.cc
CFLAGS+=	-I. -I$(.CURDIR)/../include
LDADD+=		$(LIBBIB) $(LIBGROFF) -lm
DPADD+=		$(LIBBIB) $(LIBGROFF) $(LIBMATH)

CLEANFILES+=	label.cc label.tab.h

.include "../../Makefile.inc"
.include "../Makefile.cfg"
.include <bsd.prog.mk>
