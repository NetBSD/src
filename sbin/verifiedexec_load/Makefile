#	$NetBSD: Makefile,v 1.4 2002/11/17 13:57:38 itojun Exp $

PROG=	verifiedexec_load
MAN=	verifiedexec_load.8
SRCS=	verifiedexec_parse.y verifiedexec_conf.l verifiedexec_load.c

CPPFLAGS+=	-I.
LDADD=		-ll
YFLAGS+=	-d
LDSTATIC=	-static
CLEANFILES+=	verifiedexec_parse.h
# not sure if it is really needed
CFLAGS+=	-g

.include <bsd.prog.mk>
