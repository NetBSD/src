#	$NetBSD: Makefile,v 1.5 1997/10/18 05:59:33 lukem Exp $

PROG=		tfmtodit
SRCS=		tfmtodit.cc
CFLAGS+=	-I$(.CURDIR)/../include
LDADD+=		$(LIBGROFF) -lm
DPADD+=		$(LIBGROFF) $(LIBM)

.include "../../Makefile.inc"
.include "../Makefile.cfg"
.include <bsd.prog.mk>
