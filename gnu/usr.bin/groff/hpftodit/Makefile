#	$NetBSD: Makefile,v 1.2.2.1 1997/11/08 06:53:42 lukem Exp $

PROG=		hpftodit
SRCS=		hpftodit.cc
CPPFLAGS+=	-I$(.CURDIR)/../include
LDADD+=		$(LIBGROFF) -lm
DPADD+=		$(LIBGROFF) $(LIBM)

.include "../../Makefile.inc"
.include "../Makefile.cfg"
.include <bsd.prog.mk>
