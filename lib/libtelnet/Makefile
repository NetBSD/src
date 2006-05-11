#	from: @(#)Makefile	8.2 (Berkeley) 12/15/93
#	$NetBSD: Makefile,v 1.26 2006/05/11 23:16:29 mrg Exp $

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

SRCS+=	kerberos5.c
CPPFLAGS+= -DKRB5
CPPFLAGS+= -I${DESTDIR}/usr/include/krb5
.endif

.if (${USE_PAM} != "no")
SRCS+=	sra.c pk.c
CPPFLAGS+= -DSRA
.endif

.if ${HAVE_GCC} == 4
.for f in auth enc_des kerberos5 pk
COPTS.${f}.c+=  -Wno-pointer-sign
.endfor
.endif

.include <bsd.lib.mk>
