# $NetBSD: Makefile,v 1.4.14.1 2009/05/13 19:20:22 jym Exp $

PROG=		envstat
SRCS+=		envstat.c config.c config_yacc.y config_lex.l

LDADD=		-lprop
DPADD=		${LIBPROP}

MAN=		envstat.8 envsys.conf.5

CPPFLAGS+=      -I${.CURDIR} -I. -DYY_NO_INPUT

YHEADER=	yes

.include <bsd.prog.mk>
