# $NetBSD: Makefile,v 1.3 2007/10/07 04:16:47 xtraeme Exp $

PROG=		envstat
SRCS+=		envstat.c config.c config_yacc.y config_lex.l

LDADD=		-lprop
DPADD=		${LIBPROP}

MAN=		envstat.8 envsys.conf.5
WARNS=		4

YHEADER=	yes

.include <bsd.prog.mk>
