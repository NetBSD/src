#	$NetBSD: Makefile,v 1.2.150.1 2011/06/06 09:07:04 jruoho Exp $

WARNS?=		4
PROG=		newdisk
CPPFLAGS+=	-DMBOOT=\"/usr/mdec/mboot\"
DPADD+=		${LIBUTIL}
LDADD+=		-lutil
LDSTATIC=	-static
NOMAN=		# defined

BINDIR=		/usr/mdec

.include <bsd.prog.mk>
