#	$NetBSD: Makefile,v 1.6 2001/04/19 14:41:41 wiz Exp $

.include "../Makefile.lib"

DIST=		${IDIST}/src/preproc/soelim
.PATH:		${DIST}

PROG=		soelim
SRCS=		soelim.cc
LDADD+=		$(LIBGROFF)
DPADD+=		$(LIBGROFF)

.include <bsd.prog.mk>
