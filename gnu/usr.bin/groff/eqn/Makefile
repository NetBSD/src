# Makefile for eqn

PROG=		eqn
SRCS=		main.cc lex.cc  box.cc limit.cc list.cc over.cc text.cc\
		script.cc mark.cc other.cc delim.cc sqrt.cc pile.cc special.cc\
		eqn.cc
CFLAGS+=	-I. -I$(.CURDIR)/../include
LDADD+=		$(LIBGROFF)
DPADD+=		$(LIBGROFF)

CLEANFILES+=	eqn.cc eqn.tab.h

afterinstall:
	sed -e 's/@g@/$(g)/g' $(.CURDIR)/neqn.sh > $(DESTDIR)$(BINDIR)/neqn
	chown ${BINOWN}.${BINGRP} $(DESTDIR)$(BINDIR)/neqn
	chmod ${BINMODE} $(DESTDIR)$(BINDIR)/neqn

.include "../../Makefile.inc"
.include "../Makefile.cfg"
.include <bsd.prog.mk>
