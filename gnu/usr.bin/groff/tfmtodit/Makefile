#	$NetBSD: Makefile,v 1.6 1997/10/25 10:12:30 lukem Exp $

PROG=		tfmtodit
SRCS=		tfmtodit.cc
CPPFLAGS+=	-I$(.CURDIR)/../include
LDADD+=		$(LIBGROFF) -lm
DPADD+=		$(LIBGROFF) $(LIBM)

.include "../../Makefile.inc"
.include "../Makefile.cfg"
.include <bsd.prog.mk>
