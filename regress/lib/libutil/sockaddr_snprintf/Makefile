#	$NetBSD: Makefile,v 1.2 2005/01/10 02:58:58 lukem Exp $

NOMAN=		# defined

.include <bsd.own.mk>

PROG=		sockaddr_snprintf

.if (${USE_INET6} != "no")
CPPFLAGS+=	-DINET6
.endif

regress: ${PROG}
	./${PROG}

DPADD+= ${LIBUTIL}
LDADD+= -lutil

.include <bsd.prog.mk>
