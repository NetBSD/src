#	$NetBSD: Makefile,v 1.8 2002/01/01 01:38:29 thorpej Exp $

.include "../Makefile.lib"

DIST=		${IDIST}/src/preproc/soelim
.PATH:		${DIST}

PROG_CXX=	soelim
SRCS=		soelim.cc
LDADD+=		$(LIBGROFF)
DPADD+=		$(LIBGROFF)

.include <bsd.prog.mk>
