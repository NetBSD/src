#	$NetBSD: Makefile,v 1.20 1999/07/02 08:07:40 itojun Exp $
#	from: @(#)Makefile	8.2 (Berkeley) 4/3/94

PROG=	ftp
SRCS=	cmds.c cmdtab.c complete.c domacro.c fetch.c ftp.c main.c ruserpass.c \
	util.c

# Uncomment the following to provide defaults for gate-ftp operation
#
#CPPFLAGS+=-DGATE_SERVER=\"ftp-gw.host\" # -DGATE_PORT=21

CPPFLAGS+= -DHAVE_TIMEGM	# system has timegm(3), a UTC version of mktime
LDADD+=	-ledit -ltermcap -lutil
DPADD+=	${LIBEDIT} ${LIBTERMCAP} ${LIBUTIL}

CPPFLAGS+= -DINET6

.include <bsd.prog.mk>
