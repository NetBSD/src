# Makefile for groff

PROG=		groff
SRCS=		groff.cc pipeline.c
CFLAGS+=	-I$(.CURDIR)/../include
LDADD+=		$(LIBGROFF) -lm
DPADD+=		$(LIBGROFF) $(LIBMATH)

.include <bsd.prog.mk>
.include "../../Makefile.inc"
.include "../Makefile.cfg"
