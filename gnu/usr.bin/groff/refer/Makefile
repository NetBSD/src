#	$NetBSD: Makefile,v 1.8 2001/04/19 14:41:38 wiz Exp $

.include "../Makefile.lib"

DIST=		${IDIST}/src/preproc/refer
.PATH:		${DIST}

PROG=		refer
SRCS=		command.cc ref.cc refer.cc token.cc label.cc
CPPFLAGS+=	-I${DIST}
LDADD+=		$(LIBBIB) $(LIBGROFF) -lm
DPADD+=		$(LIBBIB) $(LIBGROFF) $(LIBM)

CLEANFILES+=	label.cc label.tab.h

.include <bsd.prog.mk>
