#	$NetBSD: Makefile,v 1.9 1997/03/13 22:38:39 christos Exp $
#	from: @(#)Makefile	8.2 (Berkeley) 4/3/94

# define SMALLFTP if editing is to be disabled
#SMALLFTP=yes

PROG=	ftp
SRCS=	cmds.c cmdtab.c complete.c domacro.c fetch.c ftp.c main.c ruserpass.c \
	util.c

.if defined(SMALLFTP)
CFLAGS+=-DSMALLFTP
.else
LDADD+=	-ledit -ltermcap
.endif

.include <bsd.prog.mk>
