#	from: @(#)Makefile	8.2 (Berkeley) 12/15/93
#	$NetBSD: Makefile,v 1.9.6.1 2000/06/23 16:16:54 minoura Exp $

# XXX XXX XXX
WARNS=0

DIST=	${.CURDIR}/../../crypto/dist
.PATH:	${DIST}/heimdal/appl/telnet/libtelnet .DOTLAST

LIB=	telnet
SRCS=	auth.c enc_des.c encrypt.c genget.c getent.c misc.c \
	kerberos.c kerberos5.c

CPPFLAGS+= -DHAS_CGETENT
CPPFLAGS+= -I${DIST}/heimdal/appl/telnet/libtelnet
CPPFLAGS+= -I${.CURDIR}/../../include/heimdal
CPPFLAGS+= -I${DESTDIR}/usr/include/kerberosIV
CPPFLAGS+= -I${DESTDIR}/usr/include/krb5
CPPFLAGS+= -I${DESTDIR}/usr/include/krb
CPPFLAGS+= -I${DESTDIR}/usr/include/openssl
CPPFLAGS+= -DHAVE_CONFIG_H

.include <bsd.lib.mk>
