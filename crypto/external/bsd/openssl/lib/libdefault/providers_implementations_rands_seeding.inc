#	$NetBSD: providers_implementations_rands_seeding.inc,v 1.1.2.2 2023/08/11 13:42:14 martin Exp $

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
