#	$NetBSD: Makefile,v 1.1.6.2 2000/11/20 20:30:18 bouyer Exp $

PROG=		newdisk
CPPFLAGS+=	-DMBOOT=\"/usr/mdec/mboot\"
DPADD+=		${LIBUTIL}
LDADD+=		-lutil
LDSTATIC=	-static
MKMAN=		no		# see /usr/src/share/man/man8/man8.x68k

BINDIR=		/usr/mdec

.include <bsd.prog.mk>
