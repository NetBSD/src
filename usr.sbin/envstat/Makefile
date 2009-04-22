# $NetBSD: Makefile,v 1.6 2009/04/22 15:23:03 lukem Exp $

PROG=		envstat
SRCS+=		envstat.c config.c config_yacc.y config_lex.l

LDADD=		-lprop
DPADD=		${LIBPROP}

MAN=		envstat.8 envsys.conf.5

CPPFLAGS+=      -I${.CURDIR} -I. -DYY_NO_INPUT

YHEADER=	yes

.include <bsd.prog.mk>
