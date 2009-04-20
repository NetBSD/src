# $NetBSD: Makefile,v 1.5 2009/04/20 16:05:30 drochner Exp $

PROG=		envstat
SRCS+=		envstat.c config.c config_yacc.y config_lex.l

LDADD=		-lprop
DPADD=		${LIBPROP}

MAN=		envstat.8 envsys.conf.5
WARNS=		4

CPPFLAGS+=      -I${.CURDIR} -I. -DYY_NO_INPUT

YHEADER=	yes

.include <bsd.prog.mk>
