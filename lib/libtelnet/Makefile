#	from: @(#)Makefile	8.2 (Berkeley) 12/15/93
#	$NetBSD: Makefile,v 1.10.2.2 2000/06/22 07:09:02 thorpej Exp $

LIB=	telnet
SRCS=	auth.c encrypt.c genget.c getent.c misc.c

SRCS+=	enc_des.c kerberos.c
SRCS+=	kerberos5.c

CPPFLAGS+= -DHAS_CGETENT
CPPFLAGS+= -DENCRYPTION -DAUTHENTICATION
CPPFLAGS+= -DKRB4 -DDES_ENCRYPTION
CPPFLAGS+= -DKRB5
CPPFLAGS+= -I${DESTDIR}/usr/include/krb5
CPPFLAGS+= -I${DESTDIR}/usr/include/kerberosIV
CPPFLAGS+= -I${.CURDIR}

.include <bsd.lib.mk>
