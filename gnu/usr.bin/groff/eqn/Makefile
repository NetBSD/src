#	$NetBSD: Makefile,v 1.18 2002/01/01 01:38:26 thorpej Exp $

.include "../Makefile.lib"

DIST=		${IDIST}/src/preproc/eqn
.PATH:		${DIST}

PROG_CXX=	eqn
SRCS=		main.cc lex.cc  box.cc limit.cc list.cc over.cc text.cc\
		script.cc mark.cc other.cc delim.cc sqrt.cc pile.cc special.cc\
		eqn.cc
CPPFLAGS+=	-I. -I${DIST}
LDADD+=		$(LIBGROFF)
DPADD+=		$(LIBGROFF)
DPSRCS+=	eqn_tab.h
CLEANFILES+=	eqn.cc neqn

eqn_tab.h lex.o: eqn.cc

FILES=neqn
FILESMODE=${BINMODE}

realall: neqn

neqn:	neqn.sh
	sed -e 's/@g@/$(g)/g' ${.ALLSRC} > ${.TARGET}

.include <bsd.prog.mk>
