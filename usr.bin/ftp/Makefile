#	$NetBSD: Makefile,v 1.15 1997/10/18 15:31:20 lukem Exp $
#	from: @(#)Makefile	8.2 (Berkeley) 4/3/94

PROG=	ftp
SRCS=	cmds.c cmdtab.c complete.c domacro.c fetch.c ftp.c main.c ruserpass.c \
	util.c

# Uncomment the following to provide defaults for gate-ftp operation
#
#CPPFLAGS+=-DGATE_SERVER=\"ftp-gw.host\" # -DGATE_PORT=21

LDADD+=	-ledit -ltermcap
DPADD+=	${LIBEDIT} ${LIBTERMCAP}

.include <bsd.prog.mk>
