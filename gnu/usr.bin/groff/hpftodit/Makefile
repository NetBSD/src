#	$Id: Makefile,v 1.1 1996/08/13 07:05:03 explorer Exp $

PROG=		hpftodit
SRCS=		hpftodit.cc
CFLAGS+=	-I$(.CURDIR)/../include
LDADD+=		$(LIBGROFF) -lm
DPADD+=		$(LIBGROFF) $(LIBM)

.include "../../Makefile.inc"
.include "../Makefile.cfg"
.include <bsd.prog.mk>
