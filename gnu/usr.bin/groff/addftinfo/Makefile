#	$NetBSD: Makefile,v 1.4 1997/10/18 05:58:25 lukem Exp $

PROG=		addftinfo
SRCS=		addftinfo.cc guess.cc
CFLAGS+=	-I$(.CURDIR)/../include
LDADD+=		$(LIBGROFF)
DPADD+=		$(LIBGROFF)

.include "../../Makefile.inc"
.include "../Makefile.cfg"
.include <bsd.prog.mk>
