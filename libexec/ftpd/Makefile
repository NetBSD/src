#	from: @(#)Makefile	5.15 (Berkeley) 7/1/90
#	$Id: Makefile,v 1.6 1994/03/30 02:50:15 cgd Exp $

PROG=	ftpd
CFLAGS+=-I${.CURDIR}/../../usr.bin/ftp -DSETPROCTITLE
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
