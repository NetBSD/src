#	$NetBSD: Makefile,v 1.5.2.1 1997/11/08 06:54:02 lukem Exp $

PROG=		tfmtodit
SRCS=		tfmtodit.cc
CPPFLAGS+=	-I$(.CURDIR)/../include
LDADD+=		$(LIBGROFF) -lm
DPADD+=		$(LIBGROFF) $(LIBM)

.include "../../Makefile.inc"
.include "../Makefile.cfg"
.include <bsd.prog.mk>
