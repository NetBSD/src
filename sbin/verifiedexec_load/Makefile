#	$NetBSD: Makefile,v 1.5 2002/11/18 13:45:02 lukem Exp $

PROG=	verifiedexec_load
MAN=	verifiedexec_load.8
SRCS=	verifiedexec_parse.y verifiedexec_conf.l verifiedexec_load.c

YHEADER=	1
CPPFLAGS+=	-I.
DPADD+=		${LIBL}
LDADD+=		-ll

LDSTATIC=	-static
# not sure if it is really needed
CFLAGS+=	-g

.include <bsd.prog.mk>
