#	$Id: Makefile,v 1.6 1995/10/26 00:58:54 jtc Exp $

PROG=		groff
SRCS=		groff.cc pipeline.c
CFLAGS+=	-I$(.CURDIR)/../include
LDADD+=		$(LIBGROFF) -lm
DPADD+=		$(LIBGROFF) $(LIBM)

.include "../../Makefile.inc"
.include "../Makefile.cfg"
.include <bsd.prog.mk>
