#	$NetBSD: stack.inc,v 1.1 2025/07/17 14:25:26 christos Exp $

.PATH.c:	${OPENSSLSRC}/crypto/stack

STACK_SRCS += \
stack.c \

SRCS += ${STACK_SRCS}

.for cryptosrc in ${STACK_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/stack ${STACKCPPFLAGS}
.endfor
