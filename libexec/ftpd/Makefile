#	from : @(#)Makefile	8.2 (Berkeley) 4/4/94
#	$Id: Makefile,v 1.9.2.1 1994/08/19 11:27:18 mycroft Exp $

PROG=	ftpd
CFLAGS+=-DHASSETPROCTITLE -DSKEY
SRCS=	ftpd.c ftpcmd.c logwtmp.c popen.c
MAN8=	ftpd.0
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
