#	$Id: Makefile,v 1.9 1995/06/10 20:37:55 mycroft Exp $

PROG=		eqn
SRCS=		main.cc lex.cc  box.cc limit.cc list.cc over.cc text.cc\
		script.cc mark.cc other.cc delim.cc sqrt.cc pile.cc special.cc\
		eqn.cc
CFLAGS+=	-I. -I$(.CURDIR)/../include
LDADD+=		$(LIBGROFF)
DPADD+=		$(LIBGROFF)

CLEANFILES+=	eqn.cc eqn.tab.h

lex.o:	eqn.cc

afterinstall:
	rm -f ${DESTDIR}${BINDIR}/neqn
	sed -e 's/@g@/$(g)/g' $(.CURDIR)/neqn.sh > $(DESTDIR)$(BINDIR)/neqn
	chown ${BINOWN}:${BINGRP} $(DESTDIR)$(BINDIR)/neqn
	chmod ${BINMODE} $(DESTDIR)$(BINDIR)/neqn

.include "../../Makefile.inc"
.include "../Makefile.cfg"
.include <bsd.prog.mk>
