#	$NetBSD: Makefile,v 1.3 2001/01/14 05:22:33 itojun Exp $

.include <bsd.own.mk>

# Prevent recursion.
LIBSSHOBJDIR=${.OBJDIR}

LIB=	ssh
SRCS=	authfd.c authfile.c bufaux.c buffer.c canohost.c channels.c \
	cipher.c compat.c compress.c crc32.c deattack.c \
	hostfile.c log.c match.c mpaux.c nchan.c packet.c readpass.c \
	random.c rsa.c tildexpand.c ttymodes.c uidswap.c xmalloc.c \
	atomicio.c key.c dispatch.c kex.c hmac.c uuencode.c util.c \
	cli.c rijndael.c ssh-dss.c ssh-rsa.c

MKLINT=		no
MKMAN=		no
MKPIC=		no
MKPROFILE=	no

# only needed during build - prevent installation of library
libinstall::

.if (${MKKERBEROS} != "no")
CPPFLAGS+= -DKRB4 -I${DESTDIR}/usr/include/kerberosIV
#CPPFLAGS+= -DAFS
#SRCS+= radix.c
.endif

.include <bsd.lib.mk>
