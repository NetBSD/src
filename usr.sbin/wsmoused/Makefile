# $NetBSD: Makefile,v 1.4 2003/03/07 01:31:30 fvdl Exp $
#

PROG=	wsmoused
SRCS=	wsmoused.c config.c config_yacc.y config_lex.l events.c selection.c

MAN=	wsmoused.conf.5 wsmoused.8

CPPFLAGS+=	-I${.CURDIR} -I.
LDADD+=		-lutil

YHEADER=	yes

.include <bsd.prog.mk>
