#	from: @(#)Makefile	8.2 (Berkeley) 12/15/93
#	$NetBSD: Makefile,v 1.37 2017/05/21 15:28:41 riastradh Exp $

USE_FORT?= yes	# network protocol library

LIBISPRIVATE=	yes

.include <bsd.own.mk>

WARNS?=	5

LIB=	telnet
SRCS=	auth.c encrypt.c genget.c getent.c misc.c

CPPFLAGS+= -DHAS_CGETENT
CPPFLAGS+= -I${.CURDIR}

SRCS+=	enc_des.c
CPPFLAGS+= -DENCRYPTION -DAUTHENTICATION
CPPFLAGS+= -DDES_ENCRYPTION

.if ${USE_KERBEROS} != "no"
SRCS+=	kerberos5.c
CPPFLAGS+= -DKRB5
.endif

.if ${USE_PAM} != "no"
SRCS+=	sra.c pk.c
CPPFLAGS+= -DSRA
.endif

.for f in auth enc_des kerberos5 pk
COPTS.${f}.c+=  -Wno-pointer-sign
.endfor

.include <bsd.lib.mk>
