#	from: @(#)Makefile	8.2 (Berkeley) 12/15/93
#	$NetBSD: Makefile,v 1.24 2005/02/19 22:47:10 christos Exp $

MKPRIVATELIB=	yes

.include <bsd.own.mk>

WARNS?=	1

LIB=	telnet
SRCS=	auth.c encrypt.c genget.c getent.c misc.c

CPPFLAGS+= -DHAS_CGETENT
CPPFLAGS+= -I${.CURDIR}

.if (${USE_KERBEROS} != "no")
SRCS+=	enc_des.c
CPPFLAGS+= -DENCRYPTION -DAUTHENTICATION
CPPFLAGS+= -DDES_ENCRYPTION

.if (${USE_KERBEROS4} != "no")
SRCS+=	kerberos.c
CPPFLAGS+= -DKRB4
CPPFLAGS+= -I${DESTDIR}/usr/include/kerberosIV
.endif

SRCS+=	kerberos5.c
CPPFLAGS+= -DKRB5
CPPFLAGS+= -I${DESTDIR}/usr/include/krb5
.endif

.if (${USE_PAM} != "no")
SRCS+=	sra.c pk.c
CPPFLAGS+= -DSRA
.endif

.include <bsd.lib.mk>
