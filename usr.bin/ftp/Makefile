#	$NetBSD: Makefile,v 1.20.2.1 1999/12/27 18:36:56 wrstuden Exp $
#	from: @(#)Makefile	8.2 (Berkeley) 4/3/94

PROG=	ftp
SRCS=	cmds.c cmdtab.c complete.c domacro.c fetch.c ftp.c main.c ruserpass.c \
	util.c

# Uncomment the following to provide defaults for gate-ftp operation
#
#CPPFLAGS+=-DGATE_SERVER=\"ftp-gw.host\" # -DGATE_PORT=21

LDADD+=	-ledit -ltermcap -lutil
DPADD+=	${LIBEDIT} ${LIBTERMCAP} ${LIBUTIL}

CPPFLAGS+= -DINET6

cmds.o fetch.o: version.h
main.o:	ftp_var.h

.include <bsd.prog.mk>
