#	from : @(#)Makefile	8.2 (Berkeley) 4/4/94
#	$Id: Makefile,v 1.11 1994/12/22 10:27:01 cgd Exp $

PROG=	ftpd
CFLAGS+=-DHASSETPROCTITLE -DSKEY
SRCS=	ftpd.c ftpcmd.c logwtmp.c popen.c
MAN=	ftpd.8
CLEANFILES+=ftpcmd.c y.tab.h
.PATH:	${.CURDIR}/../../usr.bin/ftp ${.CURDIR}/../../usr.bin/login

LDADD+= -lcrypt -lskey
DPADD+= ${LIBCRYPT} ${LIBSKEY}

.if defined(KERBEROS)
SRCS+= klogin.c
CFLAGS+= -DKERBEROS
LDADD+= -lkrb -ldes
DPADD+= ${LIBKRB} ${LIBDES}
.endif

.include <bsd.prog.mk>
