# $NetBSD: Makefile,v 1.3 2003/03/04 22:31:15 jmmv Exp $
#

PROG=	wsmoused
SRCS=	wsmoused.c config.c config_yacc.y config_lex.l events.c selection.c

MAN=	wsmoused.conf.5 wsmoused.8

CPPFLAGS+=	-I.
LDADD+=		-lutil

YHEADER=	yes

.include <bsd.prog.mk>
