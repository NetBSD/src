#	$Id: Makefile,v 1.7 1995/10/26 00:58:59 jtc Exp $

PROG=		grotty
SRCS=		tty.cc
CFLAGS+=	-I$(.CURDIR)/../include
LDADD+=		$(LIBDRIVER) $(LIBGROFF) -lm
DPADD+=		$(LIBDRIVER) $(LIBGROFF) $(LIBM)

.include "../../Makefile.inc"
.include "../Makefile.cfg"
.include <bsd.prog.mk>
