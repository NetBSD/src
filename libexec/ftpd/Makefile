#	$NetBSD: Makefile,v 1.16 1997/04/27 03:21:38 lukem Exp $
#	@(#)Makefile	8.2 (Berkeley) 4/4/94

# XXX Work-around a compiler bug for now... can't use -O
.if (${MACHINE_ARCH} == "powerpc")
CFLAGS=
.endif

# uncomment this to disallow group directory owners access to uploaded files
#CFLAGS+= -DGUEST_CMASK=0777
# uncomment this to enable umask, rm and other commands for anonymous users
#CFLAGS+= -DINSECURE_GUEST

PROG=	ftpd
CFLAGS+=-DHASSETPROCTITLE -DSKEY -Dunix
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
