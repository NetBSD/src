#	$NetBSD: providers_implementations_kdfs.inc,v 1.1.2.2 2023/08/11 13:42:14 martin Exp $

.PATH:	${OPENSSLSRC}/providers/implementations/kdfs

PROVIDERS_IMPLEMENTATIONS_KDFS_SRCS += \
hkdf.c \
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
