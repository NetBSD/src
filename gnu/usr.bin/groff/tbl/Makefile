#	$Id: Makefile,v 1.6 1995/10/26 00:59:14 jtc Exp $

PROG=		tbl
SRCS=		main.cc table.cc
CFLAGS+=	-I$(.CURDIR)/../include
LDADD+=		$(LIBGROFF) -lm
DPADD+=		$(LIBGROFF) $(LIBM)

.include "../../Makefile.inc"
.include "../Makefile.cfg"
.include <bsd.prog.mk>
