#	$NetBSD: Makefile,v 1.8 1997/10/25 10:11:41 lukem Exp $

PROG=		groff
SRCS=		groff.cc pipeline.c
CPPFLAGS+=	-I$(.CURDIR)/../include
LDADD+=		$(LIBGROFF) -lm
DPADD+=		$(LIBGROFF) $(LIBM)

.include "../../Makefile.inc"
.include "../Makefile.cfg"
.include <bsd.prog.mk>
