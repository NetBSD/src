#	from: @(#)Makefile	8.2 (Berkeley) 12/15/93
#	$NetBSD: Makefile,v 1.13 2001/11/05 15:23:31 lukem Exp $

.include <bsd.own.mk>

WARNS?=	1

LIB=	telnet
SRCS=	auth.c encrypt.c genget.c getent.c misc.c

CPPFLAGS+= -DHAS_CGETENT
CPPFLAGS+= -I${.CURDIR}

.if (${MKKERBEROS} != "no")
SRCS+=	enc_des.c kerberos.c
SRCS+=	kerberos5.c

CPPFLAGS+= -DENCRYPTION -DAUTHENTICATION
CPPFLAGS+= -DKRB4 -DDES_ENCRYPTION
CPPFLAGS+= -DKRB5
CPPFLAGS+= -I${DESTDIR}/usr/include/krb5
CPPFLAGS+= -I${DESTDIR}/usr/include/kerberosIV
.endif

.include <bsd.lib.mk>
