# $NetBSD: Makefile,v 1.8 2010/11/05 13:42:37 pooka Exp $

PROG=		envstat
SRCS+=		envstat.c config.c config_yacc.y config_lex.l

LDADD=		-lprop
DPADD=		${LIBPROP}

MAN=		envstat.8 envsys.conf.5

CPPFLAGS+=      -I${.CURDIR} -I.

YHEADER=	yes

.ifdef RUMP_ACTION
LDADD+=		-lrumpclient
CPPFLAGS+=	-DRUMP_SYS_OPEN -DRUMP_SYS_CLOSE
CPPFLAGS+=	-DRUMP_ACTION
DBG=		-g
.endif

.include <bsd.prog.mk>
