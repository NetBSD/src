#	$NetBSD: crypto.inc,v 1.12 2023/05/07 19:13:15 christos Exp $

.PATH:	${OPENSSLSRC}/crypto

CRYPTO_SRCS += \
asn1_dsa.c \
bsearch.c \
context.c \
core_algorithm.c \
core_fetch.c \
core_namemap.c \
cpt_err.c \
cpuid.c \
cryptlib.c \
ctype.c \
cversion.c \
der_writer.c \
ebcdic.c \
ex_data.c \
getenv.c \
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
passphrase.c \
provider.c \
provider_child.c \
provider_conf.c \
provider_core.c \
provider_predefined.c \
punycode.c \
self_test_core.c \
sparse_array.c \
threads_lib.c \
threads_none.c \
threads_pthread.c \
threads_win.c \
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
