#	$NetBSD: Makefile,v 1.3 1997/10/25 10:11:56 lukem Exp $

PROG=		hpftodit
SRCS=		hpftodit.cc
CPPFLAGS+=	-I$(.CURDIR)/../include
LDADD+=		$(LIBGROFF) -lm
DPADD+=		$(LIBGROFF) $(LIBM)

.include "../../Makefile.inc"
.include "../Makefile.cfg"
.include <bsd.prog.mk>
