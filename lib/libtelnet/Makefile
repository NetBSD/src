#	from: @(#)Makefile	8.2 (Berkeley) 12/15/93
#	$NetBSD: Makefile,v 1.12 2000/06/23 06:01:13 thorpej Exp $

.include <bsd.own.mk>

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
