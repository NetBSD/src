#	from: @(#)Makefile	5.15 (Berkeley) 7/1/90
#	$Id: Makefile,v 1.5 1993/10/07 02:16:39 cgd Exp $

PROG=	ftpd
CFLAGS+=-I${.CURDIR}/../../usr.bin/ftp -DSETPROCTITLE
SRCS=	ftpd.c ftpcmd.c glob.c logwtmp.c popen.c vers.c
MAN8=	ftpd.0
CLEANFILES+=ftpcmd.c y.tab.h
.PATH:	${.CURDIR}/../../usr.bin/ftp

LDADD+= -lcrypt
DPADD+= ${LIBCRYPT}

.include <bsd.prog.mk>
