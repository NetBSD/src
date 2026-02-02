#	$NetBSD: Makefile,v 1.42.2.2 2026/02/02 18:08:03 martin Exp $

# libssh is not meant for applications to link against -- it's a
# private library of the ssh/sshd/&c. executables we ship.  So don't
# install compat versions (we don't ship ssh/sshd/&c. as compat
# executables, only as native ones) or expose the .so symlink or .a
# static library for applications to link against.
#
# Note that while NOLINKLIB suppresses _installing_ the .so symlink,
# the .so symlink is still created in the objdir so bin/ssh can still
# use it to link.
NOLINKLIB=		# defined

.include <bsd.own.mk>

.include "../Makefile.inc"

CWARNFLAGS.clang+=	-Wno-error=sizeof-array-div
CWARNFLAGS.clang+=	-Wno-error=format-nonliteral
LIB=	ssh
SRCS=\
addr.c \
addrmatch.c \
atomicio.c \
authfd.c \
authfile.c \
bcrypt_pbkdf.c \
bitmap.c \
blowfish.c \
canohost.c \
chacha.c \
channels.c \
cipher-chachapoly.c \
cipher.c \
cleanup.c \
compat.c \
dispatch.c \
dns.c \
ed25519.c \
fatal.c \
freezero.c \
hash.c \
hmac.c \
hostfile.c \
kex.c \
kexc25519.c \
kex-names.c \
kexgen.c \
kexmlkem768x25519.c \
kexsntrup761x25519.c \
krl.c \
log.c \
mac.c \
match.c \
misc.c \
monitor_fdpass.c \
msg.c \
nchan.c \
packet.c \
poly1305.c \
progressmeter.c \
readpass.c \
recallocarray.c \
smult_curve25519_ref.c \
sntrup761.c \
ssh-ed25519.c \
ssh-ed25519-sk.c \
ssh-pkcs11.c \
ssh-sk-client.c \
sshbuf-getput-basic.c \
sshbuf-misc.c \
sshbuf-io.c \
sshbuf.c \
ssherr.c \
sshkey.c \
ttymodes.c \
uidswap.c \
umac.c \
umac128.c \
utf8.c \
xmalloc.c 

OPENSSL_SRCS=\
dh.c \
digest-openssl.c \
kexdh.c \
kexecdh.c \
kexgex.c \
kexgexc.c \
sshbuf-getput-crypto.c \
ssh-ecdsa.c \
ssh-ecdsa-sk.c \
ssh-rsa.c

SRCS+=	fmt_scaled.c
SRCS+=	readpassphrase.c getpeereid.c getrrsetbyname.c
COPTS.monitor_fdpass.c+=-Wno-stack-protector
COPTS.sntrup761.c+=	-Wno-stack-protector

.ifdef WITH_OPENSSL
SRCS+=		${OPENSSL_SRCS}
LIBDPLIBS+=	crypto	${NETBSDSRCDIR}/crypto/external/${EXTERNAL_OPENSSL_SUBDIR}/lib/libcrypto
.else
SRCS+=		cipher-aesctr.c digest-libc.c rijndael.c
.endif

CPPFLAGS+= -DHAVE_BLF_H

CPPFLAGS+=	-I${SSHDIST}
.PATH:		${SSHDIST}

LIBDPLIBS+=	crypt	${NETBSDSRCDIR}/lib/libcrypt \
		z	${NETBSDSRCDIR}/lib/libz

.for f in dns channels hostfile ssh-pkcs11
COPTS.${f}.c+=	-Wno-pointer-sign
.endfor

# XXX
COPTS.channels.c+=	-fno-strict-aliasing

COPTS.hostfile.c+=	${CC_WNO_FORMAT_TRUNCATION}
COPTS.sshkey.c+=	${CC_WNO_FORMAT_TRUNCATION}
COPTS.cipher.c+=	-Wno-error=deprecated-declarations
COPTS.dh.c+=		-Wno-error=deprecated-declarations
COPTS.digest-libc.c+=	-Wno-error=cast-function-type
COPTS.kex.c+=		-Wno-error=deprecated-declarations
COPTS.kexdh.c+=		-Wno-error=deprecated-declarations
COPTS.kexecdh.c+=	-Wno-error=deprecated-declarations
COPTS.kexgexc.c+=	-Wno-error=deprecated-declarations
COPTS.ssh-ecdsa-sk.c+=	-Wno-error=deprecated-declarations
COPTS.ssh-ecdsa.c+=	-Wno-error=deprecated-declarations
COPTS.ssh-pkcs11.c+=	-Wno-error=deprecated-declarations
COPTS.ssh-rsa.c+=	-Wno-error=deprecated-declarations
COPTS.sshbuf-getput-crypto.c+=	-Wno-error=deprecated-declarations
COPTS.sshkey.c+=	-Wno-error=deprecated-declarations
COPTS.umac.c+=		-Wno-error=deprecated-declarations
COPTS.umac128.c+=	-Wno-error=deprecated-declarations
COPTS.kexmlkem768x25519.c+=${${ACTIVE_CC} == "clang":? -Wno-error=missing-noreturn:}



.include <bsd.lib.mk>
