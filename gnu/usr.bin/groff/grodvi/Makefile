#	$NetBSD: Makefile,v 1.6 1997/10/25 10:11:37 lukem Exp $

PROG=		grodvi
SRCS=		dvi.cc
CPPFLAGS+=	-I$(.CURDIR)/../include
LDADD+=		$(LIBDRIVER) $(LIBGROFF) -lm
DPADD+=		$(LIBDRIVER) $(LIBGROFF) $(LIBM)

.include "../../Makefile.inc"
.include "../Makefile.cfg"
.include <bsd.prog.mk>
