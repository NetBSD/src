#	$NetBSD: Makefile,v 1.17 2003/07/23 03:52:25 itojun Exp $

NOLINT=		# defined
NOMAN=		# defined
NOPIC=		# defined
NOPROFILE=	# defined

.include <bsd.own.mk>

# Prevent recursion.
LIBSSHOBJDIR=${.OBJDIR}

LIB=	ssh
SRCS=	authfd.c authfile.c bufaux.c buffer.c canohost.c channels.c \
	cipher.c compat.c compress.c crc32.c deattack.c fatal.c \
	hostfile.c log.c match.c mpaux.c nchan.c packet.c readpass.c \
	rsa.c tildexpand.c ttymodes.c xmalloc.c atomicio.c \
	key.c dispatch.c kex.c mac.c uuencode.c misc.c \
	rijndael.c ssh-dss.c ssh-rsa.c dh.c kexdh.c kexgex.c \
	kexdhc.c kexgexc.c scard.c msg.c progressmeter.c
SRCS+=	random.c
SRCS+=	readpassphrase.c getpeereid.c

# only needed during build - prevent installation of library
libinstall::

.if (${USE_KERBEROS} != "no")
CPPFLAGS+= -DKRB5 -I${DESTDIR}/usr/include/krb5
#SRCS+= radix.c
.endif

.include <bsd.lib.mk>
