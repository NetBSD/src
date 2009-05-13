#	$NetBSD: Makefile,v 1.16.6.1 2009/05/13 19:18:36 jym Exp $

.include <bsd.own.mk>

LIB=	ssh
SRCS=	authfd.c authfile.c bufaux.c bufbn.c buffer.c canohost.c channels.c \
	cipher.c cipher-3des1.c cipher-ctr.c cipher-ctr-mt.c cipher-bf1.c \
	cleanup.c compat.c compress.c crc32.c deattack.c dns.c fatal.c \
	hostfile.c log.c match.c md-sha256.c nchan.c packet.c readpass.c \
	rsa.c strtonum.c ttymodes.c xmalloc.c atomicio.c \
	key.c dispatch.c kex.c mac.c uuencode.c misc.c \
	ssh-dss.c ssh-rsa.c dh.c kexdh.c kexgex.c \
	kexdhc.c kexgexc.c scard.c msg.c progressmeter.c \
	monitor_fdpass.c uidswap.c addrmatch.c
#	umac.c
SRCS+=	random.c
SRCS+=	readpassphrase.c getpeereid.c getrrsetbyname.c
COPTS.monitor_fdpass.c = -Wno-stack-protector

WARNS=	1

SSHDIST=	${NETBSDSRCDIR}/crypto/dist/ssh
CPPFLAGS+=	-I${SSHDIST}
.PATH:		${SSHDIST}


LIBDPLIBS+=	crypto	${.CURDIR}/../libcrypto \
		crypt	${.CURDIR}/../libcrypt \
		z	${.CURDIR}/../libz

.if (defined(HAVE_GCC) && ${HAVE_GCC} == 4) || defined(HAVE_PCC)
.for f in dns channels hostfile
COPTS.${f}.c+=	-Wno-pointer-sign
.endfor
.endif

.include <bsd.lib.mk>
