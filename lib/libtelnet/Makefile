#	from: @(#)Makefile	8.2 (Berkeley) 12/15/93
#	$NetBSD: Makefile,v 1.6.2.1 1997/11/09 01:00:57 lukem Exp $

LIB=	telnet
SRCS=	auth.c encrypt.c genget.c getent.c misc.c
CPPFLAGS+= -DHAS_CGETENT

# These are the sources that have encryption stuff in them.
CRYPT_SRC= auth.c enc-proto.h enc_des.c encrypt.c
CRYPT_SRC+= encrypt.h kerberos.c kerberos5.c krb4encpwd.c
CRYPT_SRC+= misc.c spx.c Makefile
NOCRYPT_DIR=${.CURDIR}/Nocrypt

.include <bsd.lib.mk>

nocrypt:
	@echo "Encryption code already removed."
