#	$NetBSD: Makefile,v 1.5 1997/10/18 05:58:53 lukem Exp $

PROG=		grodvi
SRCS=		dvi.cc
CFLAGS+=	-I$(.CURDIR)/../include
LDADD+=		$(LIBDRIVER) $(LIBGROFF) -lm
DPADD+=		$(LIBDRIVER) $(LIBGROFF) $(LIBM)

.include "../../Makefile.inc"
.include "../Makefile.cfg"
.include <bsd.prog.mk>
