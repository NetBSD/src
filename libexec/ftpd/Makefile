#	from: @(#)Makefile	5.15 (Berkeley) 7/1/90
#	$Id: Makefile,v 1.8 1994/05/24 06:53:33 deraadt Exp $

PROG=	ftpd
CFLAGS+=-I${.CURDIR}/../../usr.bin/ftp -DHASSETPROCTITLE -DSKEY
SRCS=	ftpd.c ftpcmd.c glob.c logwtmp.c popen.c vers.c klogin.c
MAN8=	ftpd.0
CLEANFILES+=ftpcmd.c y.tab.h
.PATH:	${.CURDIR}/../../usr.bin/ftp ${.CURDIR}/../../usr.bin/login

LDADD+= -lcrypt -lskey
DPADD+= ${LIBCRYPT} ${LIBSKEY}

.if defined(KERBEROS)
CFLAGS+= -DKERBEROS
LDADD+=	-lkrb -ldes
DPADD+= ${LIBKRB} ${LIBDES}
.endif

.include <bsd.prog.mk>
