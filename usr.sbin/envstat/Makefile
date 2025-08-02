# $NetBSD: Makefile,v 1.10.58.1 2025/08/02 05:58:49 perseant Exp $

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
