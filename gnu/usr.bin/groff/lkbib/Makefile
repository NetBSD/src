#	$NetBSD: Makefile,v 1.6 1997/10/25 10:12:12 lukem Exp $

PROG=		lkbib
SRCS=		lkbib.cc
CPPFLAGS+=	-I$(.CURDIR)/../include
LDADD+=		$(LIBBIB) $(LIBGROFF) -lm
DPADD+=		$(LIBBIB) $(LIBGROFF) $(LIBM)

.include "../../Makefile.inc"
.include "../Makefile.cfg"
.include <bsd.prog.mk>
