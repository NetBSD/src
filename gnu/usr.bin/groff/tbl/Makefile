#	$NetBSD: Makefile,v 1.11 2002/01/01 01:38:29 thorpej Exp $

.include "../Makefile.lib"

DIST=		${IDIST}/src/preproc/tbl
.PATH:		${DIST}

PROG_CXX=	tbl
SRCS=		main.cc table.cc
LDADD+=		$(LIBGROFF) -lm
DPADD+=		$(LIBGROFF) $(LIBM)

.include <bsd.prog.mk>
