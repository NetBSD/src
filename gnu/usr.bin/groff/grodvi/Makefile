#	$NetBSD: Makefile,v 1.5.2.1 1997/11/08 06:53:32 lukem Exp $

PROG=		grodvi
SRCS=		dvi.cc
CPPFLAGS+=	-I$(.CURDIR)/../include
LDADD+=		$(LIBDRIVER) $(LIBGROFF) -lm
DPADD+=		$(LIBDRIVER) $(LIBGROFF) $(LIBM)

.include "../../Makefile.inc"
.include "../Makefile.cfg"
.include <bsd.prog.mk>
