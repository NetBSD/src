#	$Id: Makefile,v 1.1 1996/08/13 07:04:46 explorer Exp $

PROG=		grolj4
SRCS=		lj4.cc
CFLAGS+=	-I$(.CURDIR)/../include
LDADD+=		$(LIBDRIVER) $(LIBGROFF) -lm
DPADD+=		$(LIBDRIVER) $(LIBGROFF) $(LIBM)

.include "../../Makefile.inc"
.include "../Makefile.cfg"
.include <bsd.prog.mk>
