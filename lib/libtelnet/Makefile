#	from: @(#)Makefile	5.4 (Berkeley) 5/7/91
#	$Id: Makefile,v 1.3 1993/08/01 05:39:57 mycroft Exp $

LIB=	telnet
SRCS=	genget.c getent.c gettytab.c misc.c
.if exists(enc_des.c)
SRCS+=	enc_des.c encrypt.c
.endif
.if exists(kerberos.c)
SRCS+=	kerberos.c auth.c kerberos5.c
CFLAGS+= -I/usr/include/kerberosIV
.endif

.PATH:  ${.CURDIR}/../../libexec/getty
.include <bsd.lib.mk>

