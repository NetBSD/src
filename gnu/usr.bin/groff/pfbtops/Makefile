#	$NetBSD: Makefile,v 1.9 2002/01/22 23:57:19 thorpej Exp $

.include "../Makefile.lib"

DIST=		${IDIST}/src/utils/pfbtops
.PATH:		${DIST}

# XXX Because it uses libgroff
PROG_CXX=	pfbtops
SRCS=		pfbtops.c
LDADD+=		$(LIBGROFF)
DPADD+=		$(LIBGROFF)

.include <bsd.prog.mk>
