#	$NetBSD: Makefile,v 1.10 2002/01/01 00:27:37 thorpej Exp $

.include "../Makefile.lib"

DIST=		${IDIST}/src/preproc/tbl
.PATH:		${DIST}

PROG=		tbl
PROG_CXX=
SRCS=		main.cc table.cc
LDADD+=		$(LIBGROFF) -lm
DPADD+=		$(LIBGROFF) $(LIBM)

.include <bsd.prog.mk>
