#	$NetBSD: Makefile,v 1.5.2.1 1997/11/08 06:53:52 lukem Exp $

PROG=		lkbib
SRCS=		lkbib.cc
CPPFLAGS+=	-I$(.CURDIR)/../include
LDADD+=		$(LIBBIB) $(LIBGROFF) -lm
DPADD+=		$(LIBBIB) $(LIBGROFF) $(LIBM)

.include "../../Makefile.inc"
.include "../Makefile.cfg"
.include <bsd.prog.mk>
