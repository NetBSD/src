# $NetBSD: Makefile,v 1.2 2003/03/04 14:33:55 jmmv Exp $
#

PROG=	wsmoused
SRCS=	wsmoused.c config.c config_yacc.y config_lex.l events.c selection.c

MAN=	wsmoused.conf.5 wsmoused.8

CPPFLAGS+=	-I.

YHEADER=	yes

.include <bsd.prog.mk>
