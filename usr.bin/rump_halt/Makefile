#	$NetBSD: Makefile,v 1.4 2021/07/07 11:51:46 martin Exp $
#

PROG=	rump.halt

LDADD+= -lrumpclient
DPADD+=	${RUMPCLIENT}

.if ${RUMP_SANITIZE:Uno} != "no"
LDADD+=	-fsanitize=${RUMP_SANITIZE}
.endif

.include <bsd.prog.mk>
