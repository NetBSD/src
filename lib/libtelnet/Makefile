#	@(#)Makefile	5.4 (Berkeley) 5/7/91

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

