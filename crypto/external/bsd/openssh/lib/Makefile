#	$NetBSD: Makefile,v 1.12 2013/03/29 16:19:46 christos Exp $

.include <bsd.own.mk>

LIB=	ssh
SRCS=\
addrmatch.c \
atomicio.c \
authfd.c \
authfile.c \
bufaux.c \
bufbn.c \
bufec.c \
buffer.c \
canohost.c \
channels.c \
cipher-3des1.c \
cipher-bf1.c \
cipher-ctr.c \
cipher.c \
cleanup.c \
compat.c \
compress.c \
crc32.c \
deattack.c \
dh.c \
dispatch.c \
dns.c \
fatal.c \
hostfile.c \
jpake.c \
kex.c \
kexdh.c \
kexdhc.c \
kexecdh.c \
kexecdhc.c \
kexgex.c \
kexgexc.c \
key.c \
krl.c \
log.c \
mac.c \
match.c \
misc.c \
monitor_fdpass.c \
msg.c \
nchan.c \
packet.c \
progressmeter.c \
readpass.c \
roaming_dummy.c \
rsa.c \
schnorr.c \
ssh-dss.c \
ssh-ecdsa.c \
ssh-pkcs11.c \
ssh-rsa.c \
ttymodes.c \
uidswap.c \
uuencode.c \
xmalloc.c

#	umac.c
SRCS+=	fmt_scaled.c strtonum.c
SRCS+=	readpassphrase.c getpeereid.c getrrsetbyname.c
COPTS.monitor_fdpass.c = -Wno-stack-protector

SSHDIST=	${NETBSDSRCDIR}/crypto/external/bsd/openssh/dist
CPPFLAGS+=	-I${SSHDIST}
.PATH:		${SSHDIST}

LIBDPLIBS+=	crypto	${NETBSDSRCDIR}/crypto/external/bsd/openssl/lib/libcrypto \
		crypt	${NETBSDSRCDIR}/lib/libcrypt \
		z	${NETBSDSRCDIR}/lib/libz

.for f in dns channels hostfile ssh-pkcs11
COPTS.${f}.c+=	-Wno-pointer-sign
.endfor

# XXX
COPTS.channels.c+=	-fno-strict-aliasing

.include <bsd.lib.mk>
