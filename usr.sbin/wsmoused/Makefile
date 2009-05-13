# $NetBSD: Makefile,v 1.6.40.1 2009/05/13 19:20:44 jym Exp $
#

WARNS?= 2	# XXX -Wcast-qual (etc)

PROG=	wsmoused
SRCS=	wsmoused.c config.c config_yacc.y config_lex.l

MAN=	wsmoused.conf.5 wsmoused.8

WSMOUSED_ACTION_MODE?=		yes
WSMOUSED_SELECTION_MODE?=	yes

.if !empty(WSMOUSED_ACTION_MODE:M[Yy][Ee][Ss])
CPPFLAGS+=	-DWSMOUSED_ACTION_MODE
SRCS+=		action.c
.endif

.if !empty(WSMOUSED_SELECTION_MODE:M[Yy][Ee][Ss])
CPPFLAGS+=	-DWSMOUSED_SELECTION_MODE
SRCS+=		selection.c
.endif

CPPFLAGS+=	-I${.CURDIR} -I. -DYY_NO_INPUT
LDADD+=		-lutil

YHEADER=	yes

.include <bsd.prog.mk>
