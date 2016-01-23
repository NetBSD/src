#	$NetBSD: Makefile,v 1.2 2016/01/23 21:22:50 christos Exp $
#

PROG=	rump.halt

LDADD+= -lrumpclient
DPADD+=	${RUMPCLIENT}
CPPFLAGS+=	-D_KERNTYPES

.include <bsd.prog.mk>
