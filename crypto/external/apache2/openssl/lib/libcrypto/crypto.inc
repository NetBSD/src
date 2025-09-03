#	$NetBSD: crypto.inc,v 1.2 2025/07/19 14:37:06 christos Exp $

.PATH.c:	${OPENSSLSRC}/crypto
.PATH.S:	${OPENSSLSRC}/crypto

CRYPTO_SRCS += \
asn1_dsa.c \
bsearch.c \
comp_methods.c \
context.c \
core_algorithm.c \
core_fetch.c \
core_namemap.c \
cpt_err.c \
cpuid.c \
cryptlib.c \
ctype.c \
cversion.c \
defaults.c \
der_writer.c \
deterministic_nonce.c \
ebcdic.c \
ex_data.c \
getenv.c \
indicator_core.c \
info.c \
init.c \
initthread.c \
mem.c \
mem_sec.c \
o_dir.c \
o_fopen.c \
o_init.c \
o_str.c \
o_time.c \
packet.c \
param_build.c \
param_build_set.c \
params.c \
params_dup.c \
params_from_text.c \
params_idx.c \
passphrase.c \
provider.c \
provider_child.c \
provider_conf.c \
provider_core.c \
provider_predefined.c \
punycode.c \
quic_vlint.c \
self_test_core.c \
sleep.c \
sparse_array.c \
ssl_err.c \
threads_lib.c \
threads_none.c \
threads_pthread.c \
threads_win.c \
time.c \
time.c \
trace.c \
uid.c \

SRCS += ${CRYPTO_SRCS}

.for cryptosrc in ${CRYPTO_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/crypto ${CRYPTOCPPFLAGS}
.endfor

.if !defined(CPUID)
CRYPTO_SRCS+=  mem_clr.c
.else
CRYPTO_SRCS+=  ${CPUID_SRCS}
.endif

CRYPTOCPPFLAGS+=-DOPENSSL_NO_STATIC_ENGINE
