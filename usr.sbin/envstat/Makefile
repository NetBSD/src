# $NetBSD: Makefile,v 1.11 2025/04/01 11:39:19 brad Exp $

RUMPPRG=	envstat
SRCS+=		envstat.c config.c config_yacc.y config_lex.l

.PATH: ${.CURDIR}/../../common/lib/libprop
CPPFLAGS+=	-DRUMP_ACTION
RUMPSRCS+=	prop_kern.c

LDADD=		-lprop -lmj
DPADD=		${LIBPROP} ${LIBMJ}

MAN=		envstat.8 envsys.conf.5

CPPFLAGS+=      -I${.CURDIR} -I.

YHEADER=	yes

.include <bsd.prog.mk>
