#	$NetBSD: Makefile,v 1.1.1.2 2001/04/19 14:33:07 wiz Exp $

.include "../Makefile.lib"

DIST=		${IDIST}/src/preproc/eqn
.PATH:		${DIST}

PROG=		eqn
SRCS=		main.cc lex.cc  box.cc limit.cc list.cc over.cc text.cc\
		script.cc mark.cc other.cc delim.cc sqrt.cc pile.cc special.cc\
		eqn.cc
CPPFLAGS+=	-I${DIST}
LDADD+=		$(LIBGROFF)
DPADD+=		$(LIBGROFF)

CLEANFILES+=	eqn.cc eqn.tab.h neqn

lex.o:	eqn.cc

FILES=neqn
FILESMODE=${BINMODE}

realall: neqn

neqn:	neqn.sh
	sed -e 's/@g@/$(g)/g' ${.ALLSRC} > ${.TARGET}

.include <bsd.prog.mk>
