# Makefile for tfmtodit

PROG=		tfmtodit
SRCS=		tfmtodit.cc
CFLAGS+=	-I$(.CURDIR)/../include
LDADD+=		$(LIBGROFF) -lm
DPADD+=		$(LIBGROFF) $(LIBMATH)

.include <bsd.prog.mk>
.include "../../Makefile.inc"
.include "../Makefile.cfg"
