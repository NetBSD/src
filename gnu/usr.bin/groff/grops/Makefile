#	$NetBSD: Makefile,v 1.8.2.1 1997/11/08 06:53:38 lukem Exp $

PROG=		grops
SRCS=		ps.cc psrm.cc
CPPFLAGS+=	-I$(.CURDIR)/../include
LDADD+=		$(LIBDRIVER) $(LIBGROFF) -lm
DPADD+=		$(LIBDRIVER) $(LIBGROFF) $(LIBM)

.include "../../Makefile.inc"
.include "../Makefile.cfg"
.include <bsd.prog.mk>
