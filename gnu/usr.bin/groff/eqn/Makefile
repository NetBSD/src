#	$NetBSD: Makefile,v 1.13 2000/01/23 22:17:11 mycroft Exp $

PROG=		eqn
SRCS=		main.cc lex.cc  box.cc limit.cc list.cc over.cc text.cc\
		script.cc mark.cc other.cc delim.cc sqrt.cc pile.cc special.cc\
		eqn.cc
CPPFLAGS+=	-I. -I$(.CURDIR)/../include
LDADD+=		$(LIBGROFF)
DPADD+=		$(LIBGROFF)

CLEANFILES+=	eqn.cc eqn.tab.h neqn

lex.o:	eqn.cc

FILES=neqn
FILESMODE=${BINMODE}

realall: neqn

neqn:	neqn.sh
	sed -e 's/@g@/$(g)/g' ${.ALLSRC} > ${.TARGET}

.include "../../Makefile.inc"
.include "../Makefile.cfg"
.include <bsd.prog.mk>
