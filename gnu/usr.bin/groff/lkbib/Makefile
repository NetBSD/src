#	$NetBSD: Makefile,v 1.5 1997/10/18 05:59:12 lukem Exp $

PROG=		lkbib
SRCS=		lkbib.cc
CFLAGS+=	-I$(.CURDIR)/../include
LDADD+=		$(LIBBIB) $(LIBGROFF) -lm
DPADD+=		$(LIBBIB) $(LIBGROFF) $(LIBM)

.include "../../Makefile.inc"
.include "../Makefile.cfg"
.include <bsd.prog.mk>
