#	$NetBSD: Makefile,v 1.8.2.1 1997/11/08 06:53:40 lukem Exp $

PROG=		grotty
SRCS=		tty.cc
CPPFLAGS+=	-I$(.CURDIR)/../include
LDADD+=		$(LIBDRIVER) $(LIBGROFF) -lm
DPADD+=		$(LIBDRIVER) $(LIBGROFF) $(LIBM)

.include "../../Makefile.inc"
.include "../Makefile.cfg"
.include <bsd.prog.mk>
