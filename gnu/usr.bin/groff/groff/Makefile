#	$NetBSD: Makefile,v 1.7.2.1 1997/11/08 06:53:34 lukem Exp $

PROG=		groff
SRCS=		groff.cc pipeline.c
CPPFLAGS+=	-I$(.CURDIR)/../include
LDADD+=		$(LIBGROFF) -lm
DPADD+=		$(LIBGROFF) $(LIBM)

.include "../../Makefile.inc"
.include "../Makefile.cfg"
.include <bsd.prog.mk>
