#	$NetBSD: Makefile,v 1.7 2002/01/01 00:27:33 thorpej Exp $

.include "../Makefile.lib"

DIST=		${IDIST}/src/utils/addftinfo
.PATH:		${DIST}

PROG=		addftinfo
PROG_CXX=
SRCS=		addftinfo.cc guess.cc
LDADD+=		$(LIBGROFF)
DPADD+=		$(LIBGROFF)

.include <bsd.prog.mk>
