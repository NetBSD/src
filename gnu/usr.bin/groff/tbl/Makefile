#	$NetBSD: Makefile,v 1.9 2001/04/19 14:41:42 wiz Exp $

.include "../Makefile.lib"

DIST=		${IDIST}/src/preproc/tbl
.PATH:		${DIST}

PROG=		tbl
SRCS=		main.cc table.cc
LDADD+=		$(LIBGROFF) -lm
DPADD+=		$(LIBGROFF) $(LIBM)

.include <bsd.prog.mk>
