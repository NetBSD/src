#	from: @(#)Makefile	5.15 (Berkeley) 7/1/90
#	$Id: Makefile,v 1.7 1994/04/14 03:15:37 cgd Exp $

PROG=	ftpd
CFLAGS+=-I${.CURDIR}/../../usr.bin/ftp -DHASSETPROCTITLE
SRCS=	ftpd.c ftpcmd.c glob.c logwtmp.c popen.c vers.c klogin.c
MAN8=	ftpd.0
CLEANFILES+=ftpcmd.c y.tab.h
.PATH:	${.CURDIR}/../../usr.bin/ftp ${.CURDIR}/../../usr.bin/login

LDADD+= -lcrypt
DPADD+= ${LIBCRYPT}

.if defined(KERBEROS)
CFLAGS+= -DKERBEROS
LDADD+=	-lkrb -ldes
DPADD+= ${LIBKRB} ${LIBDES}
.endif

.include <bsd.prog.mk>
