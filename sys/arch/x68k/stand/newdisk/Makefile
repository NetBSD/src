#	$NetBSD: Makefile,v 1.2 2001/12/12 01:49:56 tv Exp $

PROG=		newdisk
CPPFLAGS+=	-DMBOOT=\"/usr/mdec/mboot\"
DPADD+=		${LIBUTIL}
LDADD+=		-lutil
LDSTATIC=	-static
NOMAN=		# defined

BINDIR=		/usr/mdec

.include <bsd.prog.mk>
