#	$NetBSD: providers_implementations_kdfs.inc,v 1.1 2025/07/17 14:26:09 christos Exp $

.PATH:	${OPENSSLSRC}/providers/implementations/kdfs

PROVIDERS_IMPLEMENTATIONS_KDFS_SRCS += \
argon2.c \
hkdf.c \
hmacdrbg_kdf.c \
kbkdf.c \
krb5kdf.c \
pbkdf2.c \
pbkdf2_fips.c \
pkcs12kdf.c \
scrypt.c \
sshkdf.c \
sskdf.c \
tls1_prf.c \
x942kdf.c \

SRCS += ${PROVIDERS_IMPLEMENTATIONS_KDFS_SRCS}

.for defaultsrc in ${PROVIDERS_IMPLEMENTATIONS_KDFS_SRCS}
CPPFLAGS.${defaultsrc} += -I${OPENSSLSRC}/providers/implementations/kdfs ${PROVIDERS_IMPLEMENTATIONS_KDFSCPPFLAGS}
.endfor
