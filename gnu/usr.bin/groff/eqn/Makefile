#	$Id: Makefile,v 1.10 1997/03/24 22:06:28 christos Exp $

PROG=		eqn
SRCS=		main.cc lex.cc  box.cc limit.cc list.cc over.cc text.cc\
		script.cc mark.cc other.cc delim.cc sqrt.cc pile.cc special.cc\
		eqn.cc
CFLAGS+=	-I. -I$(.CURDIR)/../include
LDADD+=		$(LIBGROFF)
DPADD+=		$(LIBGROFF)

CLEANFILES+=	eqn.cc eqn.tab.h neqn

lex.o:	eqn.cc

FILES=neqn
FILESMODE=${BINMODE}

all: neqn

neqn:	neqn.sh
	sed -e 's/@g@/$(g)/g' ${.ALLSRC} > ${.TARGET}

.include "../../Makefile.inc"
.include "../Makefile.cfg"
.include <bsd.prog.mk>
