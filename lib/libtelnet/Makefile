#	from: @(#)Makefile	8.2 (Berkeley) 12/15/93
#	$NetBSD: Makefile,v 1.17 2003/07/16 19:35:34 itojun Exp $

NOPIC=		# defined
NOPROFILE=	# defined
NOLINT=		# defined

.include <bsd.own.mk>

WARNS?=	1

LIB=	telnet
SRCS=	auth.c encrypt.c genget.c getent.c misc.c

CPPFLAGS+= -DHAS_CGETENT
CPPFLAGS+= -I${.CURDIR}

.if (${USE_KERBEROS} != "no")
SRCS+=	enc_des.c kerberos.c
SRCS+=	kerberos5.c

CPPFLAGS+= -DENCRYPTION -DAUTHENTICATION
CPPFLAGS+= -DKRB4 -DDES_ENCRYPTION
CPPFLAGS+= -DKRB5
CPPFLAGS+= -I${DESTDIR}/usr/include/krb5
CPPFLAGS+= -I${DESTDIR}/usr/include/kerberosIV
.endif

libinstall::

.include <bsd.lib.mk>
