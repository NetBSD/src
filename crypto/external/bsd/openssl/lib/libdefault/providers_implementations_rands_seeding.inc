#	$NetBSD: providers_implementations_rands_seeding.inc,v 1.1 2023/05/06 17:07:23 christos Exp $

.PATH:	${OPENSSLSRC}/providers/implementations/rands/seeding

PROVIDERS_IMPLEMENTATIONS_RANDS_SEEDING_SRCS += \
rand_cpu_x86.c \
rand_tsc.c \
rand_unix.c \
rand_win.c \

SRCS += ${PROVIDERS_IMPLEMENTATIONS_RANDS_SEEDING_SRCS}

.for defaultsrc in ${PROVIDERS_IMPLEMENTATIONS_RANDS_SEEDING_SRCS}
CPPFLAGS.${defaultsrc} += -I${OPENSSLSRC}/providers/implementations/rands/seeding ${PROVIDERS_IMPLEMENTATIONS_RANDS_SEEDINGCPPFLAGS}
.endfor
