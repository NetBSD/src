#	$Id: Makefile,v 1.4 1995/10/26 00:59:15 jtc Exp $

PROG=		tfmtodit
SRCS=		tfmtodit.cc
CFLAGS+=	-I$(.CURDIR)/../include
LDADD+=		$(LIBGROFF) -lm
DPADD+=		$(LIBGROFF) $(LIBM)

.include "../../Makefile.inc"
.include "../Makefile.cfg"
.include <bsd.prog.mk>
