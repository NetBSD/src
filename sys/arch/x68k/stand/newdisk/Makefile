#	$NetBSD: Makefile,v 1.2.144.1 2011/05/31 03:04:22 rmind Exp $

WARNS?=		4
PROG=		newdisk
CPPFLAGS+=	-DMBOOT=\"/usr/mdec/mboot\"
DPADD+=		${LIBUTIL}
LDADD+=		-lutil
LDSTATIC=	-static
NOMAN=		# defined

BINDIR=		/usr/mdec

.include <bsd.prog.mk>
