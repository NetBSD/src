#	$NetBSD: Makefile,v 1.3 1997/10/25 10:11:46 lukem Exp $

PROG=		grolj4
SRCS=		lj4.cc
CPPFLAGS+=	-I$(.CURDIR)/../include
LDADD+=		$(LIBDRIVER) $(LIBGROFF) -lm
DPADD+=		$(LIBDRIVER) $(LIBGROFF) $(LIBM)

.include "../../Makefile.inc"
.include "../Makefile.cfg"
.include <bsd.prog.mk>
