# $NetBSD: Makefile,v 1.5 2003/08/06 18:07:53 jmmv Exp $
#

PROG=	wsmoused
SRCS=	wsmoused.c config.c config_yacc.y config_lex.l

MAN=	wsmoused.conf.5 wsmoused.8

WSMOUSED_SELECTION_MODE?=	yes

.if !empty(WSMOUSED_SELECTION_MODE:M[Yy][Ee][Ss])
CPPFLAGS+=	-DWSMOUSED_SELECTION_MODE
SRCS+=		selection.c
.endif

CPPFLAGS+=	-I${.CURDIR} -I.
LDADD+=		-lutil

YHEADER=	yes

.include <bsd.prog.mk>
