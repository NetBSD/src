#	$NetBSD: Makefile,v 1.19 1997/07/01 20:49:59 christos Exp $
#	@(#)Makefile	8.2 (Berkeley) 4/4/94

# XXX Work-around a compiler bug for now... can't use -O
.if (${MACHINE_ARCH} == "powerpc")
CFLAGS=
.endif

WARNS=	1
PROG=	ftpd
CFLAGS+=-DHASSETPROCTITLE -DSKEY -Dunix
SRCS=	conf.c ftpd.c ftpcmd.c logwtmp.c popen.c
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
