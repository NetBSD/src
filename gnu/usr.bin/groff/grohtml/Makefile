#	$NetBSD: Makefile,v 1.3 2002/01/01 01:38:26 thorpej Exp $

.include "../Makefile.lib"

DIST=		${IDIST}/src/devices/grohtml
.PATH:		${DIST}

PROG_CXX=	grohtml
SRCS=		html.cc output.cc
LDADD+=		$(LIBDRIVER) $(LIBGROFF) -lm
DPADD+=		$(LIBDRIVER) $(LIBGROFF) $(LIBM)

.include <bsd.prog.mk>
