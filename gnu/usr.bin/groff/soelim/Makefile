#	$NetBSD: Makefile,v 1.7 2002/01/01 00:27:37 thorpej Exp $

.include "../Makefile.lib"

DIST=		${IDIST}/src/preproc/soelim
.PATH:		${DIST}

PROG=		soelim
PROG_CXX=
SRCS=		soelim.cc
LDADD+=		$(LIBGROFF)
DPADD+=		$(LIBGROFF)

.include <bsd.prog.mk>
