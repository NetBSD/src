#	$NetBSD: Makefile,v 1.1 2000/07/25 16:32:25 thorpej Exp $

.include <bsd.own.mk>

# Prevent recursion.
LIBSSHOBJDIR=${.OBJDIR}

LIB=	ssh
SRCS=	authfd.c authfile.c bufaux.c buffer.c canohost.c channels.c \
	cipher.c compat.c compress.c crc32.c deattack.c fingerprint.c \
	hostfile.c log.c match.c mpaux.c nchan.c packet.c readpass.c \
	random.c rsa.c tildexpand.c ttymodes.c uidswap.c xmalloc.c \
	atomicio.c key.c dispatch.c dsa.c kex.c hmac.c uuencode.c aux.c

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
