#	$NetBSD: Makefile,v 1.23 1997/10/12 14:04:36 mycroft Exp $
#	@(#)Makefile	8.2 (Berkeley) 4/4/94

# XXX Work-around a compiler bug for now... can't use -O
.if (${MACHINE_ARCH} == "powerpc")
CFLAGS=
.endif

PROG=	ftpd
SRCS=	conf.c ftpd.c ftpcmd.c logwtmp.c popen.c
CPPFLAGS+=-DHASSETPROCTITLE -Dunix
DPADD+=	${LIBCRYPT}
LDADD+=	-lcrypt
MAN=	ftpd.8
CLEANFILES+=ftpcmd.c y.tab.h
.PATH:	${.CURDIR}/../../usr.bin/ftp ${.CURDIR}/../../usr.bin/login

.include <bsd.prog.mk>

.ifdef SKEY
CPPFLAGS+=-DSKEY
DPADD+= ${LIBSKEY}
LDADD+= -lskey
.endif

.ifdef KERBEROS5
SRCS+=	k5login.c
CPPFLAGS+=-DKERBEROS5
DPADD+=	${LIBKRB5} ${LIBCRYPTO}
LDADD+=	-lkrb5 -lcrypto -lcom_err
.elifdef KERBEROS
SRCS+=	klogin.c
CPPFLAGS+=-DKERBEROS
DPADD+=	${LIBKRB} ${LIBDES}
LDADD+=	-lkrb -ldes
.endif
