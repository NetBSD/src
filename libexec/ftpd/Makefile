#	$NetBSD: Makefile,v 1.26 1998/04/09 00:32:36 tv Exp $
#	@(#)Makefile	8.2 (Berkeley) 4/4/94

# XXX Work-around a compiler bug for now... can't use -O
.if (${MACHINE_ARCH} == "powerpc")
CFLAGS=
.endif

PROG=	ftpd
SRCS=	conf.c ftpd.c ftpcmd.y logwtmp.c popen.c
CPPFLAGS+=-DHASSETPROCTITLE -Dunix
DPADD+=	${LIBCRYPT}
LDADD+=	-lcrypt
MAN=	ftpd.8
MLINKS+=ftpd.8 ftpd.conf.5
.PATH:	${.CURDIR}/../../usr.bin/ftp ${.CURDIR}/../../usr.bin/login

.include <bsd.own.mk>

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

.include <bsd.prog.mk>
