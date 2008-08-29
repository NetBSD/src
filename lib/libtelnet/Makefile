#	from: @(#)Makefile	8.2 (Berkeley) 12/15/93
#	$NetBSD: Makefile,v 1.31 2008/08/29 00:02:22 gmcgarry Exp $

USE_FORT?= yes	# network protocol library

LIBISPRIVATE=	yes

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

.if ${USE_PAM} != "no" && ${MKCRYPTO} != "no"
SRCS+=	sra.c pk.c
CPPFLAGS+= -DSRA
.endif

.if (defined(HAVE_GCC) && ${HAVE_GCC} == 4) || defined(HAVE_PCC)
.for f in auth enc_des kerberos5 pk
COPTS.${f}.c+=  -Wno-pointer-sign
.endfor
.endif

.include <bsd.lib.mk>
