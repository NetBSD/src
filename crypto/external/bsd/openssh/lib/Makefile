#	$NetBSD: Makefile,v 1.24 2018/02/04 03:19:52 christos Exp $

.include <bsd.own.mk>

.include "../Makefile.inc"

LIB=	ssh
SRCS=\
addrmatch.c \
atomicio.c \
authfd.c \
authfile.c \
bcrypt_pbkdf.c \
bitmap.c \
blocks.c \
blowfish.c \
bufaux.c \
bufbn.c \
bufec.c \
buffer.c \
canohost.c \
channels.c \
cipher-chachapoly.c \
cipher.c \
chacha.c \
cleanup.c \
compat.c \
crc32.c \
dh.c \
dispatch.c \
dns.c \
ed25519.c \
fatal.c \
fe25519.c \
ge25519.c \
hash.c \
hmac.c \
hostfile.c \
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
opacket.c \
packet.c \
poly1305.c \
progressmeter.c \
readpass.c \
recallocarray.c \
sc25519.c \
smult_curve25519_ref.c \
sshbuf-getput-basic.c \
sshbuf-misc.c \
sshbuf-getput-crypto.c \
sshbuf.c \
ssherr.c \
sshkey.c \
ssh-ed25519.c \
ssh-pkcs11.c \
ttymodes.c \
uidswap.c \
umac.c \
umac128.c \
utf8.c \
uuencode.c \
verify.c \
xmalloc.c

OPENSSL_SRCS=\
digest-openssl.c \
kexc25519.c \
kexc25519c.c \
kexc25519s.c \
ssh-dss.c \
ssh-ecdsa.c \
ssh-rsa.c

SRCS+=	fmt_scaled.c
SRCS+=	readpassphrase.c getpeereid.c getrrsetbyname.c
COPTS.monitor_fdpass.c = -Wno-stack-protector

.if WITH_OPENSSL
SRCS+=		${OPENSSL_SRCS}
.else
SRCS+=		digest-libc.c
.endif

CPPFLAGS+= -DHAVE_BLF_H

CPPFLAGS+=	-I${SSHDIST}
.PATH:		${SSHDIST}

LIBDPLIBS+=	crypto	${NETBSDSRCDIR}/crypto/external/bsd/${EXTERNAL_OPENSSL_SUBDIR}/lib/libcrypto \
		crypt	${NETBSDSRCDIR}/lib/libcrypt \
		z	${NETBSDSRCDIR}/lib/libz

.for f in dns channels hostfile ssh-pkcs11
COPTS.${f}.c+=	-Wno-pointer-sign
.endfor

# XXX
COPTS.channels.c+=	-fno-strict-aliasing

# XXX
.if ${MACHINE} == "vax"
COPTS.poly1305.c+=	-O0
COPTS.umac.c+=		-O0
.endif

.include <bsd.lib.mk>
