#	$NetBSD: Makefile,v 1.40 2023/06/03 09:09:16 lukem Exp $
#	from: @(#)Makefile	8.2 (Berkeley) 4/3/94

.include <bsd.own.mk>

USE_FORT?= yes	# network client

PROG=	ftp
SRCS=	cmds.c cmdtab.c complete.c domacro.c fetch.c ftp.c main.c \
	progressbar.c ruserpass.c util.c
SRCS+=	ssl.c

# Uncomment the following to provide defaults for gate-ftp operation
#
#CPPFLAGS+=-DGATE_SERVER=\"ftp-gw.host\" # -DGATE_PORT=21

.if defined(SMALLPROG)
CPPFLAGS+=-DNO_EDITCOMPLETE -DNO_ABOUT -DNO_AUTH -DNO_HELP -DNO_STATUS -DNO_DEBUG -DNO_USAGE
.else
LDADD+=	-ledit -lterminfo
DPADD+=	${LIBEDIT} ${LIBTERMINFO}
CPPFLAGS+= -DWITH_SSL
LDADD+= -lssl -lcrypto
DPADD+= ${LIBSSL} ${LIBCRYPTO}
.endif

.if (!defined(SMALLPROG) || defined(SMALLPROG_INET6)) && (${USE_INET6} != "no")
CPPFLAGS+= -DINET6
.endif

cmds.o fetch.o: version.h
main.o:	ftp_var.h

CWARNFLAGS.gcc+=	${CC_WNO_FORMAT_OVERFLOW}

.include <bsd.prog.mk>
