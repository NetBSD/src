#	$NetBSD: Makefile,v 1.8 2002/01/01 01:38:25 thorpej Exp $

.include "../Makefile.lib"

DIST=		${IDIST}/src/utils/addftinfo
.PATH:		${DIST}

PROG_CXX=	addftinfo
SRCS=		addftinfo.cc guess.cc
LDADD+=		$(LIBGROFF)
DPADD+=		$(LIBGROFF)

.include <bsd.prog.mk>
