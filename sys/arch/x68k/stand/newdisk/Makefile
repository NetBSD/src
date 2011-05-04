#	$NetBSD: Makefile,v 1.3 2011/05/04 11:44:25 tsutsui Exp $

WARNS?=		4
PROG=		newdisk
CPPFLAGS+=	-DMBOOT=\"/usr/mdec/mboot\"
DPADD+=		${LIBUTIL}
LDADD+=		-lutil
LDSTATIC=	-static
NOMAN=		# defined

BINDIR=		/usr/mdec

.include <bsd.prog.mk>
