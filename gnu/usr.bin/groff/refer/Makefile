#	$NetBSD: Makefile,v 1.9 2001/11/12 17:09:46 tv Exp $

.include "../Makefile.lib"

DIST=		${IDIST}/src/preproc/refer
.PATH:		${DIST}

PROG=		refer
SRCS=		command.cc ref.cc refer.cc token.cc label.cc
CPPFLAGS+=	-I${DIST}
LDADD+=		$(LIBBIB) $(LIBGROFF) -lm
DPADD+=		$(LIBBIB) $(LIBGROFF) $(LIBM)

CLEANFILES+=	label.cc label_tab.h

.include <bsd.prog.mk>
