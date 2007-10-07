# $NetBSD: Makefile,v 1.4 2007/10/07 12:19:37 xtraeme Exp $

PROG=		envstat
SRCS+=		envstat.c config.c config_yacc.y config_lex.l

LDADD=		-lprop
DPADD=		${LIBPROP}

MAN=		envstat.8 envsys.conf.5
WARNS=		4

CPPFLAGS+=      -I${.CURDIR} -I.

YHEADER=	yes

.include <bsd.prog.mk>
