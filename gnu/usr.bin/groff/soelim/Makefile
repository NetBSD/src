#	$NetBSD: Makefile,v 1.4 1997/10/18 05:59:30 lukem Exp $

PROG=		soelim
SRCS=		soelim.cc
CFLAGS+=	-I$(.CURDIR)/../include
LDADD+=		$(LIBGROFF)
DPADD+=		$(LIBGROFF)

.include "../../Makefile.inc"
.include "../Makefile.cfg"
.include <bsd.prog.mk>
