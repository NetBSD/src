#	$Id: Makefile,v 1.4 1995/10/26 00:59:04 jtc Exp $

PROG=		lkbib
SRCS=		lkbib.cc
CFLAGS+=	-I$(.CURDIR)/../include
LDADD+=		$(LIBBIB) $(LIBGROFF) -lm
DPADD+=		$(LIBBIB) $(LIBGROFF) $(LIBM)

.include "../../Makefile.inc"
.include "../Makefile.cfg"
.include <bsd.prog.mk>
