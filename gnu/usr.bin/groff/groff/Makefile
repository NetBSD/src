#	$NetBSD: Makefile,v 1.7 1997/10/18 05:58:55 lukem Exp $

PROG=		groff
SRCS=		groff.cc pipeline.c
CFLAGS+=	-I$(.CURDIR)/../include
LDADD+=		$(LIBGROFF) -lm
DPADD+=		$(LIBGROFF) $(LIBM)

.include "../../Makefile.inc"
.include "../Makefile.cfg"
.include <bsd.prog.mk>
