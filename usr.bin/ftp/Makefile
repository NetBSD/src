#	$NetBSD: Makefile,v 1.12 1997/07/20 09:45:35 lukem Exp $
#	from: @(#)Makefile	8.2 (Berkeley) 4/3/94

PROG=	ftp
SRCS=	cmds.c cmdtab.c complete.c domacro.c fetch.c ftp.c main.c ruserpass.c \
	util.c
WARNS=	1

LDADD+=	-ledit -ltermcap
DPADD+=	${LIBEDIT} ${LIBTERMCAP}

.include <bsd.prog.mk>
