#	$NetBSD: Makefile,v 1.5 1997/10/25 10:11:29 lukem Exp $

PROG=		addftinfo
SRCS=		addftinfo.cc guess.cc
CPPFLAGS+=	-I$(.CURDIR)/../include
LDADD+=		$(LIBGROFF)
DPADD+=		$(LIBGROFF)

.include "../../Makefile.inc"
.include "../Makefile.cfg"
.include <bsd.prog.mk>
