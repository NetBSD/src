#	$NetBSD: Makefile,v 1.9 1997/10/25 10:11:53 lukem Exp $

PROG=		grotty
SRCS=		tty.cc
CPPFLAGS+=	-I$(.CURDIR)/../include
LDADD+=		$(LIBDRIVER) $(LIBGROFF) -lm
DPADD+=		$(LIBDRIVER) $(LIBGROFF) $(LIBM)

.include "../../Makefile.inc"
.include "../Makefile.cfg"
.include <bsd.prog.mk>
