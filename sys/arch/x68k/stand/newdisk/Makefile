#	$NetBSD: Makefile,v 1.1 1999/12/24 16:51:25 minoura Exp $

PROG=		newdisk
CPPFLAGS+=	-DMBOOT=\"/usr/mdec/mboot\"
DPADD+=		${LIBUTIL}
LDADD+=		-lutil
LDSTATIC=	-static
MKMAN=		no		# see /usr/src/share/man/man8/man8.x68k

BINDIR=		/usr/mdec

.include <bsd.prog.mk>
