#	$NetBSD: Makefile,v 1.1.1.2 2001/04/19 14:33:14 wiz Exp $

.include "../Makefile.lib"

DIST=		${IDIST}/src/preproc/tbl
.PATH:		${DIST}

PROG=		tbl
SRCS=		main.cc table.cc
LDADD+=		$(LIBGROFF) -lm
DPADD+=		$(LIBGROFF) $(LIBM)

.include <bsd.prog.mk>
