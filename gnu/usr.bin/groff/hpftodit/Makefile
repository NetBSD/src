#	$NetBSD: Makefile,v 1.2 1997/10/18 05:59:02 lukem Exp $

PROG=		hpftodit
SRCS=		hpftodit.cc
CFLAGS+=	-I$(.CURDIR)/../include
LDADD+=		$(LIBGROFF) -lm
DPADD+=		$(LIBGROFF) $(LIBM)

.include "../../Makefile.inc"
.include "../Makefile.cfg"
.include <bsd.prog.mk>
