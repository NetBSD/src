#	$NetBSD: Makefile,v 1.12 2002/01/01 01:38:29 thorpej Exp $

.include "../Makefile.lib"

DIST=		${IDIST}/src/preproc/refer
.PATH:		${DIST}

PROG_CXX=	refer
SRCS=		command.cc ref.cc refer.cc token.cc label.cc
CPPFLAGS+=	-I. -I${DIST}
LDADD+=		$(LIBBIB) $(LIBGROFF) -lm
DPADD+=		$(LIBBIB) $(LIBGROFF) $(LIBM)
DPSRCS+=	label_tab.h
CLEANFILES+=	label.cc

label_tab.h: label.cc

.include <bsd.prog.mk>
