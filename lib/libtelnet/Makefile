#	from: @(#)Makefile	8.2 (Berkeley) 12/15/93
#	$NetBSD: Makefile,v 1.14 2002/01/29 17:58:36 thorpej Exp $

.include <bsd.own.mk>

WARNS?=	1

LIB=	telnet
SRCS=	auth.c encrypt.c genget.c getent.c misc.c

CPPFLAGS+= -DHAS_CGETENT
CPPFLAGS+= -I${.CURDIR}

# XXX See doc/HACKS gcc-3-libtelnet
.if defined(HAVE_GCC3)
COPTS+=	-O0
.endif

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
