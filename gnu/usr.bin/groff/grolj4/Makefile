#	$NetBSD: Makefile,v 1.2.2.1 1997/11/08 06:53:36 lukem Exp $

PROG=		grolj4
SRCS=		lj4.cc
CPPFLAGS+=	-I$(.CURDIR)/../include
LDADD+=		$(LIBDRIVER) $(LIBGROFF) -lm
DPADD+=		$(LIBDRIVER) $(LIBGROFF) $(LIBM)

.include "../../Makefile.inc"
.include "../Makefile.cfg"
.include <bsd.prog.mk>
