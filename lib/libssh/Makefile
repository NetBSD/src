#	$NetBSD: Makefile,v 1.1 2005/01/03 06:00:12 lukem Exp $

.include <bsd.own.mk>

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

WARNS=	1

SSHDIST=	${NETBSDSRCDIR}/crypto/dist/ssh
CPPFLAGS+=	-I${SSHDIST} -DHAVE_LOGIN_CAP
.PATH:		${SSHDIST}


LIBDPLIBS=	crypto	${.CURDIR}/../libcrypto \
		z	${.CURDIR}/../libz

.if (${USE_KERBEROS} != "no")
CPPFLAGS+=	-DKRB5 -I${DESTDIR}/usr/include/krb5
.if (${USE_KERBEROS4} != "no")
SRCS+=		radix.c
CPPFLAGS+=	-DKRB4 -DAFS -I${DESTDIR}/usr/include/kerberosIV
LIBDPLIBS+=	krb	${.CURDIR}/../libkrb \
		des	${.CURDIR}/../libdes
.endif	# ${USE_KERBEROS4} != "no"
LIBDPLIBS+=	com_err	${.CURDIR}/../libcom_err \
		roken	${.CURDIR}/../libroken
.endif	# ${USE_KERBEROS} != "no"

.include <bsd.lib.mk>
