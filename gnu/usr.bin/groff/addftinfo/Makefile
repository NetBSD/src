#	$NetBSD: Makefile,v 1.6 2001/04/19 14:40:41 wiz Exp $

.include "../Makefile.lib"

DIST=		${IDIST}/src/utils/addftinfo
.PATH:		${DIST}

PROG=		addftinfo
SRCS=		addftinfo.cc guess.cc
LDADD+=		$(LIBGROFF)
DPADD+=		$(LIBGROFF)

.include <bsd.prog.mk>
