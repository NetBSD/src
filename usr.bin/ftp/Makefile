#	$NetBSD: Makefile,v 1.15.2.1 1998/11/10 18:48:30 cgd Exp $
#	from: @(#)Makefile	8.2 (Berkeley) 4/3/94

PROG=	ftp
SRCS=	cmds.c cmdtab.c complete.c domacro.c fetch.c ftp.c main.c ruserpass.c \
	util.c fparseln.c

# Uncomment the following to provide defaults for gate-ftp operation
#
#CPPFLAGS+=-DGATE_SERVER=\"ftp-gw.host\" # -DGATE_PORT=21
CPPFLAGS+= -I${.CURDIR}

LDADD+=	-ledit -ltermcap -lutil
DPADD+=	${LIBEDIT} ${LIBTERMCAP} ${LIBUTIL}

.include <bsd.prog.mk>
