#	$NetBSD: Makefile,v 1.8 2001/09/27 03:24:01 itojun Exp $

.include <bsd.own.mk>

# Prevent recursion.
LIBSSHOBJDIR=${.OBJDIR}

LIB=	ssh
SRCS=	authfd.c authfile.c bufaux.c buffer.c canohost.c \
	channels.c cipher.c compat.c compress.c crc32.c deattack.c \
	hostfile.c log.c match.c mpaux.c nchan.c packet.c readpass.c \
	rsa.c tildexpand.c ttymodes.c uidswap.c xmalloc.c atomicio.c \
	key.c dispatch.c kex.c mac.c uuencode.c misc.c \
	rijndael.c ssh-dss.c ssh-rsa.c dh.c kexdh.c kexgex.c \
	scard.c
SRCS+=	random.c
SRCS+=	readpassphrase.c

MKLINT=		no
MKMAN=		no
MKPIC=		no
MKPROFILE=	no

# only needed during build - prevent installation of library
libinstall::

.if (${MKKERBEROS} != "no")
CPPFLAGS+= -DKRB5 -DAFS -I${DESTDIR}/usr/include/krb5
CPPFLAGS+= -DKRB4 -I${DESTDIR}/usr/include/kerberosIV
SRCS+= radix.c
.endif

.include <bsd.lib.mk>
