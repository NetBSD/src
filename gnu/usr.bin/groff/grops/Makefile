#	$Id: Makefile,v 1.7 1995/10/26 00:58:57 jtc Exp $

PROG=		grops
SRCS=		ps.cc psrm.cc
CFLAGS+=	-I$(.CURDIR)/../include
LDADD+=		$(LIBDRIVER) $(LIBGROFF) -lm
DPADD+=		$(LIBDRIVER) $(LIBGROFF) $(LIBM)

.include "../../Makefile.inc"
.include "../Makefile.cfg"
.include <bsd.prog.mk>
