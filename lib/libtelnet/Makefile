#	from: @(#)Makefile	8.2 (Berkeley) 12/15/93
#	$NetBSD: Makefile,v 1.22 2004/05/23 02:24:51 lukem Exp $

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

.include <bsd.lib.mk>
