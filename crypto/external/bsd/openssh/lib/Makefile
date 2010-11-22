#	$NetBSD: Makefile,v 1.6 2010/11/22 22:19:54 christos Exp $

.include <bsd.own.mk>

LIB=	ssh
SRCS=	authfd.c authfile.c bufaux.c bufbn.c buffer.c canohost.c channels.c \
	cipher.c cipher-3des1.c cipher-bf1.c cipher-ctr.c \
	cleanup.c compat.c compress.c crc32.c deattack.c fatal.c \
	hostfile.c log.c match.c nchan.c packet.c readpass.c \
	rsa.c ttymodes.c xmalloc.c atomicio.c \
	key.c dispatch.c kex.c mac.c uidswap.c uuencode.c misc.c \
	ssh-dss.c ssh-rsa.c dh.c kexdh.c kexgex.c \
	kexdhc.c kexgexc.c msg.c progressmeter.c dns.c \
	monitor_fdpass.c addrmatch.c schnorr.c jpake.c ssh-pkcs11.c \
	roaming_dummy.c
#	umac.c
SRCS+=	fmt_scaled.c random.c strtonum.c
SRCS+=	readpassphrase.c getpeereid.c getrrsetbyname.c
COPTS.monitor_fdpass.c = -Wno-stack-protector

WARNS=	1

SSHDIST=	${NETBSDSRCDIR}/crypto/external/bsd/openssh/dist
CPPFLAGS+=	-I${SSHDIST}
.PATH:		${SSHDIST}

LIBDPLIBS+=	crypto	${NETBSDSRCDIR}/crypto/external/bsd/openssl/lib/libcrypto \
		crypt	${NETBSDSRCDIR}/lib/libcrypt \
		z	${NETBSDSRCDIR}/lib/libz

.if (defined(HAVE_GCC) && ${HAVE_GCC} == 4) || defined(HAVE_PCC)
.for f in dns channels hostfile roaming_common ssh-pkcs11
COPTS.${f}.c+=	-Wno-pointer-sign
.endfor
.endif

.include <bsd.lib.mk>
